#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int fatal_error(const char *message){
	fprintf(stderr,"%s", message);
	exit(1);
}


int main(int argc, char **argv) {
    // program input
    char user_input[4];
    int res = scanf("%s", user_input);

    // parse input

    if (res <= 0) fatal_error("Invalid command.\n");
    if (strcmp(user_input, "set") == 0) { // set --> set\n<location>\n<content_length>\n<contents>
        char filename[100]; // dynamically allocate
        int res = scanf("%s", filename);
    	if (res <= 0) fatal_error("Invalid command.\n");

        // need to reset file contents?!!

        int fd = open(filename, O_CREAT | O_WRONLY, S_IRWXU | S_IRGRP);

        if (fd == -1) fatal_error("Invalid command.\n");
        int length = 0;
        res = scanf("%d", &length);

        char buffer[4096];

        for (int i = 0; i < 4096; i++) {
            buffer[i] = '\0';
        }

        res = scanf("%s", buffer);

        int bytes = 0;
        while (bytes < length) {
            bytes += write(fd, buffer, length - bytes);
            if (buffer[bytes] == '\0') {
                bytes = length;
            }
        }
        close(fd);

    } else if (strcmp(user_input, "get") == 0) { // get
        char filename[100];
        int res = scanf("%s", filename);
        int fd = open(filename, O_RDONLY, 0);

        // check to see if there's extra info included in command !!
        char buffer[4096];

        if (res <= 0 || fd == -1) fatal_error("Invalid command.\n");

        ssize_t bytes = 0;
        while ((bytes = read(fd, buffer, 16)) > 0) {
            for (int i = 0; i < bytes; i++) {
                fprintf(stdout, "%c", buffer[i]);
            }
        }

        close(fd);

    } else { // error
       fatal_error("Invalid command.\n");
    }

    (void) argc;
    (void) argv;
    return 0;
}
