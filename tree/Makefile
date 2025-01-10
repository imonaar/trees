CC = gcc
CFLAGS = -Wall -Wextra -g

SRCS = tree.c main.c
TARGET = tree

all: $(TARGET)
$(TARGET) : $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)