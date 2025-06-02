#include "../include/tokenizer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static bool is_whitespace(const char c) { return isspace((unsigned char)c); }

static char peek(const TokenizerCtx *ctx, size_t offset) {
    if (ctx->pos + offset >= ctx->input_length) {
        return '\0';
    }
    return ctx->input[ctx->pos + offset];
}

static void advance(TokenizerCtx *ctx) {
    if (ctx->pos >= ctx->input_length) {
        return;
    }

    char c = ctx->input[ctx->pos];
    if (c == '\n') {
        ctx->line++;
        ctx->column = 0;
    } else if (c == '\r') {
        if (ctx->pos + 1 < ctx->input_length &&
            ctx->input[ctx->pos + 1] == '\n') {
            ctx->pos++;
        }
        ctx->line++;
        ctx->column = 0;
    } else {
        ctx->column++;
    }

    ctx->pos++;
}

static Token create_simple_token(TokenType type, TokenizerCtx *ctx) {
    Token token = {
        .type = type,
        .start = ctx->input + ctx->pos,
        .length = 1,
        .line = ctx->line,
        .column = ctx->column,
    };

    advance(ctx);
    return token;
}

static Token create_invalid_token(TokenizerCtx *ctx) {
    Token token = {
        .type = TOKEN_INVALID,
        .start = NULL,
        .length = 0,
        .line = ctx->line,
        .column = ctx->column,
    };

    return token;
}

static Token extract_string(TokenizerCtx *ctx) {
    size_t start_pos = ctx->pos;
    size_t start_line = ctx->line;
    size_t start_column = ctx->column;

    if (peek(ctx, 0) != '"') {
        return create_invalid_token(ctx);
    }
    advance(ctx);

    while (peek(ctx, 0) != '\0') {
        char c = peek(ctx, 0);
        if (c < 0x20) { // Control characters invalid unless escaped
            return create_invalid_token(ctx);
        }

        if (c == '"') {
            advance(ctx);
            return (Token){
                .type = TOKEN_STRING,
                .start = ctx->input + start_pos,
                .length = ctx->pos - start_pos,
                .line = start_line,
                .column = start_column,
            };
        }

        if (c == '\\') {
            advance(ctx);
            char escaped = peek(ctx, 0);
            static const char valid_escapes[] = "\"\\/bfnrt";
            if (strchr(valid_escapes, escaped)) {
                advance(ctx);
            } else if (escaped == 'u') {
                advance(ctx);
                for (size_t i = 0; i < 4; i++) {
                    if (!isxdigit(peek(ctx, 0))) {
                        return create_invalid_token(ctx);
                    }
                    advance(ctx);
                }
                // TODO: Validate UTF-8
            } else {
                return create_invalid_token(ctx);
            }
        } else {
            advance(ctx);
        }
    }

    return create_invalid_token(ctx);
}

static Token extract_number(TokenizerCtx *ctx) {
    size_t start_pos = ctx->pos;
    size_t start_line = ctx->line;
    size_t start_column = ctx->column;

    if (peek(ctx, 0) == '-') {
        advance(ctx);
    }

    if (peek(ctx, 0) == '0') {
        advance(ctx);
        if (peek(ctx, 0) != '.' && isdigit(peek(ctx, 0))) {
            return create_invalid_token(ctx);
        }
    } else {
        if (!isdigit(peek(ctx, 0))) {
            return create_invalid_token(ctx);
        }
        while (isdigit(peek(ctx, 0))) {
            advance(ctx);
        }
    }

    if (peek(ctx, 0) == '.') {
        advance(ctx);
        if (!isdigit(peek(ctx, 0))) {
            return create_invalid_token(ctx);
        }
        while (isdigit(peek(ctx, 0))) {
            advance(ctx);
        }
    }

    if (peek(ctx, 0) == 'e' || peek(ctx, 0) == 'E') {
        advance(ctx);
        if (peek(ctx, 0) == '+' || peek(ctx, 0) == '-') {
            advance(ctx);
        }
        if (!isdigit(peek(ctx, 0))) {
            return create_invalid_token(ctx);
        }
        while (isdigit(peek(ctx, 0))) {
            advance(ctx);
        }
    }

    char next_char = peek(ctx, 0);
    if (next_char != '\0' && !isspace(next_char) && next_char != ',' &&
        next_char != ']' && next_char != '}') {
        return create_invalid_token(ctx);
    }

    return (Token){
        .type = TOKEN_NUMBER,
        .start = ctx->input + start_pos,
        .length = ctx->pos - start_pos,
        .line = start_line,
        .column = start_column,
    };
}

static Token extract_literal(TokenizerCtx *ctx, const char *literal, size_t len,
                             TokenType type) {

    if (strncmp(ctx->input + ctx->pos, literal, len) == 0) {
        for (size_t i = 0; i < len; i++) {
            advance(ctx);
        }

        char next = peek(ctx, 0);
        if (next != '\0' && !isspace(next) && next != ',' && next != ']' &&
            next != '}') {
            return create_invalid_token(ctx);
        }

        return (Token){
            .type = type,
            .start = ctx->input + ctx->pos - len,
            .length = len,
            .line = ctx->line,
            .column = ctx->column - len,
        };
    }

    return create_invalid_token(ctx);
}

Token next_token(TokenizerCtx *ctx) {
    while (is_whitespace(peek(ctx, 0))) {
        advance(ctx);
    }

    char current = peek(ctx, 0);
    switch (current) {
    case '{':
        return create_simple_token(TOKEN_LEFT_BRACE, ctx);
    case '}':
        return create_simple_token(TOKEN_RIGHT_BRACE, ctx);
    case '[':
        return create_simple_token(TOKEN_LEFT_BRACKET, ctx);
    case ']':
        return create_simple_token(TOKEN_RIGHT_BRACKET, ctx);
    case ',':
        return create_simple_token(TOKEN_COMMA, ctx);
    case ':':
        return create_simple_token(TOKEN_COLON, ctx);
    case '\0':
        if (ctx->pos >= ctx->input_length) {
            return (Token){
                .type = TOKEN_EOF,
                .start = ctx->input + ctx->input_length,
                .length = 0,
                .line = ctx->line,
                .column = ctx->column,
            };
        }
        return create_invalid_token(ctx);
    }

    if (current == '"') {
        return extract_string(ctx);
    }

    if (isdigit(current) || current == '-') {
        return extract_number(ctx);
    }

    if (isalpha(current)) {
        if (current == 't' && strncmp(ctx->input + ctx->pos, "true", 4) == 0) {
            return extract_literal(ctx, "true", 4, TOKEN_TRUE);
        }
        if (current == 'f' && strncmp(ctx->input + ctx->pos, "false", 5) == 0) {
            return extract_literal(ctx, "false", 5, TOKEN_FALSE);
        }
        if (current == 'n' && strncmp(ctx->input + ctx->pos, "null", 4) == 0) {
            return extract_literal(ctx, "null", 4, TOKEN_NULL);
        }
    }

    return create_invalid_token(ctx);
}
