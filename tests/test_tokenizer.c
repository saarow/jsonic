#include "../include/tokenizer.h"
#include "./Unity/src/unity.h"
#include "./Unity/src/unity_internals.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

TokenizerCtx create_ctx(const char *input, size_t input_length) {
    return (TokenizerCtx){input, input_length, 0, 1, 1};
}

void assert_token(Token actual, TokenType expected_type,
                  const char *expected_value, size_t expected_length,
                  size_t expected_line, size_t expected_column,
                  ErrorCode expected_error) {
    TEST_ASSERT_EQUAL(expected_type, actual.type);
    if (expected_value != NULL) {
        TEST_ASSERT_EQUAL_STRING_LEN(expected_value, actual.start,
                                     expected_length);
    }
    TEST_ASSERT_EQUAL(expected_length, actual.length);
    TEST_ASSERT_EQUAL(expected_line, actual.line);
    TEST_ASSERT_EQUAL(expected_column, actual.column);
    TEST_ASSERT_EQUAL(expected_error, actual.error);
}

void test_simple_tokens(void) {
    const char *input = "{ } [ ] , : true false null";
    TokenizerCtx ctx = create_ctx(input, strlen(input));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 3, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACKET, "[", 1, 1, 5, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACKET, "]", 1, 1, 7, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COMMA, ",", 1, 1, 9, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 11, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_TRUE, "true", 4, 1, 13, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_FALSE, "false", 5, 1, 18, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NULL, "null", 4, 1, 24, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 28, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 28, ERR_NONE);
}

void test_unexpected_character(void) {
    const char *inputs[] = {";", "&", "=", "@", "!"};

    for (int i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        TokenizerCtx ctx = create_ctx(inputs[i], strlen(inputs[i]));
        Token token = next_token(&ctx);
        assert_token(token, TOKEN_INVALID, NULL, 0, 1, 1,
                     ERR_UNEXPECTED_CHARACTER);
    }
}

void test_string_tokens(void) {
    const char *input =
        "\"Hello\" \"escaped\\n\" \"unicode\\u0041\" \"invalid\\u00X\"";
    TokenizerCtx ctx = create_ctx(input, strlen(input));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"Hello\"", 7, 1, 1, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"escaped\\n\"", 11, 1, 9, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"unicode\\u0041\"", 15, 1, 21,
                 ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 49,
                 ERR_INVALID_UNICODE_ESCAPE);
}

void test_number_tokens(void) {
    const char *input = "123 -456 78.9 1e3 1.2e-4 0123";
    TokenizerCtx ctx = create_ctx(input, strlen(input));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "123", 3, 1, 1, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "-456", 4, 1, 5, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "78.9", 4, 1, 10, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "1e3", 3, 1, 15, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "1.2e-4", 6, 1, 19, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 27,
                 ERR_INVALID_NUMBER_FORMAT);
}

void test_invalid_numbers(void) {
    const char *input = "007";
    TokenizerCtx ctx = create_ctx(input, strlen(input));
    Token token = next_token(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 2,
                 ERR_INVALID_NUMBER_FORMAT);

    const char *input2 = "1e+";
    TokenizerCtx ctx2 = create_ctx(input2, strlen(input2));
    Token token2 = next_token(&ctx2);
    assert_token(token2, TOKEN_INVALID, NULL, 0, 1, 4, ERR_INCOMPLETE_EXPONENT);

    const char *input3 = ".123";
    TokenizerCtx ctx3 = create_ctx(input3, strlen(input3));
    Token token3 = next_token(&ctx3);
    assert_token(token3, TOKEN_INVALID, NULL, 0, 1, 1,
                 ERR_UNEXPECTED_CHARACTER);
}

void test_nested_json(void) {
    const char *json = "{\"a\":{\"b\":{\"c\":123}}}";
    TokenizerCtx ctx = create_ctx(json, strlen(json));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"a\"", 3, 1, 2, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 5, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 6, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"b\"", 3, 1, 7, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 10, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 11, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"c\"", 3, 1, 12, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 15, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "123", 3, 1, 16, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 19, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 20, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 21, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 22, ERR_NONE);
}

void test_utf8_characters(void) {
    const char *json =
        "{\"utf8-emoji\": \"😊\", \"utf8-japanese\": \"こんにちは\"}";
    TokenizerCtx ctx = create_ctx(json, strlen(json));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"utf8-emoji\"", 12, 1, 2, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 14, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"😊\"", 6, 1, 16, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COMMA, ",", 1, 1, 22, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"utf8-japanese\"", 15, 1, 24, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 39, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"こんにちは\"", 17, 1, 41, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 58, ERR_NONE);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 59, ERR_NONE);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_simple_tokens);
    RUN_TEST(test_unexpected_character);
    RUN_TEST(test_string_tokens);
    RUN_TEST(test_number_tokens);
    RUN_TEST(test_invalid_numbers);
    RUN_TEST(test_nested_json);
    RUN_TEST(test_utf8_characters);

    return UNITY_END();
}
