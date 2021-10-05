CFLAGS = -Iinclude -Wall -Wextra -g -DDEBUGGING=1
LIBS = -llz4

OBJS = 	snapshot.o \
	save_file.o \
	game_save.o \
	save_stream.o \
	compression.o \

TARGET = cegse

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): main.o $(OBJS)
	$(CC) -o $(TARGET) main.o $(OBJS) $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TARGET)

TEST_FILES=$(wildcard tests/*.c)

test: $(TEST_FILES) $(OBJS)
	$(CC) -o test $(TEST_FILES) $(OBJS) $(LIBS) $(CFLAGS)
