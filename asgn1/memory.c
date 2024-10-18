#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h> //opendir
#include <assert.h>
#define MAX 10000000

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
        char *txt = strstr(filename, ".txt");
        if (txt == NULL)
            fatal_error("Invalid Command\n");
        // create or open file
        // does this check for directories?? !!
        int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
        if (fd == -1)
            fatal_error("Invalid Command\n");
        //printf("opened file\n");
        // initialize FILELENGTH
        int maxlen = 12; // max length of string length
        char str_length[maxlen]; // str(len)
        char buffer[4096];
        read_value(maxlen, str_length);
        int length = atoi(str_length);

        /*if (length >= MAX){
		printf("MAX MEMORY USING UNDEFIED BEHAVIOR\n");
		exit(0);
	}*/
        // if no bytes are to be written
        if (length == 0) {
            write(STDOUT_FILENO, "OK\n", 3);
            close(fd);
            exit(0);
        }

        // reset tracker variables
        //printf("past:%d\n",length);
        char c;
        int count = 0;
        int already_written = 0;
        // ---------------------------------------------------------------//
        // READING IN MESSAGE CONTENT ------------------------------------//
        // ---------------------------------------------------------------//
        // CHANGE TO 10 MB
        if (length > 10000000)
            length = 10000000;
        while (count < length && read(STDIN_FILENO, &c, 1) > 0) {
            if (count != 0 && (count % 4096) == 0) {
                // must reset buffer, and write all characters to memory
                int bytes = 0;
                while (bytes < 4096) {
                    int val = write(fd, buffer,
                        4096 - bytes); //is this resetting the buffer, or do i need to update it
                    // printf("bytes written:%d\n",val);
                    if (val < 0)
                        fatal_error("Operation Failed\n");
                    bytes += val;
                }
                already_written += 4096;
                for (int i = 0; i < 4096; i++) {
                    buffer[i] = 'A';
                }
            }
            if (c == '\n') {
                //user_success = 1;
                buffer[count % 4096] = c;
                count++;
                if (read(STDIN_FILENO, &c, 1) <= 0) {
                    break;
                }
                // check if there's stuff after
            }
            buffer[count % 4096] = c;
            count++;
        }
        //for (int i = 0; i < count; i++){
        //	printf("%c",buffer[i]);
        //}
        //exit(0);
        //printf("%d: total length\n",length);
        //printf("%d: count\n",count);
        //printf("%d:length of buffer\n", msg_len);
        if (length > count) { // not enough of msg to write
            length = count; // shorten length
        }
        int bytes = 0;
        int goal = length - already_written;
        while (bytes < goal) {
            bytes += write(fd, buffer, goal - bytes);
        }

        // successful finish
        write(STDOUT_FILENO, "OK\n", 3);
        close(fd);
        //check that file has closed !!

    } else if (strncmp(user_input, "get", 3) == 0) { // get
        //int res = scanf(" %[^\n]", filename);
        // edge case: what if 4096 passed and no newline --> invalid?
        // null characters necessary
        /*
	read in one character at a time. if still reading by 4096 and no newline has been reached, exit with fatal error
	*/
        // READING IN FILENAME
        // checks --> if filename is too long
        //        --> if filename is not terminated by a newline character
        //        --> if filename is termiated by \n but there is still following
        char filename[PATH_MAX];
        int res = read_value(PATH_MAX, filename);
        assert(res < 4096);
        filename[res] = '\0';

        //printf("%s\n",filename);
        char c; // may check for extra stuff after \n
        //printf("acquired filename\n");
        if (read(STDIN_FILENO, &c, 1) > 0)
            fatal_error("Invalid Command\n");
        // no \n, or filename too long
        //printf("no more stuff after file\n");
        char *dirname = NULL;
        if (opendir(filename) != NULL) {
            free(dirname);
            fatal_error("Invalid Command\n");
        }
        free(dirname);
        //printf("filename:%s\n",filename);
        int fd = open(filename, O_RDONLY, 0);

        // may not exceed 10,000,000 bytes
        if (fd == -1)
            fatal_error("Invalid Command\n");
        char buffer[4096];

        ssize_t bytes = 0;
        while (bytes < 10000000 && (bytes = read(fd, buffer, 16)) > 0) {
            for (int i = 0; i < bytes; i++) {
                fprintf(stdout, "%c", buffer[i]);
            }
        }

        close(fd);

    } else { // error
        fatal_error("Invalid Command\n");
    }

    (void) argc;
    (void) argv;
    return 0;
}
