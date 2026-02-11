#include "../include/tokenizer.h"
#include "./Unity/src/unity.h"
#include "./Unity/src/unity_internals.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void assert_token(JsonToken actual, JsonTokenType expected_type,
                         const char *expected_value, size_t expected_length,
                         size_t expected_line, size_t expected_column) {
    TEST_ASSERT_EQUAL_MESSAGE(expected_type, actual.type,
                              "Token type mismatch");
    if (expected_value != NULL) {
        TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(expected_value, actual.start,
                                             expected_length,
                                             "Token value mismatch");
    }
    TEST_ASSERT_EQUAL_MESSAGE(expected_length, actual.length,
                              "Token length mismatch");
    TEST_ASSERT_EQUAL_MESSAGE(expected_line, actual.line,
                              "Token line mismatch");
    TEST_ASSERT_EQUAL_MESSAGE(expected_column, actual.column,
                              "Token column mismatch");
}

static void
tokenize_and_assert_sequence(const char *input, JsonTokenType expected_types[],
                             const char *expected_values[],
                             size_t expected_lengths[], size_t expected_lines[],
                             size_t expected_columns[], size_t token_count) {
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    for (size_t i = 0; i < token_count; i++) {
        JsonToken token = json_tokenizer_next(&ctx);
        assert_token(token, expected_types[i], expected_values[i],
                     expected_lengths[i], expected_lines[i],
                     expected_columns[i]);
    }

    JsonToken final_token = json_tokenizer_next(&ctx);
    assert_token(final_token, TOKEN_EOF, NULL, 0,
                 expected_lines[token_count - 1],
                 expected_columns[token_count - 1] + 1);
}

void test_tokenizer_initialization_empty_input(void) {
    const char *input = "";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    TEST_ASSERT_EQUAL_PTR(input, ctx.input);
    TEST_ASSERT_EQUAL_size_t(0, ctx.input_length);
    TEST_ASSERT_EQUAL_size_t(0, ctx.pos);
    TEST_ASSERT_EQUAL_size_t(1, ctx.line);
    TEST_ASSERT_EQUAL_size_t(1, ctx.column);
}

void test_tokenizer_initialization_nonempty_input(void) {
    const char *input = "{\"test\":123}";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    TEST_ASSERT_EQUAL_PTR(input, ctx.input);
    TEST_ASSERT_EQUAL_size_t(12, ctx.input_length);
    TEST_ASSERT_EQUAL_size_t(0, ctx.pos);
    TEST_ASSERT_EQUAL_size_t(1, ctx.line);
    TEST_ASSERT_EQUAL_size_t(1, ctx.column);
}

void test_single_brace_tokens(void) {
    const char *input = "{";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_LEFT_BRACE, "{", 1, 1, 1);
}

void test_single_bracket_tokens(void) {
    const char *input = "[";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_LEFT_BRACKET, "[", 1, 1, 1);
}

void test_single_punctuation_tokens(void) {
    struct {
            const char *input;
            JsonTokenType type;
    } test_cases[] = {{"}", TOKEN_RIGHT_BRACE},
                      {"]", TOKEN_RIGHT_BRACKET},
                      {",", TOKEN_COMMA},
                      {":", TOKEN_COLON}};

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        JsonTokenizerCtx ctx = json_tokenizer_init(test_cases[i].input,
                                                   strlen(test_cases[i].input));
        JsonToken token = json_tokenizer_next(&ctx);
        assert_token(token, test_cases[i].type, test_cases[i].input, 1, 1, 1);
    }
}

void test_literal_tokens_true(void) {
    const char *input = "true";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_TRUE, "true", 4, 1, 1);
}

void test_literal_tokens_false(void) {
    const char *input = "false";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_FALSE, "false", 5, 1, 1);
}

void test_literal_tokens_null(void) {
    const char *input = "null";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NULL, "null", 4, 1, 1);
}

void test_simple_string_token(void) {
    const char *input = "\"hello\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"hello\"", 7, 1, 1);
}

void test_empty_string_token(void) {
    const char *input = "\"\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"\"", 2, 1, 1);
}

void test_string_with_spaces(void) {
    const char *input = "\"hello world\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"hello world\"", 13, 1, 1);
}

void test_string_with_escape_quote(void) {
    const char *input = "\"quote:\\\"test\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"quote:\\\"test\"", 14, 1, 1);
}

void test_string_with_backslash_escape(void) {
    const char *input = "\"path\\\\test\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"path\\\\test\"", 12, 1, 1);
}

void test_string_with_forward_slash_escape(void) {
    const char *input = "\"url\\/path\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"url\\/path\"", 11, 1, 1);
}

void test_string_with_backspace_escape(void) {
    const char *input = "\"back\\b\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"back\\b\"", 8, 1, 1);
}

void test_string_with_formfeed_escape(void) {
    const char *input = "\"form\\f\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"form\\f\"", 8, 1, 1);
}

void test_string_with_newline_escape(void) {
    const char *input = "\"line\\n\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"line\\n\"", 8, 1, 1);
}

void test_string_with_carriage_return_escape(void) {
    const char *input = "\"return\\r\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"return\\r\"", 10, 1, 1);
}

void test_string_with_tab_escape(void) {
    const char *input = "\"tab\\t\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"tab\\t\"", 7, 1, 1);
}

void test_string_with_unicode_escape_basic(void) {
    const char *input = "\"\\u0041\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"\\u0041\"", 8, 1, 1);
}

void test_string_with_unicode_escape_lowercase(void) {
    const char *input = "\"\\u00ab\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_STRING, "\"\\u00ab\"", 8, 1, 1);
}

void test_simple_integer_number(void) {
    const char *input = "123";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "123", 3, 1, 1);
}

void test_negative_integer_number(void) {
    const char *input = "-456";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "-456", 4, 1, 1);
}

void test_zero_number(void) {
    const char *input = "0";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "0", 1, 1, 1);
}

void test_decimal_number(void) {
    const char *input = "78.9";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "78.9", 4, 1, 1);
}

void test_negative_decimal_number(void) {
    const char *input = "-12.34";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "-12.34", 6, 1, 1);
}

void test_exponential_number_positive(void) {
    const char *input = "1e3";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "1e3", 3, 1, 1);
}

void test_exponential_number_negative(void) {
    const char *input = "1e-3";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "1e-3", 4, 1, 1);
}

void test_exponential_number_uppercase(void) {
    const char *input = "1E3";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "1E3", 3, 1, 1);
}

void test_exponential_number_with_plus(void) {
    const char *input = "1e+3";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "1e+3", 4, 1, 1);
}

void test_decimal_with_exponential(void) {
    const char *input = "1.2e-4";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_NUMBER, "1.2e-4", 6, 1, 1);
}

void test_whitespace_space_handling(void) {
    const char *input = "   {   }   ";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token1 = json_tokenizer_next(&ctx);
    assert_token(token1, TOKEN_LEFT_BRACE, "{", 1, 1, 4);

    JsonToken token2 = json_tokenizer_next(&ctx);
    assert_token(token2, TOKEN_RIGHT_BRACE, "}", 1, 1, 8);
}

void test_whitespace_tab_handling(void) {
    const char *input = "\t{\t}\t";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token1 = json_tokenizer_next(&ctx);
    assert_token(token1, TOKEN_LEFT_BRACE, "{", 1, 1, 2);

    JsonToken token2 = json_tokenizer_next(&ctx);
    assert_token(token2, TOKEN_RIGHT_BRACE, "}", 1, 1, 4);
}

void test_newline_line_tracking_lf(void) {
    const char *input = "{\n}";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token1 = json_tokenizer_next(&ctx);
    assert_token(token1, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    JsonToken token2 = json_tokenizer_next(&ctx);
    assert_token(token2, TOKEN_RIGHT_BRACE, "}", 1, 2, 1);
}

void test_newline_line_tracking_crlf(void) {
    const char *input = "{\r\n}";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token1 = json_tokenizer_next(&ctx);
    assert_token(token1, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    JsonToken token2 = json_tokenizer_next(&ctx);
    assert_token(token2, TOKEN_RIGHT_BRACE, "}", 1, 2, 1);
}

void test_newline_line_tracking_cr_only(void) {
    const char *input = "{\r}";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token1 = json_tokenizer_next(&ctx);
    assert_token(token1, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    JsonToken token2 = json_tokenizer_next(&ctx);
    assert_token(token2, TOKEN_RIGHT_BRACE, "}", 1, 2, 1);
}

void test_multiple_lines_column_tracking(void) {
    const char *input = "  {\n    \"key\":\n    123\n  }";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token1 = json_tokenizer_next(&ctx);
    assert_token(token1, TOKEN_LEFT_BRACE, "{", 1, 1, 3);

    JsonToken token2 = json_tokenizer_next(&ctx);
    assert_token(token2, TOKEN_STRING, "\"key\"", 5, 2, 5);

    JsonToken token3 = json_tokenizer_next(&ctx);
    assert_token(token3, TOKEN_COLON, ":", 1, 2, 10);

    JsonToken token4 = json_tokenizer_next(&ctx);
    assert_token(token4, TOKEN_NUMBER, "123", 3, 3, 5);

    JsonToken token5 = json_tokenizer_next(&ctx);
    assert_token(token5, TOKEN_RIGHT_BRACE, "}", 1, 4, 3);
}

void test_sequence_simple_tokens(void) {
    const char *input = "{ } [ ] , :";

    JsonTokenType expected_types[] = {TOKEN_LEFT_BRACE,   TOKEN_RIGHT_BRACE,
                                      TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
                                      TOKEN_COMMA,        TOKEN_COLON};
    const char *expected_values[] = {"{", "}", "[", "]", ",", ":"};
    size_t expected_lengths[] = {1, 1, 1, 1, 1, 1};
    size_t expected_lines[] = {1, 1, 1, 1, 1, 1};
    size_t expected_columns[] = {1, 3, 5, 7, 9, 11};

    tokenize_and_assert_sequence(input, expected_types, expected_values,
                                 expected_lengths, expected_lines,
                                 expected_columns, 6);
}

void test_sequence_literals(void) {
    const char *input = "true";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_TRUE, "true", 4, 1, 1);
}

void test_complex_json_sequence(void) {
    const char *input = "{\"key\":[123,\"value\",true,null]}";

    JsonTokenType expected_types[] = {
        TOKEN_LEFT_BRACE, TOKEN_STRING, TOKEN_COLON,  TOKEN_LEFT_BRACKET,
        TOKEN_NUMBER,     TOKEN_COMMA,  TOKEN_STRING, TOKEN_COMMA,
        TOKEN_TRUE,       TOKEN_COMMA,  TOKEN_NULL,   TOKEN_RIGHT_BRACKET,
        TOKEN_RIGHT_BRACE};
    const char *expected_values[] = {"{",    "\"key\"",   ":", "[",    "123",
                                     ",",    "\"value\"", ",", "true", ",",
                                     "null", "]",         "}"};
    size_t expected_lengths[] = {1, 5, 1, 1, 3, 1, 7, 1, 4, 1, 4, 1, 1};
    size_t expected_lines[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    size_t expected_columns[] = {1, 2, 7, 8, 9, 12, 13, 20, 21, 25, 26, 30, 31};

    tokenize_and_assert_sequence(input, expected_types, expected_values,
                                 expected_lengths, expected_lines,
                                 expected_columns, 13);
}

void test_invalid_unterminated_string(void) {
    const char *input = "\"unterminated";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 14);
}

void test_invalid_bad_escape_sequence(void) {
    const char *input = "\"bad escape \\x\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 14);
}

void test_invalid_incomplete_unicode_escape(void) {
    const char *input = "\"bad \\u12\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 10);
}

void test_invalid_control_character_in_string(void) {
    const char *input = "\"control \x01 char\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 10);
}

void test_invalid_unicode_escape_non_hex(void) {
    const char *input = "\"bad \\uZZZZ\"";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 8);
}

void test_invalid_leading_zeros_in_number(void) {
    const char *input = "007";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 2);
}

void test_invalid_incomplete_exponent(void) {
    const char *input = "1e+";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 4);
}

void test_invalid_leading_decimal(void) {
    const char *input = ".123";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 1);
}

void test_invalid_trailing_decimal(void) {
    const char *input = "1.";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 3);
}

void test_invalid_unexpected_characters(void) {
    const char *invalid_chars[] = {";", "&", "=", "@", "!"};

    for (size_t i = 0; i < sizeof(invalid_chars) / sizeof(invalid_chars[0]);
         i++) {
        JsonTokenizerCtx ctx =
            json_tokenizer_init(invalid_chars[i], strlen(invalid_chars[i]));
        JsonToken token = json_tokenizer_next(&ctx);
        assert_token(token, TOKEN_INVALID, NULL, 0, 1, 1);
    }
}

void test_invalid_incomplete_literal(void) {
    const char *input = "tru";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 1);
}

void test_invalid_literal_with_trailing_chars(void) {
    const char *input = "trueX";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_INVALID, NULL, 0, 1, 5);
}

void test_eof_empty_input(void) {
    const char *input = "";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 1, 1);
}

void test_eof_after_valid_tokens(void) {
    const char *input = "{}";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token1 = json_tokenizer_next(&ctx);
    assert_token(token1, TOKEN_LEFT_BRACE, "{", 1, 1, 1);

    JsonToken token2 = json_tokenizer_next(&ctx);
    assert_token(token2, TOKEN_RIGHT_BRACE, "}", 1, 1, 2);

    JsonToken token3 = json_tokenizer_next(&ctx);
    assert_token(token3, TOKEN_EOF, NULL, 0, 1, 3);
}

void test_whitespace_only_input(void) {
    const char *input = "   \t\n\r   ";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    JsonToken token = json_tokenizer_next(&ctx);
    assert_token(token, TOKEN_EOF, NULL, 0, 3, 4);
}

void test_nested_structure_position_tracking(void) {
    const char *input = "{\"a\":{\"b\":[{\"c\":123}]}}";
    JsonTokenizerCtx ctx = json_tokenizer_init(input, strlen(input));

    struct {
            JsonTokenType type;
            const char *value;
            size_t length;
            size_t line;
            size_t column;
    } expected[] = {{TOKEN_LEFT_BRACE, "{", 1, 1, 1},
                    {TOKEN_STRING, "\"a\"", 3, 1, 2},
                    {TOKEN_COLON, ":", 1, 1, 5},
                    {TOKEN_LEFT_BRACE, "{", 1, 1, 6},
                    {TOKEN_STRING, "\"b\"", 3, 1, 7},
                    {TOKEN_COLON, ":", 1, 1, 10},
                    {TOKEN_LEFT_BRACKET, "[", 1, 1, 11},
                    {TOKEN_LEFT_BRACE, "{", 1, 1, 12},
                    {TOKEN_STRING, "\"c\"", 3, 1, 13},
                    {TOKEN_COLON, ":", 1, 1, 16},
                    {TOKEN_NUMBER, "123", 3, 1, 17},
                    {TOKEN_RIGHT_BRACE, "}", 1, 1, 20},
                    {TOKEN_RIGHT_BRACKET, "]", 1, 1, 21},
                    {TOKEN_RIGHT_BRACE, "}", 1, 1, 22},
                    {TOKEN_RIGHT_BRACE, "}", 1, 1, 23}};

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        JsonToken token = json_tokenizer_next(&ctx);
        assert_token(token, expected[i].type, expected[i].value,
                     expected[i].length, expected[i].line, expected[i].column);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_tokenizer_initialization_empty_input);
    RUN_TEST(test_tokenizer_initialization_nonempty_input);

    RUN_TEST(test_single_brace_tokens);
    RUN_TEST(test_single_bracket_tokens);
    RUN_TEST(test_single_punctuation_tokens);

    RUN_TEST(test_literal_tokens_true);
    RUN_TEST(test_literal_tokens_false);
    RUN_TEST(test_literal_tokens_null);

    RUN_TEST(test_simple_string_token);
    RUN_TEST(test_empty_string_token);
    RUN_TEST(test_string_with_spaces);
    RUN_TEST(test_string_with_escape_quote);
    RUN_TEST(test_string_with_backslash_escape);
    RUN_TEST(test_string_with_forward_slash_escape);
    RUN_TEST(test_string_with_backspace_escape);
    RUN_TEST(test_string_with_formfeed_escape);
    RUN_TEST(test_string_with_newline_escape);
    RUN_TEST(test_string_with_carriage_return_escape);
    RUN_TEST(test_string_with_tab_escape);
    RUN_TEST(test_string_with_unicode_escape_basic);
    RUN_TEST(test_string_with_unicode_escape_lowercase);

    RUN_TEST(test_simple_integer_number);
    RUN_TEST(test_negative_integer_number);
    RUN_TEST(test_zero_number);
    RUN_TEST(test_decimal_number);
    RUN_TEST(test_negative_decimal_number);
    RUN_TEST(test_exponential_number_positive);
    RUN_TEST(test_exponential_number_negative);
    RUN_TEST(test_exponential_number_uppercase);
    RUN_TEST(test_exponential_number_with_plus);
    RUN_TEST(test_decimal_with_exponential);

    RUN_TEST(test_whitespace_space_handling);
    RUN_TEST(test_whitespace_tab_handling);
    RUN_TEST(test_newline_line_tracking_lf);
    RUN_TEST(test_newline_line_tracking_crlf);
    RUN_TEST(test_newline_line_tracking_cr_only);
    RUN_TEST(test_multiple_lines_column_tracking);

    RUN_TEST(test_sequence_simple_tokens);
    RUN_TEST(test_sequence_literals);
    RUN_TEST(test_complex_json_sequence);

    RUN_TEST(test_invalid_unterminated_string);
    RUN_TEST(test_invalid_bad_escape_sequence);
    RUN_TEST(test_invalid_incomplete_unicode_escape);
    RUN_TEST(test_invalid_control_character_in_string);
    RUN_TEST(test_invalid_unicode_escape_non_hex);
    RUN_TEST(test_invalid_leading_zeros_in_number);
    RUN_TEST(test_invalid_incomplete_exponent);
    RUN_TEST(test_invalid_leading_decimal);
    RUN_TEST(test_invalid_trailing_decimal);
    RUN_TEST(test_invalid_unexpected_characters);
    RUN_TEST(test_invalid_incomplete_literal);
    RUN_TEST(test_invalid_literal_with_trailing_chars);

    RUN_TEST(test_eof_empty_input);
    RUN_TEST(test_eof_after_valid_tokens);
    RUN_TEST(test_whitespace_only_input);

    RUN_TEST(test_nested_structure_position_tracking);

    return UNITY_END();
}
