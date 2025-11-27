CC=gcc
CFLAGS=-Wall -Wextra -O2 -g -pthread -std=c99 -D_POSIX_C_SOURCE=200112L
LDFLAGS=-pthread

# List your source files
SRCS = main.c house.c helpers.c hunter.c ghost.c evidence.c room.c
OBJS = $(SRCS:.c=.o)

TARGET = ghost_hunter_sim

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c defs.h helpers.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.csv

