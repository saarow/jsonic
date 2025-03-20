#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TOKEN_INVALID,
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
    TOKEN_EOF
} TokenType;

typedef enum {
    ERR_NONE,
    ERR_UNTERMINATED_STRING,
    ERR_INVALID_ESCAPE_SEQUENCE,
    ERR_INVALID_UNICODE_ESCAPE,
    ERR_CONTROL_CHARACTER_IN_STRING,
    ERR_INVALID_NUMBER_FORMAT,
    ERR_TRAILING_DECIMAL_POINT,
    ERR_INCOMPLETE_EXPONENT,
    ERR_UNEXPECTED_CHARACTER,
    ERR_INVALID_LITERAL,
    ERR_UNTERMINATED_JSON
} ErrorCode;

typedef struct {
        TokenType type;
        const char *start;
        size_t length;
        size_t line;
        size_t column;
        ErrorCode error;
        const char *error_pos;
} Token;

typedef struct {
        const char *input;
        size_t input_length;
        size_t pos;
        size_t line;
        size_t column;
} TokenizerCtx;

static bool is_whitespace(const char c);
static char peek(const TokenizerCtx *ctx, size_t offset);
static void advance(TokenizerCtx *ctx);
static const char *get_error_message(const ErrorCode error);
static void print_error_context(const Token *token, const char *input,
                                size_t input_length);
static bool is_valid_utf8_sequence(const char *input, size_t input_size,
                                   size_t *length);

static Token create_simple_token(TokenType type, TokenizerCtx *ctx);
static Token create_invalid_token(const TokenizerCtx *ctx, ErrorCode error);
static Token extract_string(TokenizerCtx *ctx);
static Token extract_number(TokenizerCtx *ctx);
static Token extract_literal(TokenizerCtx *ctx, const char *literal, size_t len,
                             TokenType type);

Token next_token(TokenizerCtx *ctx);
