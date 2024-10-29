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
void handle_connection(int connfd) {
    /* Your code here */
    char buffer[2049];
    ssize_t res = read_until(connfd, buffer, 2048, ""); 
    if (res == -1){
    	close(connfd);
 	return;
	} // maybe don't exit with error
    buffer[res] = '\0';
    printf("%s\n",buffer);	
    close(connfd);
    return;
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
