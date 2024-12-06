// Asgn 2: A simple HTTP server.
// By: Eugene Chou
//     Andrew Quinn
//     Brian Zhao

#include "asgn2_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "response.h"
#include "request.h"
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "queue.h"
#include "rwlock.h"
#include <pthread.h>
#define OPTI "t"

struct thread_args {
    rwlock_t *rw;
    queue_t *q;
};
void handle_connection(int, rwlock_t *rw);
//void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void *thread_work_func(void *args) {
    struct thread_args *t_args = (struct thread_args *) args;
    rwlock_t *rw = t_args->rw;
    queue_t *q = t_args->q;

    while (1) {
        int *connfd;
        queue_pop(q, (void **) &connfd);
        handle_connection(*connfd, rw);
        close(*connfd);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    /* Use getopt() to parse the command-line arguments */
    int opt;
    int num_threads = 0; // atoi(argv[2]);

    while ((opt = getopt(argc, argv, OPTI)) != -1) {
        switch (opt) {
        case 't':
            num_threads = atoi(argv[optind]); //optarg);
            break;
        case '?': printf("unknown option :%c\n", optopt); break;
        }
    }
    assert(num_threads != 0 && "number of threads is 0, cannot continue");
    char *endptr = NULL;
    size_t port = (size_t) strtoull(argv[optind + 1], &endptr, 10);
    if (endptr && *endptr != '\0') {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    if (listener_init(&sock, port) < 0) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    // initialize queue and RW lock
    queue_t *q = queue_new(512);
    rwlock_t *rw = rwlock_new(READERS, num_threads);

    struct thread_args args;
    args.q = q;
    args.rw = rw;

    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, thread_work_func, (void *) &args);
    }

    // initialize Queue if threads are blocked keep track of more requests
    /* Initialize worker threads, the queue, and other data structures */
    // block if all of your threads are active
    /* Hint: You will need to change how handle_connection() is used */
    while (1) {
        printf("waiting for connection...\n");
        int connfd = listener_accept(&sock);
        printf("connfd: %d\n", connfd);
        queue_push(q, (void *) &connfd);
    }
    //handle_connection(connfd);
    //close(connfd);
    // if not possible --> wait
    // continue loop
    return EXIT_SUCCESS;
}

void handle_connection(int connfd, rwlock_t *rw) {
    conn_t *conn = conn_new(connfd);

    const Response_t *res = conn_parse(conn);
    // block in here if atomicity/concurrency is violated
    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        //debug("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            reader_lock(rw);
            handle_get(conn);
            reader_unlock(rw);
        } else if (req == &REQUEST_PUT) {
            writer_lock(rw);
            handle_put(conn);
            writer_unlock(rw);
        } else { // do we need a lock for this?
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    //debug("Handling GET request for %s", uri);
    printf("URI of file: %s\n", uri);
    char *rid = conn_get_header(conn, "Request-Id");
    printf("RID of file: %s\n", rid);
    int sc = 0;
    // What are the steps in here?

    // 1. Open the file.
    // If open() returns < 0, then use the result appropriately
    //   a. Cannot access -- use RESPONSE_FORBIDDEN
    //   b. Cannot find the file -- use RESPONSE_NOT_FOUND
    //   c. Other error? -- use RESPONSE_INTERNAL_SERVER_ERROR
    // (Hint: Check errno for these cases)!
    int fd = open(uri, O_RDONLY);
    printf("fd: %d\n", fd);
    if (fd == -1) {
        printf("couldn't open file\n");
        strerror(errno);
        switch (errno) {
        case EACCES:
            sc = 403;
            conn_send_response(conn, &RESPONSE_FORBIDDEN);
            break;
        case ENOENT:
            sc = 404;
            conn_send_response(conn, &RESPONSE_NOT_FOUND);
            break;
        case EISDIR:
            sc = 403;
            conn_send_response(conn, &RESPONSE_FORBIDDEN);
            break;
        default:
            sc = 500;
            conn_send_response(conn, &RESPONSE_INTERNAL_SERVER_ERROR);
            break;
        }
        close(fd);
        return;
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        printf("ERROR reading length of file\n");
    } else {
        printf("got length of file successfully\n");
    }
    // 2. Get the size of the file.
    // (Hint: Checkout the function fstat())!

    // 3. Check if the file is a directory, because directories *will*
    // open, but are not valid.
    // (Hint: Checkout the macro "S_IFDIR", which you can use after you call fstat()!)

    // 4. Send the file
    // (Hint: Checkout the conn_send_file() function!)
    const Response_t *res = conn_send_file(conn, fd, file_stat.st_size);
    if (res != NULL) {
        printf("error in sending file!\n");
    } else {
        printf("res should be null\n");
    }
    if (rid == NULL) {
        printf("request id was null\n");
        rid = "0";
    }
    fprintf(stderr, "GET,/%s,%d,%s\n", uri, sc, rid);
    close(fd);
    // 5. Close the file
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    // debug("Handling PUT request for %s", uri);
    char *rid = conn_get_header(conn, "Request-Id");
    int sc = 0;
    // What are the steps in here?

    // 1. Check if file already exists before opening it.
    // (Hint: check the access() function)!
    if (access(uri, F_OK) != 0) { // have to create file if it doesn't exist
        sc = 201;
    } else {
        sc = 200;
    }
    // 2. Open the file.
    // If open() returns < 0, then use the result appropriately
    //   a. Cannot access -- use RESPONSE_FORBIDDEN
    //   b. File is a directory -- use RESPONSE_FORBIDDEN
    //   c. Cannot find the file -- use RESPONSE_FORBIDDEN
    //   d. Other error? -- use RESPONSE_INTERNAL_SERVER_ERROR
    // (Hint: Check errno for these cases)!

    int fd = open(uri, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
    printf("file opened\n");
    if (fd == -1) {
        strerror(errno);
        switch (errno) {
        case EACCES:
            sc = 403;
            conn_send_response(conn, &RESPONSE_FORBIDDEN);
            break;
        case ENOENT:
            sc = 404;
            conn_send_response(conn, &RESPONSE_NOT_FOUND);
            break;
        case EISDIR:
            sc = 403;
            conn_send_response(conn, &RESPONSE_FORBIDDEN);
            break;
        default:
            sc = 500;
            conn_send_response(conn, &RESPONSE_INTERNAL_SERVER_ERROR);
            break;
        }
        close(fd);
        return;
    }
    printf("got through to here\n");
    // 3. Receive the file
    // (Hint: Checkout the conn_recv_file() function)!
    const Response_t *result = conn_recv_file(conn, fd);
    printf("file received\n");
    // 4. Send the response
    // (Hint: Checkout the conn_send_response() function)!
    if (sc == 200)
    result = conn_send_response(conn, &RESPONSE_OK);
    if (sc == 201)
    result = conn_send_response(conn, &RESPONSE_CREATED);
    if (result != NULL) {
        printf("error in receiving file!!!\n");
    }
    // 5. Close the file
    close(fd);
    printf("file closed\n");
    if (rid == NULL) {
        printf("request id was null\n");
        rid = "0";
    }
    fprintf(stderr, "PUT,/%s,%d,%s\n", uri, sc, rid);
    printf("audit log wrirten too\n");
}

void handle_unsupported(conn_t *conn) {
    // debug("Handling unsupported request");

    // Send responses
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}
