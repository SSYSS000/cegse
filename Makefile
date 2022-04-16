CFLAGS  := -Iinclude -Wall -Wextra -g -DDEBUGGING=1
LIBS    := -llz4 -lz
OBJ_DIR := build

SRC_FILES := $(wildcard *.c)
TARGET    := cegse

TEST_SRC_FILES := $(subst main.c,,$(SRC_FILES)) $(wildcard tests/*.c)
TEST_TARGET    := test

OBJS      := $(SRC_FILES:%=$(OBJ_DIR)/%.o)
TEST_OBJS := $(TEST_SRC_FILES:%=$(OBJ_DIR)/%.o)

.PHONY: clean

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LIBS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) -o $(TEST_TARGET) $(TEST_OBJS) $(LIBS) $(CFLAGS)

$(OBJ_DIR)/%.c.o: %.c | $(OBJ_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR):
	mkdir $@ $@/tests

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET)
