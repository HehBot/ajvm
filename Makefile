CC := gcc
LD := gcc
TARGET_NAME := ajvm

BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src

TARGET_EXEC := $(BIN_DIR)/$(TARGET_NAME)

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

DEPS := $(OBJS:.o=.d)

INC_FLAGS := -I$(SRC_DIR)

CCFLAGS := -Wall -Wpedantic -Werror $(INC_FLAGS) -g -MMD -MP -DNDEBUG
LDFLAGS :=
LIBFLAGS := -lm

.PHONY: all test test_mem x86-64 clean

all: $(TARGET_EXEC)

$(TARGET_EXEC): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CCFLAGS) -o $@ $<

clean:
	$(RM) -r $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean

-include $(DEPS)
