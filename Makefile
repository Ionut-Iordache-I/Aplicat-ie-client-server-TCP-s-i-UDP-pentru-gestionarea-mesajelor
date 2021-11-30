# Iordache Ionut-Iulian 323CB
# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -g

all: server subscriber

# server.c
server: server.c -lm

# subscriber.c
subscriber: subscriber.c -lm

.PHONY: clean

clean:
	rm -f server subscriber