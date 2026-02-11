CC = gcc
CFLAGS = 
#-Wall -Wextra -std=c99 -Iinclude -g

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
UNITY_DIR = $(TEST_DIR)/Unity/src

SRCS = $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/src/%.o, $(SRCS))
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/tests/%.o, $(TEST_SRCS))

TOKENIZER_TEST_RUNNER = $(BUILD_DIR)/test_tokenizer_runner
PARSER_TEST_RUNNER = $(BUILD_DIR)/test_parser_runner

all: test

test: test_tokenizer test_parser

test_tokenizer: $(TOKENIZER_TEST_RUNNER)
	@echo "Running tokenizer tests..."
	@./$(TOKENIZER_TEST_RUNNER)
	@echo "-----------------------"

test_parser: $(PARSER_TEST_RUNNER)
	@echo "Running parser tests..."
	@./$(PARSER_TEST_RUNNER)
	@echo "-----------------------"

$(TOKENIZER_TEST_RUNNER): $(OBJS) $(BUILD_DIR)/tests/test_tokenizer.o
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ $^ $(UNITY_DIR)/unity.c

$(PARSER_TEST_RUNNER): $(OBJS) $(BUILD_DIR)/tests/test_parser.o
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ $^ $(UNITY_DIR)/unity.c

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test clean
