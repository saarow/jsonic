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

TEST_RUNNER = $(BUILD_DIR)/test_runner

all: test

test: $(TEST_RUNNER)
	@echo "Running tests..."
	./$(TEST_RUNNER)

$(TEST_RUNNER): $(OBJS) $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(UNITY_DIR)/unity.c

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test clean
