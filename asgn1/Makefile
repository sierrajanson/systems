# Author: Sierra Janson
#

CC = clang
CFLAGS = -gdwarf -pedantic -Wall -Wextra -Werror

all: memory

memory: memory.o
	$(CC) $(CFLAGS) $< -o memory
memory.o: memory.c
	$(CC) $(CFLAGS) -c memory.c

clean:
	rm memory *.o
