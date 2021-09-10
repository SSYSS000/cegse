CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -g
LIBS =

OBJS = 	main.o \
	file_io.o

TARGET = cegse

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TARGET)
