CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -g -DDEBUGGING=1
LIBS = -llz4

OBJS = 	main.o \
	snapshot.o \
	save_file.o \
	game_save.o \
	save_stream.o \

TARGET = cegse

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TARGET)
