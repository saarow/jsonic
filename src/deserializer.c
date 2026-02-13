#include "../include/deserializer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static JsonValue *find_value(const JsonObject *obj, const char *key) {
    for (size_t i = 0; i < obj->size; i++) {
        if (strcmp(key, obj->keys[i]) == 0) {
            return obj->values[i];
        }
    }
    return NULL;
}
bool json_to_struct(void *struct_ptr, const JsonObject *json,
                    const FieldDescriptor *fields, size_t num_fields,
                    JsonError *error) {
    if (error) {
        error->key = NULL;
        error->message[0] = '\0';
    }

    for (size_t i = 0; i < num_fields; i++) {
        const FieldDescriptor *field = &fields[i];
        JsonValue *value = find_value(json, field->key);
        if (!value) {
            error->key = field->key;
            snprintf(error->message, sizeof(error->message),
                     "Key '%s' not found in JSON", field->key);
            return false;
        }

        void *field_ptr = (char *)struct_ptr + field->offset;
        switch (field->type) {
        case FIELD_INT:
            if (value->type != JSON_NUMBER) {
                if (error) {
                    error->key = field->key;
                    snprintf(error->message, sizeof(error->message),
                             "Expected number for key '%s', got %d", field->key,
                             value->type);
                }
                return false;
            }

            int *int_ptr = (int *)field_ptr;
            *int_ptr = (int)value->number;
            break;

        case FIELD_STRING:
            if (value->type != JSON_STRING) {
                if (error) {
                    error->key = field->key;
                    snprintf(error->message, sizeof(error->message),
                             "Expected string for key '%s', got %d", field->key,
                             value->type);
                }
                return false;
            }

            char **str_ptr = (char **)field_ptr;
            *str_ptr = strdup(value->string);
            if (!*str_ptr) {
                if (error) {
                    error->key = field->key;
                    snprintf(error->message, sizeof(error->message),
                             "Out of memory copying string for key '%s'",
                             field->key);
                }
                return false;
            }
            break;

        default:
            if (error) {
                error->key = field->key;
                snprintf(error->message, sizeof(error->message),
                         "Unknown field type %d", field->type);
            }
            return false;
        }
    }

    return true;
}

void json_free_struct(void *struct_ptr, const FieldDescriptor *fields,
                      size_t num_fields) {
    if (!struct_ptr || !fields) {
        return;
    }

    for (size_t i = 0; i < num_fields; i++) {
        const FieldDescriptor *field = &fields[i];
        void *field_ptr = (char *)struct_ptr + field->offset;

        switch (field->type) {
        case FIELD_STRING: {
            char **str_ptr = (char **)field_ptr;
            free(*str_ptr);
            *str_ptr = NULL;
            break;
        }
        case FIELD_INT:
            break;
        }
    }
}
