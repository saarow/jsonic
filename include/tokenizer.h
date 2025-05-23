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
} TokenType;

typedef enum {
    TOKEN_ERR_NONE,
} TokenErr;

typedef struct {
        const char *input;
        size_t input_length;
        size_t pos;
        size_t line;
        size_t column;
} TokenizerCtx;

typedef struct {
        TokenType type;
        const char *start;
        size_t length;
        size_t line;
        size_t column;
} Token;

static bool is_whitespace(const char c);
static char peek(const TokenizerCtx *ctx, size_t offset);
static void advance(TokenizerCtx *ctx);

static Token create_simple_token(TokenType type, TokenizerCtx *ctx);
static Token create_invalid_token(TokenizerCtx *ctx);

static Token extract_string(TokenizerCtx *ctx);
static Token extract_number(TokenizerCtx *ctx);
static Token extract_literal(TokenizerCtx *ctx, const char *literal, size_t len,
                             TokenType type);

Token next_token(TokenizerCtx *ctx);
