#include "../include/tokenizer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool is_whitespace(const char c) { return isspace((unsigned char)c); }

char peek(const TokenizerCtx *ctx, size_t offset) {
    if (ctx->pos + offset >= ctx->input_length) {
        return '\0';
    }
    return ctx->input[ctx->pos + offset];
}

void advance(TokenizerCtx *ctx) {
    if (ctx->pos >= ctx->input_length) {
        return;
    }

    char current = ctx->input[ctx->pos];
    if (current == '\n' || (current == '\r' && peek(ctx, 1) != '\n')) {
        ctx->line++;
        ctx->column = 0;
    } else {
        ctx->column++;
    }
    ctx->pos++;
    if (current == '\r' && peek(ctx, 1) == '\n') {
        ctx->pos++;
        ctx->column = 0;
    }
}

const char *get_error_message(const ErrorCode error) {
    switch (error) {
    case ERR_NONE:
        return "No error";
    case ERR_UNTERMINATED_STRING:
        return "Unterminated string";
    case ERR_INVALID_ESCAPE_SEQUENCE:
        return "Invalid escape sequence";
    case ERR_INVALID_UNICODE_ESCAPE:
        return "Invalid Unicode escape";
    case ERR_CONTROL_CHARACTER_IN_STRING:
        return "Control character in string";
    case ERR_INVALID_NUMBER_FORMAT:
        return "Invalid number format";
    case ERR_TRAILING_DECIMAL_POINT:
        return "Trailing decimal point in number";
    case ERR_INCOMPLETE_EXPONENT:
        return "Incomplete exponent in number";
    case ERR_UNEXPECTED_CHARACTER:
        return "Unexpected character";
    case ERR_UNTERMINATED_JSON:
        return "Unterminated JSON";
    case ERR_INVALID_LITERAL:
        return "Invalid literal";
    default:
        return "Unknown error";
    }
}

void print_error_context(const Token *token, const char *input,
                         size_t input_length) {
    if (token->error == ERR_NONE || token->error_pos == NULL ||
        token->error_pos < input || token->error_pos >= input + input_length) {
        return;
    }

    const char *line_start = token->error_pos;
    while (line_start > input && *(line_start - 1) != '\n' &&
           *(line_start - 1) != '\r') {
        line_start--;
    }

    const char *line_end = token->error_pos;
    while (line_end < input + input_length && *line_end != '\0' &&
           *line_end != '\n' && *line_end != '\r') {
        line_end++;
    }

    fprintf(stderr, "\033[1;31mError at line %zu, column %zu:\033[0m\n",
            token->line, token->column);
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);
    fprintf(stderr, "%*s\033[1;31m^\033[0m\n",
            (int)(token->error_pos - line_start), "");
    fprintf(stderr, "Message: %s\n", get_error_message(token->error));
}

bool is_valid_utf8_sequence(const char *input, size_t input_size,
                            size_t *length) {
    if (input == NULL || length == NULL) {
        return false;
    }

    if (input_size == 0) {
        return false;
    }

    unsigned char c = (unsigned char)input[0];
    size_t seq_length;

    if (c < 0x80) {
        seq_length = 1;
    } else if ((c & 0xE0) == 0xC0) {
        seq_length = 2;
        if (c < 0xC2) {
            return false;
        }
    } else if ((c & 0xF0) == 0xE0) {
        seq_length = 3;
    } else if ((c & 0xF8) == 0xF0) {
        seq_length = 4;
        if (c > 0xF4) {
            return false;
        }
    } else {
        return false;
    }

    if (input_size < seq_length) {
        return false;
    }

    for (size_t i = 1; i < seq_length; i++) {
        if ((input[i] & 0xC0) != 0x80) {
            return false;
        }
    }

    if (seq_length == 3 && c == 0xE0 && (unsigned char)input[1] < 0xA0) {
        return false;
    }
    if (seq_length == 4 && c == 0xF0 && (unsigned char)input[1] < 0x90) {
        return false;
    }

    if (seq_length == 3 && c == 0xED && (unsigned char)input[1] >= 0xA0) {
        return false;
    }

    *length = seq_length;
    return true;
}

Token create_simple_token(TokenType type, TokenizerCtx *ctx) {
    advance(ctx);
    return (Token){type,      ctx->input + ctx->pos - 1, 1,
                   ctx->line, ctx->column - 1,           ERR_NONE,
                   NULL};
}

Token create_invalid_token(const TokenizerCtx *ctx, ErrorCode error) {
    const char *error_pos = (ctx->pos < ctx->input_length)
                                ? ctx->input + ctx->pos
                                : ctx->input + ctx->input_length;
    return (Token){
        TOKEN_INVALID, NULL, 0, ctx->line, ctx->column, error, error_pos,
    };
}

Token extract_string(TokenizerCtx *ctx) {
    if (ctx == NULL || ctx->input == NULL) {
        return create_invalid_token(ctx, ERR_UNEXPECTED_CHARACTER);
    }

    size_t start_pos = ctx->pos;
    size_t start_line = ctx->line;
    size_t start_column = ctx->column;

    if (peek(ctx, 0) != '"') {
        return create_invalid_token(ctx, ERR_UNEXPECTED_CHARACTER);
    }
    advance(ctx);

    while (peek(ctx, 0) != '\0') {
        char c = peek(ctx, 0);

        if (c == '"') {
            advance(ctx);
            return (Token){
                TOKEN_STRING, ctx->input + start_pos, ctx->pos - start_pos,
                start_line,   start_column,           ERR_NONE,
                NULL};
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
                        return create_invalid_token(ctx,
                                                    ERR_INVALID_UNICODE_ESCAPE);
                    }
                    advance(ctx);
                }
            } else {
                return create_invalid_token(ctx, ERR_INVALID_ESCAPE_SEQUENCE);
            }
        } else if ((unsigned char)c >= 0x80) {
            size_t utf8_length = 0;
            size_t remaining_input_size = ctx->input_length - ctx->pos;
            if (!is_valid_utf8_sequence(ctx->input + ctx->pos,
                                        remaining_input_size, &utf8_length)) {
                return create_invalid_token(ctx,
                                            ERR_CONTROL_CHARACTER_IN_STRING);
            }
            ctx->pos += utf8_length;
            ctx->column += 1;
            // ctx->column += utf8_length;

        } else if (c < 32) {
            return create_invalid_token(ctx, ERR_CONTROL_CHARACTER_IN_STRING);
        } else {
            advance(ctx);
        }
    }

    return create_invalid_token(ctx, ERR_UNTERMINATED_STRING);
}

Token extract_number(TokenizerCtx *ctx) {
    size_t start_pos = ctx->pos;
    size_t start_line = ctx->line;
    size_t start_column = ctx->column;
    bool has_integer_part = false;

    if (peek(ctx, 0) == '-') {
        advance(ctx);
    }

    if (peek(ctx, 0) == '0') {
        advance(ctx);
        if (peek(ctx, 0) != '.' && isdigit(peek(ctx, 0))) {
            return create_invalid_token(ctx, ERR_INVALID_NUMBER_FORMAT);
        }
    } else {
        if (!isdigit(peek(ctx, 0))) {
            return create_invalid_token(ctx, ERR_INVALID_NUMBER_FORMAT);
        }
        has_integer_part = true;
        while (isdigit(peek(ctx, 0))) {
            advance(ctx);
        }
    }

    if (peek(ctx, 0) == '.') {
        advance(ctx);
        if (!isdigit(peek(ctx, 0))) {
            return create_invalid_token(ctx, ERR_TRAILING_DECIMAL_POINT);
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
            return create_invalid_token(ctx, ERR_INCOMPLETE_EXPONENT);
        }
        while (isdigit(peek(ctx, 0))) {
            advance(ctx);
        }
    }

    char next_char = peek(ctx, 0);
    if (next_char != '\0' && !isspace(next_char) && next_char != ',' &&
        next_char != ']' && next_char != '}') {
        return create_invalid_token(ctx, ERR_UNEXPECTED_CHARACTER);
    }

    return (Token){TOKEN_NUMBER, ctx->input + start_pos, ctx->pos - start_pos,
                   start_line,   start_column,           ERR_NONE,
                   NULL};
}

Token extract_literal(TokenizerCtx *ctx, const char *literal, size_t len,
                      TokenType type) {
    if (strncmp(ctx->input + ctx->pos, literal, len) == 0) {
        for (size_t i = 0; i < len; i++) {
            advance(ctx);
        }

        char next = peek(ctx, 0);
        if (next != '\0' && !isspace(next) && next != ',' && next != ']' &&
            next != '}') {
            return create_invalid_token(ctx, ERR_UNEXPECTED_CHARACTER);
        }

        return (Token){type,      ctx->input + ctx->pos - len, len,
                       ctx->line, ctx->column - len,           ERR_NONE,
                       NULL};
    }
    return create_invalid_token(ctx, ERR_UNEXPECTED_CHARACTER);
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
            return (Token){TOKEN_EOF,   ctx->input + ctx->input_length,
                           0,           ctx->line,
                           ctx->column, ERR_NONE,
                           NULL};
        }
        return create_invalid_token(ctx, ERR_UNTERMINATED_JSON);
    }

    if (current == '"') {
        return extract_string(ctx);
    }

    if (isdigit(current) || current == '-') {
        return extract_number(ctx);
    }

    if (isalpha(current)) {
        if (strncmp(ctx->input + ctx->pos, "true", 4) == 0) {
            return extract_literal(ctx, "true", 4, TOKEN_TRUE);
        }
        if (strncmp(ctx->input + ctx->pos, "false", 5) == 0) {
            return extract_literal(ctx, "false", 5, TOKEN_FALSE);
        }
        if (strncmp(ctx->input + ctx->pos, "null", 4) == 0) {
            return extract_literal(ctx, "null", 4, TOKEN_NULL);
        }
    }

    return create_invalid_token(ctx, ERR_UNEXPECTED_CHARACTER);
}
