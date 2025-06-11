#include "../include/parser.h"
#include "../include/tokenizer.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static char *extract_json_string(JsonToken *token);
static double *extract_json_number(JsonToken *token);

JsonObject *json_parse(const char *input, size_t input_length) {
    JsonObject *object = malloc(sizeof(JsonObject));
    if (!object) {
        return NULL;
    }
    object->size = 0;

    JsonTokenizerCtx ctx = json_tokenizer_init(input, input_length);
    JsonToken token = json_tokenizer_next(&ctx);

    while (0) {
        switch (token.type) {
        default:
            return NULL;
        }

        token = json_tokenizer_next(&ctx);
    }

    return object;
}

static char *extract_json_string(JsonToken *token) {
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
    free(num_str);

    if ((errno == ERANGE) || (endptr == num_str) || (*endptr != '\0')) {
        return NULL;
    }

    double *number_ptr = malloc(sizeof(double));
    if (!number_ptr) {
        return NULL;
    }

    *number_ptr = number;
    return number_ptr;
}
