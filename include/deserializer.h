#pragma once

#include "parser.h"
#include <stddef.h>

typedef enum {
    FIELD_INT,
    FIELD_STRING,
} FieldType;

typedef struct {
        const char *key;
        FieldType type;
        size_t offset;
} FieldDescriptor;

typedef struct {
        const char *key;
        char message[256];
} JsonError;

bool json_to_struct(void *struct_ptr, const JsonObject *json,
                    const FieldDescriptor *fields, size_t num_fields,
                    JsonError *error);

void json_free_struct(void *struct_ptr, const FieldDescriptor *fields,
                      size_t num_fields);
