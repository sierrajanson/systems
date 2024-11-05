/**
 * @File httpserver.c
 *
 * This file contains the main function for the HTTP server.
 *
 * @author Sierra Janson :)
 */

#include "asgn2_helper_funcs.h"
#include "debug.h"
#include "protocol.h"
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex.h>
#define BUFFER_SIZE 4096
#define LENGTH_REGEX "Length: ([0-9]+{1,128})"
//#define END_REGEX "\r\n(*{100})" // change later!!!
//#define EXTEND_REGEX HEADER_FIELD_REGEX END_REGEX

/** @brief Handles a connection from a client.
 *
 *  @param connfd The file descriptor for the connection.
 *
 *  @return void
 */

void send_response(int soc, int status_code) {
    char *buf = NULL;
    switch (status_code) {
    case 200: buf = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n"; break;
    case 201: buf = "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n"; break;
    case 400: buf = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n"; break;
    case 403: buf = "HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\n\r\nForbidden\n"; break;
    case 404: buf = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found\n"; break;
    case 500:
        buf = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server "
              "Error\n";
        break;
    case 501:
        buf = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n";
        break;
        "Supported\n";
        break;
    }

    if (buf) {
        if (write_n_bytes(soc, buf, strlen(buf)) == -1) {
            fprintf(stderr, "Error sending response code %d\n", status_code);
        }
    }
}
int integer_len(int val) {
    int count = 0;
    while (val / 10 >= 1) {
        count += 1;
        val = val / 10;
    }
    return count;
}

int get(int connfd, const char *filename) {
    char *dirname = NULL;
    if (opendir(filename) != NULL) {
        //free(dirname);
        send_response(connfd, 403);
        return -1;
    }
    free(dirname);
    int fd = open(filename, O_RDONLY, 0);

    if (fd == -1) {
        send_response(connfd, 404);
        return -1;
    }
    struct stat fileStat;
    if (stat(filename, &fileStat) != 0) {
        send_response(connfd, 500);
        close(fd);
        return -1; // Error occurred
    }
    long long length = fileStat.st_size;
    char *beginning = "HTTP/1.1 200 OK\r\nContent-Length: \r\n\r\n";
    int size = strlen(beginning) + snprintf(NULL, 0, "%lld", length) + 2;
    char response[size];
    snprintf(response, size, "HTTP/1.1 200 OK\r\nContent-Length: %lld\r\n\r\n", length);
    int res = write_n_bytes(connfd, response, size - 2);
    if (res == -1) {
        send_response(connfd, 500);
        close(fd);
        return -1; // Error occurred
    }
    char buffer[PATH_MAX];
    ssize_t bytes_read = 0;

    // write away rest of buffer then start afresh i guess

    while ((bytes_read = read(fd, buffer, PATH_MAX)) > 0) {
        ssize_t bytes_written = 0;
        // writing the bytes
        while (bytes_written < bytes_read) {
            ssize_t result = write(connfd, buffer + bytes_written, bytes_read - bytes_written);
            // operation faile
            if (result < 0) {
                send_response(connfd, 500);
                close(fd);
                return 1;
            }
            bytes_written += result;
        }
    }
    close(fd);
    return 0;
}

int put(int connfd, const char *filename, const char *message) {
    if (!filename || !message) {
        fprintf(stderr, "Filename or message is NULL\n");
        return -1;
    }
    FILE *f = fopen(filename, "r");
    int code = 0;
    if (f == NULL) {
        code = 201;
    } else {
        code = 200;
        fclose(f);
    }

    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
    if (fd == -1) {
        send_response(connfd, 500);
        return -1;
    }
    unsigned long bytes_written = 0;
    while (bytes_written < strlen(message)) {
        ssize_t res = write(fd, message + bytes_written, strlen(message) - bytes_written);
        if (res < 0) {
            send_response(connfd, 500);
            return -1;
        }
        bytes_written += res;
    }
    if (close(fd) == -1) {
        send_response(connfd, 500);
        return -1;
    }

    send_response(connfd, code);
    return 0;
}

regoff_t match_pattern(const char *buffer, char *substring, const char *pattern, int size) {
    regex_t preg;
    int rc = regcomp(&preg, pattern, REG_EXTENDED);
    if (rc != 0) {
        return -1;
    }

    size_t nmatch = 8;
    regmatch_t pmatch[8];
    rc = regexec(&preg, buffer, nmatch, pmatch, 0);

    if (rc != 0) {
        // No match found, so we should free the regex and return -1
        regfree(&preg);
        return -1;
    }

    // Calculate the length of the matched substring
    regoff_t len = pmatch[0].rm_eo - pmatch[0].rm_so;
    if (len > size - 1) {
        len = size - 1; // Ensure we don't exceed the buffer
    }

    snprintf(substring, size, "%.*s", (int) len, buffer + pmatch[0].rm_so);
    regfree(&preg);
    return pmatch[0].rm_eo; // Return the end offset of the match
}

int handle_filename(int connfd, const char *filename) {
    if (!filename) {
        fprintf(stderr, "Filename or message is NULL\n");
        return -1;
    }
    FILE *f = fopen(filename, "r");
    int code = 0;
    if (f == NULL) {
        code = 201;
    } else {
        code = 200;
        fclose(f);
    }
    return code;
    printf("move file code\n");
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
    if (fd == -1) {
        send_response(connfd, 500);
        return -1;
    }
    return fd;
}

void handle_connection(int connfd) {
    int BUFF_SIZE = 2049;
    char buffer[BUFF_SIZE];
    ssize_t res = read_until(connfd, buffer, BUFF_SIZE - 1, "");

    if (res == -1) {
        send_response(connfd, 500);
        return;
    }
    buffer[res] = '\0';
    char command[8];
    res = match_pattern(buffer, command, METHOD_REGEX, 8);
    if (res == -1) {
        send_response(connfd, 500);
        return;
    }
    char uri[64];
    res = match_pattern(buffer, uri, URI_REGEX, 64);
    if (res == -1) {
        send_response(connfd, 500);
        return;
    }

    // GET
    if (strncmp(command, "GET", 3) == 0) {
        get(connfd, uri + 1);
    }
    // PUT
    else if (strncmp(command, "PUT", 3) == 0) {

        // write rest of bytes from og buffer into file
        // then create new buffer

        //char holder[258];
        //printf("buffer: %s\n----------\n", buffer);
        //int p = match_pattern(buffer, holder, HEADER_FIELD_REGEX, 128);
        char value[128];
        //printf("holder: %s\n-----------\n", holder);
        int p = match_pattern(buffer, value, LENGTH_REGEX, 128);
	//printf("index:--%d--, character--%c%c%c%chmm%c--", p, buffer[p], buffer[p+1],buffer[p+2],buffer[p+3],buffer[p+4]);
	//printf("value: ------- %s\n---------------\n",value);
        int length = atoi(value+8);
        
	//printf("MESSAGE LEN: %d\n", length);
	
        int message_pointer = p + 4;
        int code = handle_filename(connfd, uri + 1);
        char *filename = uri + 1;
        int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
        if (fd == -1) {
            send_response(connfd, 500);
            return;
        }

        int remainder = strlen(buffer) - message_pointer;
        ssize_t res = write_n_bytes(fd, buffer + message_pointer, remainder);
        if (res == -1) {
            send_response(connfd, 500);
            return;
        }

        if (length - remainder > 0) {
            res = pass_n_bytes(connfd, fd, length - remainder);
            if (res == -1) {
                send_response(connfd, 500);
                return;
            }
        }

        //int bytes_read = 0;
        /*
        char buffer_remainder[strlen(buffer) - message_pointer]; // mp is 0-indexed
        //printf("size of buffer: %d\n",(int)strlen(buffer));
        printf("diff%lu\n", strlen(buffer) - message_pointer);
        for (int i = message_pointer; i < (int) strlen(buffer); i++) {
            buffer_remainder[i - message_pointer] = buffer[i];
        }
        buffer_remainder[strlen(buffer)] = '\0';
        //printf("size of remainder:%lu\n", strlen(buffer_remainder));
        ssize_t bytes_written = 0;
        int HARDCODE = 2;
	
        while (bytes_written < (int) strlen(buffer_remainder) - HARDCODE) {
            ssize_t result = write(fd, buffer_remainder + bytes_written,
                strlen(buffer_remainder) - bytes_written - HARDCODE);
            bytes_written += result;
            printf("are you getting stuck?\n");
        }
        printf("in btwn\n");*/
        //pass_n_bytes(connfd,fd,length);
        // ---------------------------------------------------------------//
        // READING IN MESSAGE CONTENT ------------------------------------//
        // ---------------------------------------------------------------//
        //char newBuffer[2049];
        //printf("hello\n");
        //bytes_read = read(connfd, newBuffer, 1);
        /*while ((bytes_read = read(connfd, buffer, PATH_MAX)) > 0) {
            bytes_written = 0;
            printf("reading in message content\n");
            // writing the bytes
            while (bytes_written < bytes_read) {
                ssize_t result = write(fd, buffer + bytes_written, bytes_read - bytes_written);
                printf("bytes written:%d\n", (int) result);
                // error message
                if (result < 0) {
                    send_response(connfd, 500);
                    return;
                }
                bytes_written += result;
            }
            printf("stuck here??\n");
        }if (bytes_read == 0) {
 	   // Connection closed cleanly; exit the loop
	      printf("End of data received.\n");
	} else if (bytes_read < 0) {
  	  perror("read error");
  	  send_response(connfd, 500);
    	return;
	}*/
        //printf("sending code??\n");
        send_response(connfd, code);
        //printf("reutnring..\n");
        close(fd);
        return;

        /*
		figure out where to start reading from...
                initialize message buffer to be equal to size 2049
		while bytes read < length
			read into buffer
			while bytes_written < bytes_read
				write to connfd
				

	*/
        //char message[length + 1];
        //for (int i = message_pointer; i < message_pointer + length; i++) {
        ////    message[i - message_pointer] = buffer[i];
        // }
        //message[length] = '\0'; // terminate just in case
        //put(connfd, uri + 1, message); // add one to uri to bypass /

    } else {
        send_response(connfd, 505);
        return;
    }
}

/** @brief Main function for the HTTP server.
 *
 *  @param argc The number of arguments.
 *  @param argv The arguments.
 *
 *  @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise.
 */
int main(int argc, char **argv) {
    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    size_t port = (size_t) strtoull(argv[1], &endptr, 10);

    /* Add error checking for the port number */
    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    int res = listener_init(&sock, port);
    if (res < 0) {
        write(STDERR_FILENO, "Invalid port\n", 14); // ensure 14 not 13
        return -1;
    }

    while (1) {
        int connfd = listener_accept(&sock);
        handle_connection(connfd);
        close(connfd);
    }

    return EXIT_SUCCESS;
}
