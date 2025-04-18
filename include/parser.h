#pragma once

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
        JsonValue **values;
        size_t size;
} JsonArray;

typedef struct {
        char **keys;
        JsonValue **values;
        size_t size;
} JsonObject;

struct JsonValue {
        JsonType type;
        union {
                char *string;
                double number;
                bool boolean;
                JsonArray array;
                JsonObject object;
        };
};
