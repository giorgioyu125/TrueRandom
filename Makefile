CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = 
TARGET = test
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

arm:
	aarch64-linux-gnu-gcc -march=armv8-a+rng -static test.c -o test_arm64
	qemu-aarch64-static ./test_arm64

clean:
	rm -f $(OBJS) $(TARGET) test_arm64

.PHONY: all arm clean
