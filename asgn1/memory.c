#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h> //opendir
#include <assert.h>

int fatal_error(const char *message) {
    fprintf(stderr, "%s", message);
    exit(1);
}

int read_value(int max, char *value) {
    char c;
    int count = 0;
    int user_success = 0;
    while (count < max && read(STDIN_FILENO, &c, 1) > 0) {
        //printf("%c\n",c);
        if (c == '\n') {
            user_success = 1;
            break;
        }
        value[count] = c;
        count += 1;
    }
    if (user_success == 0)
        fatal_error("Invalid Command\n");
    return count;
}

int main(int argc, char **argv) {
    char user_input[4];
    read_value(4, user_input);
    //printf("command:%s\n",user_input);
    if (strncmp(user_input, "set", 3)
        == 0) { // set --> set\n<location>\n<content_length>\n<contents>
        // initialize FILENAME
        char filename[PATH_MAX]; // dynamically allocate

        int res = read_value(PATH_MAX, filename);
        assert(res < 4096);
        filename[res] = '\0';
        //printf("got to filename\n");
        // check for valid text file
        /*char *txt = strstr(filename, ".txt"); // <-- MAKE SURE .dat or .txt
        char *dat = strstr(filename, ".dat");
	if (txt == NULL && dat == NULL)
            fatal_error("Invalid Command\n");
        */
        // create or open file
        // does this check for directories?? !!
        int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
        if (fd == -1)
            fatal_error("Invalid Command\n");
        //printf("opened file\n");
        // initialize FILELENGTH
        int maxlen = 12; // max length of string length
        char str_length[maxlen]; // str(len)
        char buffer[PATH_MAX];

        read_value(maxlen, str_length);
        (void) str_length;

        int bytes_read = 0;
        // ---------------------------------------------------------------//
        // READING IN MESSAGE CONTENT ------------------------------------//
        // ---------------------------------------------------------------//
        while ((bytes_read = read(STDIN_FILENO, buffer, PATH_MAX)) > 0) {
            ssize_t bytes_written = 0;
            while (bytes_written < bytes_read) {
                ssize_t result = write(fd, buffer + bytes_written, bytes_read - bytes_written);
                if (result < 0)
                    fatal_error("Operation Failed\n");
                bytes_written += result;
            }
        }

        // successful finish
        write(STDOUT_FILENO, "OK\n", 3);
        close(fd);
        //check that file has closed !!

    } else if (strncmp(user_input, "get", 3) == 0) { // get
        // READING IN FILENAME
        // checks --> if filename is too long
        //        --> if filename is not terminated by a newline character
        //        --> if filename is termiated by \n but there is still following
        char filename[PATH_MAX];
        int res = read_value(PATH_MAX, filename);
        // should never be called -->
        assert(res < PATH_MAX);
        filename[res] = '\0';

        char c; // may check for extra stuff after \n
        if (read(STDIN_FILENO, &c, 1) > 0)
            fatal_error("Invalid Command\n");
        // no \n, or filename too long
        char *dirname = NULL;
        if (opendir(filename) != NULL) {
            free(dirname);
            fatal_error("Invalid Command\n");
        }
        free(dirname);
        int fd = open(filename, O_RDONLY, 0);

        if (fd == -1)
            fatal_error("Invalid Command\n");
        char buffer[PATH_MAX];

        ssize_t bytes_read = 0;
        while ((bytes_read = read(fd, buffer, PATH_MAX)) > 0) {
            ssize_t bytes_written = 0;
            while (bytes_written < bytes_read) {
                ssize_t result
                    = write(STDOUT_FILENO, buffer + bytes_written, bytes_read - bytes_written);
                if (result < 0)
                    fatal_error("Operation Failed\n");
                bytes_written += result;
            }
        }
        /*while ((bytes = read(fd, buffer, 16)) > 0) {
            for (int i = 0; i < bytes; i++) {
                fprintf(stdout, "%c", buffer[i]);
            }
        }*/

        close(fd);

    } else { // error
        fatal_error("Invalid Command\n");
    }

    (void) argc;
    (void) argv;
    return 0;
}
