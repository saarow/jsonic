#include "../include/tokenizer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static bool is_whitespace(const char c);
static char peek(const JsonTokenizerCtx *ctx, size_t offset);
static void advance(JsonTokenizerCtx *ctx);
static JsonToken simple_json_token(JsonTokenType type, JsonTokenizerCtx *ctx);
static JsonToken invalid_json_token(JsonTokenizerCtx *ctx);
static JsonToken extract_string_token(JsonTokenizerCtx *ctx);
static JsonToken extract_number_token(JsonTokenizerCtx *ctx);
static JsonToken extract_literal_token(JsonTokenizerCtx *ctx,
                                       const char *literal, size_t len,
                                       JsonTokenType type);

JsonTokenizerCtx json_tokenizer_init(const char *json_input, size_t length) {
    return (JsonTokenizerCtx){
        .input = json_input,
        .input_length = length,
        .pos = 0,
        .line = 1,
        .column = 1,
    };
}

JsonToken json_tokenizer_next(JsonTokenizerCtx *ctx) {
    while (is_whitespace(peek(ctx, 0))) {
        advance(ctx);
    }

    char current = peek(ctx, 0);
    switch (current) {
    case '{':
        return simple_json_token(TOKEN_LEFT_BRACE, ctx);
    case '}':
        return simple_json_token(TOKEN_RIGHT_BRACE, ctx);
    case '[':
        return simple_json_token(TOKEN_LEFT_BRACKET, ctx);
    case ']':
        return simple_json_token(TOKEN_RIGHT_BRACKET, ctx);
    case ',':
        return simple_json_token(TOKEN_COMMA, ctx);
    case ':':
        return simple_json_token(TOKEN_COLON, ctx);
    case '\0':
        if (ctx->pos >= ctx->input_length) {
            return (JsonToken){
                .type = TOKEN_EOF,
                .start = ctx->input + ctx->input_length,
                .length = 0,
                .line = ctx->line,
                .column = ctx->column,
            };
        }
        return invalid_json_token(ctx);
    }

    if (current == '"') {
        return extract_string_token(ctx);
    }

    if (isdigit(current) || current == '-') {
        return extract_number_token(ctx);
    }

    if (isalpha(current)) {
        if (current == 't' && strncmp(ctx->input + ctx->pos, "true", 4) == 0) {
            return extract_literal_token(ctx, "true", 4, TOKEN_TRUE);
        }
        if (current == 'f' && strncmp(ctx->input + ctx->pos, "false", 5) == 0) {
            return extract_literal_token(ctx, "false", 5, TOKEN_FALSE);
        }
        if (current == 'n' && strncmp(ctx->input + ctx->pos, "null", 4) == 0) {
            return extract_literal_token(ctx, "null", 4, TOKEN_NULL);
        }
    }

    return invalid_json_token(ctx);
}

static bool is_whitespace(const char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static char peek(const JsonTokenizerCtx *ctx, size_t offset) {
    if (ctx->pos + offset >= ctx->input_length) {
        return '\0';
    }
    return ctx->input[ctx->pos + offset];
}

static void advance(JsonTokenizerCtx *ctx) {
    if (ctx->pos >= ctx->input_length) {
        return;
    }

    char c = ctx->input[ctx->pos];
    if (c == '\n') {
        ctx->line++;
        ctx->column = 1;
    } else if (c == '\r') {
        if (ctx->pos + 1 < ctx->input_length &&
            ctx->input[ctx->pos + 1] == '\n') {
            ctx->pos++;
        }
        ctx->line++;
        ctx->column = 1;
    } else {
        ctx->column++;
    }

    ctx->pos++;
}

static JsonToken simple_json_token(JsonTokenType type, JsonTokenizerCtx *ctx) {
    JsonToken token = {
        .type = type,
        .start = ctx->input + ctx->pos,
        .length = 1,
        .line = ctx->line,
        .column = ctx->column,
    };

    advance(ctx);
    return token;
}

static JsonToken invalid_json_token(JsonTokenizerCtx *ctx) {
    JsonToken token = {
        .type = TOKEN_INVALID,
        .start = NULL,
        .length = 0,
        .line = ctx->line,
        .column = ctx->column,
    };

    return token;
}

static JsonToken extract_string_token(JsonTokenizerCtx *ctx) {
    size_t start_pos = ctx->pos;
    size_t start_line = ctx->line;
    size_t start_column = ctx->column;

    if (peek(ctx, 0) != '"') {
        return invalid_json_token(ctx);
    }
    advance(ctx);

    while (peek(ctx, 0) != '\0') {
        char c = peek(ctx, 0);

        if (c < 0x20 && c != '\t') {
            return invalid_json_token(ctx);
        }

        if (c == '"') {
            advance(ctx);
            return (JsonToken){
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
                        return invalid_json_token(ctx);
                    }
                    advance(ctx);
                }
                // TODO: Validate UTF-8
            } else {
                return invalid_json_token(ctx);
            }
        } else {
            advance(ctx);
        }
    }

    return invalid_json_token(ctx);
}

static JsonToken extract_number_token(JsonTokenizerCtx *ctx) {
    size_t start_pos = ctx->pos;
    size_t start_line = ctx->line;
    size_t start_column = ctx->column;

    if (peek(ctx, 0) == '-') {
        advance(ctx);
    }

    if (peek(ctx, 0) == '0') {
        advance(ctx);
        if (isdigit(peek(ctx, 0))) {
            return invalid_json_token(ctx);
        }
    } else {
        if (!isdigit(peek(ctx, 0))) {
            return invalid_json_token(ctx);
        }
        while (isdigit(peek(ctx, 0))) {
            advance(ctx);
        }
    }

    if (peek(ctx, 0) == '.') {
        advance(ctx);
        if (!isdigit(peek(ctx, 0))) {
            return invalid_json_token(ctx);
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
            return invalid_json_token(ctx);
        }
        while (isdigit(peek(ctx, 0))) {
            advance(ctx);
        }
    }

    char next = peek(ctx, 0);
    if (next != '\0' && !isspace(next) && next != ',' && next != ']' &&
        next != '}') {
        return invalid_json_token(ctx);
    }

    return (JsonToken){
        .type = TOKEN_NUMBER,
        .start = ctx->input + start_pos,
        .length = ctx->pos - start_pos,
        .line = start_line,
        .column = start_column,
    };
}

static JsonToken extract_literal_token(JsonTokenizerCtx *ctx,
                                       const char *literal, size_t len,
                                       JsonTokenType type) {

    if (strncmp(ctx->input + ctx->pos, literal, len) == 0) {
        for (size_t i = 0; i < len; i++) {
            advance(ctx);
        }

        char next = peek(ctx, 0);
        if (next != '\0' && !isspace(next) && next != ',' && next != ']' &&
            next != '}') {
            return invalid_json_token(ctx);
        }

        return (JsonToken){
            .type = type,
            .start = ctx->input + ctx->pos - len,
            .length = len,
            .line = ctx->line,
            .column = ctx->column - len,
        };
    }

    return invalid_json_token(ctx);
}
