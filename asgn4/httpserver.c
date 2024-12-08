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
#include <search.h>
#define OPTI "t"

pthread_mutex_t lock;
ENTRY item, *ret;
struct thread_args {
    int num_threads;
    int tid;
    queue_t *q;
};

void handle_connection(int, int, int); //rwlock_t *rw);
void handle_get(conn_t *, int);
void handle_put(conn_t *, int);
void handle_unsupported(conn_t *);
volatile int running = 1;
volatile int num_threads = 0;
void handle_sigint(int sig) {
    (void) sig;
    fprintf(stderr, "sigint called\n");
    for (int i = 0; i < num_threads; i++) {
        pthread_exit(NULL);
    }
    running = 0;
}
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// FUNCTION FOR THE THREAD TO COMPLETE
void *thread_work_func(void *args) {
    // PARSE ARGS
    struct thread_args *t_args = (struct thread_args *) args;
    int num_threads = t_args->num_threads;
    queue_t *q = t_args->q;
    int tid = t_args->tid;
    // CONTINUOUSLY POP FROM QUEUE
    while (running) {
        (void) num_threads;
        (void) tid;
        (void) q;
        pthread_mutex_lock(&queue_mutex);
        while (queue_is_empty(q) && running) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        if (!running) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        int *cfd = NULL;

        queue_pop(q, (void **) &cfd);
        pthread_mutex_unlock(&queue_mutex);
        if (cfd != NULL) {
            handle_connection(*cfd, num_threads, tid);
        }
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

    while ((opt = getopt(argc, argv, OPTI)) != -1) {
        switch (opt) {
        case 't':
            num_threads = atoi(argv[optind]); //optarg);
            break;
        case '?': printf("unknown option :%c\n", optopt); break;
        }
    }
    assert(num_threads != 0); // ||  "number of threads is 0, cannot continue");
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

    signal(SIGINT, handle_sigint);
    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    if (listener_init(&sock, port) < 0) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    // -----------------------------------
    // CREATING THREADS
    // ----------------------------------

    queue_t *q = queue_new(512);

    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        struct thread_args args;
        args.q = q;
        args.tid = i;
        args.num_threads = num_threads;
        pthread_create(&threads[i], NULL, *thread_work_func, (void *) &args);
    }

    // CREATING HASHMAP
    hcreate(512);

    while (running) {
        printf("waiting for connection...\n");
        int connfd = listener_accept(&sock);
        pthread_mutex_lock(&queue_mutex);
        queue_push(q, (void *) &connfd);
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
    }
    running = 0;

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    queue_delete(&q);
    hdestroy();
    fprintf(stderr, "exiting now\n");
    return EXIT_SUCCESS;
}

void handle_connection(int connfd, int num_threads, int tid) { //, rwlock_t *rw) {
    ENTRY holder, *found_holder;
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);

    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        // GET REQUEST
        const Request_t *req = conn_get_request(conn);

        // -------------------------------------
        // PROTECT RW LOCK IN HASHMAP
        // ------------------------------------
        // trunc tmpfile, dont have to trunc uri
        char *uri = conn_get_uri(conn);
        holder.key = uri;
        pthread_mutex_lock(&lock); // DS protected by a lock!
        found_holder = hsearch(holder, FIND);
        printf("stuck in here?\n");
        if (found_holder == NULL) {
            printf("value was not in hashmpa\n");
            holder.key = uri;
            holder.data = rwlock_new(N_WAY, num_threads);
            hsearch(holder, ENTER);
        } else {
            printf("value was in hashmap\n");
        }
        pthread_mutex_unlock(&lock);
        if (req == &REQUEST_GET) {
            printf("get \n");
            handle_get(conn, tid);
        } else if (req == &REQUEST_PUT) {
            printf("put \n");
            fflush(stdout);
            handle_put(conn, tid);
        } else { // do we need a lock for this?
            handle_unsupported(conn);
        }
    }
    close(connfd);
    conn_delete(&conn);
}

void handle_get(conn_t *conn, int tid) {
    // SEARCH FOR LOCK
    (void) tid;
    char *uri = conn_get_uri(conn);
    item.key = uri;
    ret = hsearch(item, FIND);
    rwlock_t *rw = (rwlock_t *) ret->data;
    //debug("Handling GET request for %s", uri);
    printf("URI of file: %s\n", uri);
    // GET RID
    char *rid = conn_get_header(conn, "Request-Id");
    printf("RID of file: %s\n", rid);
    int sc = 0;

    reader_lock(rw);
    int fd = open(uri, O_RDONLY);
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        printf("ERROR reading length of file\n");
    } else {
        printf("ERROR reading length of file\n");
    }
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
        fprintf(stderr, "GET,/%s,%d,%s\n", uri, sc, rid);
        reader_unlock(rw);
        return;
    }
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
    close(fd);
    fprintf(stderr, "GET,/%s,%d,%s\n", uri, sc, rid);
    fflush(stderr);
    reader_unlock(rw);
}
// tnam
// debug("Handling PUT request for %s", uri);
void handle_put(conn_t *conn, int tid) { //, int num_threads) {

    // -----------------------------------
    // FINDING RW LOCK
    // -----------------------------------
    char *uri = conn_get_uri(conn);
    char *rid = conn_get_header(conn, "Request-Id");
    item.key = uri;
    ret = hsearch(item, FIND);
    rwlock_t *rw = (rwlock_t *) ret->data;

    // ------------------------------------
    // CREATING TMP FILENAME
    char conn_to_str[10];
    sprintf(conn_to_str, "%d", tid);
    char tmp_fn[] = "ti";
    strcat(tmp_fn, conn_to_str);
    //char generate[L_tmpnam + 1];
    //tmpnam(generate);
    // -----------------------------------

    // CREATE UNIQUE TMP FILE
    int tmp_fd = open(tmp_fn, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
    assert(tmp_fd != -1);
    const Response_t *result = conn_recv_file(conn, tmp_fd);
    close(tmp_fd);

    // LOCKING --> ONCE FILE HAS BEEN WRITTEN
    writer_lock(rw);

    int sc = 0;
    if (access(uri, F_OK) != 0) { // have to create file if it doesn't exist
        sc = 201;
    } else {
        sc = 200;
    }

    int fd = open(uri, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
    rename(tmp_fn, uri);
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
        writer_unlock(rw);
        fprintf(stderr, "GET,/%s,%d,%s\n", uri, sc, rid);
        return;
    }
    if (sc == 200)
        result = conn_send_response(conn, &RESPONSE_OK);
    if (sc == 201)
        result = conn_send_response(conn, &RESPONSE_CREATED);
    if (result != NULL) {
        printf("error in receiving file!!!\n");
    }
    close(fd);
    printf("file closed\n");
    if (rid == NULL) {
        printf("request id was null\n");
        rid = "0";
    }
    fprintf(stderr, "PUT,/%s,%d,%s\n", uri, sc, rid);
    fflush(stderr);
    writer_unlock(rw);
}

void handle_unsupported(conn_t *conn) {
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}
