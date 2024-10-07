# Author: Sierra Janson
# # 10/2/2024
#
#
CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic
#

all: hello

hello: hello.o
	$(CC) $(CFLAGS) $< -o hello
hello.o: hello.c
	$(CC) $(CFLAGS) -c hello.c
clean:
	rm -f hello *.o


