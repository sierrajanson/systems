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

#define BUFFER_SIZE 4096

/** @brief Handles a connection from a client.
 *
 *  @param connfd The file descriptor for the connection.
 *
 *  @return void
 */

void send_response(int soc, int status_code){
	char *buf;
	switch (status_code){
		case 200:
		    buf = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
		    break;
		case 201:
		    buf = "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n";
		    break;
		case 400:
		    buf = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n";
		    break;
		case 403:
		    buf = "HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\n\r\nForbidden\n";
		    break;
		case 404:
		    buf = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found\n";
		    break;
		case 500:
		    buf = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server Error\n";
		    break;
		case 501:
		    buf = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n";
		    break;
		case 505:
		    buf = "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not Supported\n";
		    break;
		}
	int res = write_n_bytes(soc, buf, strlen(buf));//soc, buf, n);
	if (res == -1) printf("erorr sending code\n");
	}

int get(int connfd, const char *filename){
	// retrieve content from file into a buffer
	// write_n_bytes() all of message to client
	
	FILE *f = fopen(filename, "r");
	if (f == NULL) { // FILE NOT FOUND
		send_response(connfd, 404);
	} else {	
		int fd = open(filename, O_RDONLY, 0);
		if (fd == -1) return -1;
		char buffer[4096];
		ssize_t bytes_read = 0;
		ssize_t res = 0;
		while ((res = read(fd, buffer, 4096)) > 0){
			bytes_read += res;	
		}
		res = write_n_bytes(connfd, buffer, bytes_read);
		if (res == -1) return -1;
		fclose(f);
		send_response(connfd, 200);
	}	
	// if length of a file is 0 do something special
	return 0;
}

	
int put(int connfd, const char *filename, const char *message){
	// produce response with status-code, message-body, and content=length	
    	printf("filename:%s message:%s\n",filename,message);
	
	FILE *f = fopen(filename, "r");
	int code = 0;
	if (f == NULL) {code = 201;} else {code = 200;fclose(f);}	
	printf("code: %d\n",code);
	
	int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
	if (fd == -1) return -1;
	printf("fd created");	
	unsigned long bytes_written = 0;
	while (bytes_written < strlen(message)){
		ssize_t res = write(fd, message + bytes_written, strlen(message) - bytes_written);
		if (res < 0) return -1;
		bytes_written += res;
	}
	printf("code: %d\n",code);
	send_response(connfd, code);
	return 0;	
}



void handle_connection(int connfd) {
    char buffer[2049];
    ssize_t res = read_until(connfd, buffer, 2048, ""); 
    if (res == -1){
	printf("error\n");
        return; 
    } // maybe don't exit with error
    buffer[res] = '\0';
    //int result = put(connfd, "happy.txt", "new");
    int result = get(connfd, "hoppy.txt");
    if (result == -1) printf("Error\n");
    if (result == 0) printf("Success\n");
    close(connfd);
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
    if (res < 0){
	write(STDERR_FILENO, "Invalid port\n", 14); // ensure 14 not 13
	return -1;
    }

    while (1) {
        int connfd = listener_accept(&sock);
	handle_connection(connfd);
    }

    return EXIT_SUCCESS;
}
