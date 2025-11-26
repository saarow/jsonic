#include "../include/parser.h"
#include "../include/tokenizer.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static char *extract_json_string(JsonToken *token);
static double *extract_json_number(JsonToken *token);
static JsonValue *extract_json_value(JsonToken *token);
static JsonArray *extract_json_array(JsonTokenizerCtx *ctx, bool is_nested);
static JsonObject *extract_json_object(JsonTokenizerCtx *ctx, bool is_nested);

JsonObject *json_parse(const char *input, size_t input_length) {
    JsonTokenizerCtx ctx = json_tokenizer_init(input, input_length);

    JsonObject *object = extract_json_object(&ctx, false);

    if (object) {
        JsonToken token = json_tokenizer_next(&ctx);
        if (token.type != TOKEN_EOF) {
            free_json_value((JsonValue *)object);
            return NULL;
        }
    }

    return object;
}

static char *extract_json_string(JsonToken *token) {
    if (token->length <= 2) {
        return strdup("");
    }

    char *string = strndup(token->start + 1, token->length - 2);
    if (!string) {
        return NULL;
    }

    return string;
}

static double *extract_json_number(JsonToken *token) {
    char *num_str = strndup(token->start, token->length);
    if (!num_str) {
        return NULL;
    }

    char *endptr;
    errno = 0;
    double number = strtod(num_str, &endptr);

    if ((errno == ERANGE) || (endptr == num_str)) {
        free(num_str);
        return NULL;
    }

    double *number_ptr = malloc(sizeof(double));
    if (!number_ptr) {
        return NULL;
    }

    free(num_str);
    *number_ptr = number;

    return number_ptr;
}

static JsonArray *extract_json_array(JsonTokenizerCtx *ctx, bool is_nested) {
    JsonToken token;
    if (!is_nested) {
        token = json_tokenizer_next(ctx);
        if (token.type != TOKEN_LEFT_BRACKET) {
            return NULL;
        }
        token = json_tokenizer_next(ctx);
    } else {
        token = json_tokenizer_next(ctx);
    }

    JsonArray *array = malloc(sizeof(JsonArray));
    if (!array) {
        return NULL;
    }
    array->size = 0;
    array->values = NULL;

    while (token.type != TOKEN_RIGHT_BRACKET && token.type != TOKEN_EOF) {
        JsonValue **new_values =
            realloc(array->values, sizeof(JsonValue *) * (array->size + 1));
        if (!new_values) {
            goto error_cleanup;
        }
        array->values = new_values;

        array->values[array->size] = malloc(sizeof(JsonValue));
        if (!array->values[array->size]) {
            goto error_cleanup;
        }

        switch (token.type) {
        case TOKEN_STRING: {
            array->values[array->size]->type = JSON_STRING;
            array->values[array->size]->string = extract_json_string(&token);
            if (!array->values[array->size]->string) {
                goto error_cleanup;
            }
            break;
        }

        case TOKEN_NUMBER: {
            double *num_ptr = extract_json_number(&token);
            if (!num_ptr) {
                goto error_cleanup;
            }
            array->values[array->size]->type = JSON_NUMBER;
            array->values[array->size]->number = *num_ptr;
            free(num_ptr);
            break;
        }

        case TOKEN_LEFT_BRACE: {
            JsonObject *obj = extract_json_object(ctx, true);
            if (!obj) {
                goto error_cleanup;
            }
            array->values[array->size]->type = JSON_OBJECT;
            array->values[array->size]->object = *obj;
            free(obj);
            break;
        }

        case TOKEN_LEFT_BRACKET: {
            JsonArray *arr = extract_json_array(ctx, true);
            if (!arr) {
                goto error_cleanup;
            }
            array->values[array->size]->type = JSON_ARRAY;
            array->values[array->size]->array = *arr;
            free(arr);
            break;
        }

        case TOKEN_TRUE:
            array->values[array->size]->type = JSON_BOOL;
            array->values[array->size]->boolean = true;
            break;

        case TOKEN_FALSE:
            array->values[array->size]->type = JSON_BOOL;
            array->values[array->size]->boolean = false;
            break;

        case TOKEN_NULL:
            array->values[array->size]->type = JSON_NULL;
            array->values[array->size]->null = NULL;
            break;

        case TOKEN_COMMA:
            goto error_cleanup;

        default:
            goto error_cleanup;
        }

        array->size++;
        token = json_tokenizer_next(ctx);

        if (token.type == TOKEN_COMMA) {
            token = json_tokenizer_next(ctx);
            if (token.type == TOKEN_RIGHT_BRACKET) {
                goto error_cleanup;
            }
        }
    }

    if (token.type != TOKEN_RIGHT_BRACKET) {
        goto error_cleanup;
    }

    return array;

error_cleanup:
    if (array) {
        if (array->values) {
            for (size_t i = 0; i < array->size; i++) {
                if (array->values[i]) {
                    free_json_value(array->values[i]);
                }
            }
            free(array->values);
        }
        free(array);
    }

    return NULL;
}

static JsonObject *extract_json_object(JsonTokenizerCtx *ctx, bool is_nested) {
    JsonToken token;
    if (!is_nested) {
        token = json_tokenizer_next(ctx);
        if (token.type != TOKEN_LEFT_BRACE) {
            return NULL;
        }
        token = json_tokenizer_next(ctx);
    } else {
        token = json_tokenizer_next(ctx);
    }

    JsonObject *object = malloc(sizeof(JsonObject));
    if (!object) {
        return NULL;
    }
    object->size = 0;
    object->keys = NULL;
    object->values = NULL;

    while (token.type != TOKEN_RIGHT_BRACE) {
        char **new_keys =
            realloc(object->keys, sizeof(char *) * (object->size + 1));
        JsonValue **new_values =
            realloc(object->values, sizeof(JsonValue *) * (object->size + 1));
        if (!new_keys || !new_values) {
            goto error_cleanup;
        }
        object->keys = new_keys;
        object->values = new_values;

        if (token.type != TOKEN_STRING) {
            goto error_cleanup;
        }
        object->keys[object->size] = extract_json_string(&token);
        if (!object->keys[object->size]) {
            goto error_cleanup;
        }

        token = json_tokenizer_next(ctx);
        if (token.type != TOKEN_COLON) {
            goto error_cleanup;
        }

        token = json_tokenizer_next(ctx);

        object->values[object->size] = malloc(sizeof(JsonValue));
        if (!object->values[object->size]) {
            goto error_cleanup;
        }

        switch (token.type) {
        case TOKEN_STRING: {
            object->values[object->size]->type = JSON_STRING;
            object->values[object->size]->string = extract_json_string(&token);
            if (!object->values[object->size]->string) {
                goto error_cleanup;
            }
            break;
        }

        case TOKEN_NUMBER: {
            double *num_ptr = extract_json_number(&token);
            if (!num_ptr) {
                goto error_cleanup;
            }
            object->values[object->size]->type = JSON_NUMBER;
            object->values[object->size]->number = *num_ptr;
            free(num_ptr);
            break;
        }

        case TOKEN_LEFT_BRACE: {
            JsonObject *obj = extract_json_object(ctx, true);
            if (!obj) {
                goto error_cleanup;
            }
            object->values[object->size]->type = JSON_OBJECT;
            object->values[object->size]->object = *obj;
            free(obj);
            break;
        }

        case TOKEN_LEFT_BRACKET: {
            JsonArray *arr = extract_json_array(ctx, true);
            if (!arr) {
                goto error_cleanup;
            }
            object->values[object->size]->type = JSON_ARRAY;
            object->values[object->size]->array = *arr;
            free(arr);
            break;
        }

        case TOKEN_TRUE:
            object->values[object->size]->type = JSON_BOOL;
            object->values[object->size]->boolean = true;
            break;

        case TOKEN_FALSE:
            object->values[object->size]->type = JSON_BOOL;
            object->values[object->size]->boolean = false;
            break;

        case TOKEN_NULL:
            object->values[object->size]->type = JSON_NULL;
            object->values[object->size]->null = NULL;
            break;

        default:
            goto error_cleanup;
        }

        object->size++;
        token = json_tokenizer_next(ctx);
        if (token.type == TOKEN_COMMA) {
            token = json_tokenizer_next(ctx);
        }
    }

    return object;

error_cleanup:
    if (object) {
        if (object->keys) {
            for (size_t i = 0; i < object->size; i++) {
                if (object->keys[i]) {
                    free(object->keys[i]);
                }
            }
            free(object->keys);
        }

        if (object->values) {
            for (size_t i = 0; i < object->size; i++) {
                if (object->values[i]) {
                    free_json_value(object->values[i]);
                }
            }
            free(object->values);
        }

        free(object);
    }
    return NULL;
}

void free_json_value(JsonValue *value) {
    if (!value) {
        return;
    }

    switch (value->type) {
    case JSON_STRING:
        free(value->string);
        break;

    case JSON_ARRAY:
        for (size_t i = 0; i < value->array.size; i++) {
            free_json_value(value->array.values[i]);
        }
        free(value->array.values);
        break;

    case JSON_OBJECT:
        for (size_t i = 0; i < value->object.size; i++) {
            free(value->object.keys[i]);
            free_json_value(value->object.values[i]);
        }
        free(value->object.keys);
        free(value->object.values);
        break;

    case JSON_NUMBER:
    case JSON_BOOL:
    case JSON_NULL:
        break;
    }

    free(value);
}
