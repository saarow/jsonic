#pragma once

#include "../include/tokenizer.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOL,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_NULL
} JsonType;

typedef struct JsonValue JsonValue;

typedef struct {
        char **keys;
        JsonValue **values;
        size_t size;
} JsonObject;

typedef struct {
        JsonValue **values;
        size_t size;
} JsonArray;

struct JsonValue {
        JsonType type;
        union {
                char *string;
                double number;
                bool boolean;
                JsonArray array;
                JsonObject object;
                void *null;
        };
};

JsonObject *json_parse(const char *input, size_t input_length);
void free_json_value(JsonValue *value);
