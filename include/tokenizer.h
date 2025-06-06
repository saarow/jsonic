#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    TOKEN_EOF,
    TOKEN_INVALID
} JsonTokenType;

typedef struct {
        const char *input;
        size_t input_length;
        size_t pos;
        size_t line;
        size_t column;
} JsonTokenizerCtx;

typedef struct {
        JsonTokenType type;
        const char *start;
        size_t length;
        size_t line;
        size_t column;
} JsonToken;

JsonTokenizerCtx json_tokenizer_init(const char *json_input, size_t length);
JsonToken json_tokenizer_next(JsonTokenizerCtx *ctx);
