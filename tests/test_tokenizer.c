#include "../include/tokenizer.h"
#include "./Unity/src/unity.h"
#include "./Unity/src/unity_internals.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void assert_token(JsonToken actual, JsonTokenType expected_type,
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
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 3);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_LEFT_BRACKET, "[", 1, 1, 5);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACKET, "]", 1, 1, 7);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_COMMA, ",", 1, 1, 9);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 11);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_TRUE, "true", 4, 1, 13);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_FALSE, "false", 5, 1, 18);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NULL, "null", 4, 1, 24);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 28);
}

void test_string_tokens(void) {
    const char *input = "\"Hello\" \"escaped\\n\" \"unicode\\u0041\" "
                        "\"quote:\\\"balance is the key\\\"\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"Hello\"", 7, 1, 1);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"escaped\\n\"", 11, 1, 9);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"unicode\\u0041\"", 15, 1, 21);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"quote:\\\"balance is the key\\\"\"",
                 30, 1, 37);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 67);
}

void test_invalid_strings(void) {
    const char *inputs[] = {"\"unterminated", "\"bad escape \\x\"",
                            "\"bad \\u12\"", "\"control \x01 char\"",
                            "\"bad \\uZZZZ\""};

    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        JsonTokenizerCtx ctx =
            json_tokenizer_init(inputs[i], strlen(inputs[i]));
        JsonToken token = json_tokenizer_next(&ctx);
        assert_token(
            token, TOKEN_INVALID, NULL, 0, 1,
            i == 0 ? 14 : (i == 1 ? 14 : (i == 2 ? 10 : (i == 3 ? 10 : 8))));
    }
}

void test_number_tokens(void) {
    const char *input = "123 -456 78.9 1e3 1.2e-4";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "123", 3, 1, 1);
    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "-456", 4, 1, 5);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "78.9", 4, 1, 10);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "1e3", 3, 1, 15);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "1.2e-4", 6, 1, 19);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 25);
}

void test_invalid_numbers(void) {
    const char *input = "007";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));
    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 2);

    const char *input2 = "1e+";
    JsonTokenizerCtx ctx2 = json_tokenizer_init(input2, strlen(input2));
    JsonToken token2 = json_tokenizer_next(&ctx2);
    assert_token(token2, TOKEN_INVALID, NULL, 0, 1, 4);

    const char *input3 = ".123";
    JsonTokenizerCtx ctx3 = json_tokenizer_init(input3, strlen(input3));
    JsonToken token3 = json_tokenizer_next(&ctx3);
    assert_token(token3, TOKEN_INVALID, NULL, 0, 1, 1);

    const char *input4 = "1.";
    JsonTokenizerCtx ctx4 = json_tokenizer_init(input4, strlen(input4));
    JsonToken token4 = json_tokenizer_next(&ctx4);
    assert_token(token4, TOKEN_INVALID, NULL, 0, 1, 3);
}

void test_nested_json(void) {
    const char *json = "{\"a\":{\"b\":{\"c\":123}}}";
    JsonTokenizerCtx ctx = json_tokenizer_init(json, strlen(json));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"a\"", 3, 1, 2);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 5);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 6);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"b\"", 3, 1, 7);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 10);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 11);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"c\"", 3, 1, 12);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_COLON, ":", 1, 1, 15);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "123", 3, 1, 16);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 19);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 20);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_RIGHT_BRACE, "}", 1, 1, 21);

    token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 22);
}

void test_unexpected_character(void) {
    const char *inputs[] = {";", "&", "=", "@", "!"};

    for (int i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        JsonTokenizerCtx ctx =
            json_tokenizer_init(inputs[i], strlen(inputs[i]));
        JsonToken token = json_tokenizer_next(&ctx);
        assert_token(token, TOKEN_INVALID, NULL, 0, 1, 1);
    }
}

void test_mixed_content(void) {
    const char *input = "{\"key\": [123, \"value\", true, null]}";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken tokens[] = {{TOKEN_LEFT_BRACE, input + 0, 1, 1, 1},
                          {TOKEN_STRING, input + 1, 5, 1, 2},
                          {TOKEN_COLON, input + 6, 1, 1, 7},
                          {TOKEN_LEFT_BRACKET, input + 8, 1, 1, 9},
                          {TOKEN_NUMBER, input + 9, 3, 1, 10},
                          {TOKEN_COMMA, input + 12, 1, 1, 13},
                          {TOKEN_STRING, input + 14, 7, 1, 15},
                          {TOKEN_COMMA, input + 21, 1, 1, 22},
                          {TOKEN_TRUE, input + 23, 4, 1, 24},
                          {TOKEN_COMMA, input + 27, 1, 1, 28},
                          {TOKEN_NULL, input + 29, 4, 1, 30},
                          {TOKEN_RIGHT_BRACKET, input + 33, 1, 1, 34},
                          {TOKEN_RIGHT_BRACE, input + 34, 1, 1, 35}};

    for (size_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
        JsonToken token = json_tokenizer_next(&ctx);
        assert_token(token, tokens[i].type, tokens[i].start, tokens[i].length,
                     tokens[i].line, tokens[i].column);
    }
}

void test_empty_input(void) {
    const char *input = "";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));
    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_simple_tokens);
    RUN_TEST(test_string_tokens);
    RUN_TEST(test_invalid_strings);
    RUN_TEST(test_number_tokens);
    RUN_TEST(test_invalid_numbers);
    RUN_TEST(test_nested_json);
    RUN_TEST(test_unexpected_character);
    RUN_TEST(test_empty_input);
    RUN_TEST(test_mixed_content);

    return UNITY_END();
}
