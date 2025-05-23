#include "../include/tokenizer.h"
#include "./Unity/src/unity.h"
#include "./Unity/src/unity_internals.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

TokenizerCtx create_ctx(const char *input, size_t input_length) {
    return (TokenizerCtx){
        .input = input,
        .input_length = input_length,
        .pos = 0,
        .line = 1,
        .column = 1,
    };
}

void assert_token(Token actual, TokenType expected_type,
                  const char *expected_value, size_t expected_length,
                  size_t expected_line, size_t expected_column) {
    TEST_ASSERT_EQUAL(expected_type, actual.type);
    if (expected_value != NULL) {
        TEST_ASSERT_EQUAL_STRING_LEN(expected_value, actual.start,
                                     expected_length);
    }
    TEST_ASSERT_EQUAL(expected_length, actual.length);
    TEST_ASSERT_EQUAL(expected_line, actual.line);
    TEST_ASSERT_EQUAL(expected_column, actual.column);
}

void test_simple_tokens(void) {
    const char *input = "{ } [ ] , : true false null";
    TokenizerCtx ctx = create_ctx(input, strlen(input));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 3);

    token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACKET, "[", 1, 1, 5);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACKET, "]", 1, 1, 7);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COMMA, ",", 1, 1, 9);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 11);

    token = next_token(&ctx);
    assert_token(token, TOKEN_TRUE, "true", 4, 1, 13);

    token = next_token(&ctx);
    assert_token(token, TOKEN_FALSE, "false", 5, 1, 18);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NULL, "null", 4, 1, 24);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 28);
}

void test_string_tokens(void) {
    const char *input = "\"Hello\" \"escaped\\n\" \"unicode\\u0041\" "
                        "\"quote:\\\"balance is the key\\\"\"";
    TokenizerCtx ctx = create_ctx(input, strlen(input));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"Hello\"", 7, 1, 1);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"escaped\\n\"", 11, 1, 9);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"unicode\\u0041\"", 15, 1, 21);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"quote:\\\"balance is the key\\\"\"",
                 30, 1, 37);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 67);
}

void test_number_tokens(void) {
    const char *input = "123 -456 78.9 1e3 1.2e-4";
    TokenizerCtx ctx = create_ctx(input, strlen(input));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "123", 3, 1, 1);
    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "-456", 4, 1, 5);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "78.9", 4, 1, 10);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "1e3", 3, 1, 15);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "1.2e-4", 6, 1, 19);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 25);
}

void test_nested_json(void) {
    const char *json = "{\"a\":{\"b\":{\"c\":123}}}";
    TokenizerCtx ctx = create_ctx(json, strlen(json));

    Token token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"a\"", 3, 1, 2);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 5);

    token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 6);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"b\"", 3, 1, 7);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 10);

    token = next_token(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 11);

    token = next_token(&ctx);
    assert_token(token, TOKEN_STRING, "\"c\"", 3, 1, 12);

    token = next_token(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 15);

    token = next_token(&ctx);
    assert_token(token, TOKEN_NUMBER, "123", 3, 1, 16);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 19);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 20);

    token = next_token(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 21);

    token = next_token(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 22);
}

void test_unexpected_character(void) {
    const char *inputs[] = {";", "&", "=", "@", "!"};

    for (int i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        TokenizerCtx ctx = create_ctx(inputs[i], strlen(inputs[i]));
        Token token = next_token(&ctx);
        assert_token(token, TOKEN_INVALID, NULL, 0, 1, 1);
    }
}

void test_invalid_numbers(void) {
    const char *input = "007";
    TokenizerCtx ctx = create_ctx(input, strlen(input));
    Token token = next_token(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 2);

    const char *input2 = "1e+";
    TokenizerCtx ctx2 = create_ctx(input2, strlen(input2));
    Token token2 = next_token(&ctx2);
    assert_token(token2, TOKEN_INVALID, NULL, 0, 1, 4);

    const char *input3 = ".123";
    TokenizerCtx ctx3 = create_ctx(input3, strlen(input3));
    Token token3 = next_token(&ctx3);
    assert_token(token3, TOKEN_INVALID, NULL, 0, 1, 1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_simple_tokens);
    RUN_TEST(test_string_tokens);
    RUN_TEST(test_number_tokens);
    RUN_TEST(test_nested_json);
    RUN_TEST(test_unexpected_character);
    RUN_TEST(test_invalid_numbers);

    return UNITY_END();
}
