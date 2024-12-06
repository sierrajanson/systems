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

struct thread_args {
	rwlock_t *rw;
	queue_t *q;
};
void handle_connection(int, rwlock_t *rw);
//void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void *thread_work_func(void *args){
	struct thread_args *t_args = (struct thread_args *)args;
	rwlock_t *rw = t_args->rw;
	queue_t *q = t_args->q;

	while(1){
	int *connfd;
	queue_pop(q, (void **)&connfd);
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

    char *endptr = NULL;
    size_t port = (size_t) strtoull(argv[1], &endptr, 10);
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
    int opt;
    int num_threads = 0;

    while ((opt = getopt(argc, argv,":if:lrx")) != -1)
	{
	switch(opt){
		case 't':
			num_threads = atoi(optarg);
			break;
		case '?':
			printf("unknown option :%c\n",optopt);
			break;
		}	
	}
    assert(num_threads != 0);
    queue_t *q = queue_new(64);
    rwlock_t *rw = rwlock_new(READERS, num_threads); 
	
    struct thread_args args;
    args.q = q;
    args.rw = rw;

    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++){
    	pthread_create(&threads[i], NULL, thread_work_func, (void *)&args);
    }    

    // initialize Queue if threads are blocked keep track of more requests
    /* Initialize worker threads, the queue, and other data structures */
    // block if all of your threads are active 
    /* Hint: You will need to change how handle_connection() is used */
    while (1) {
        int connfd = listener_accept(&sock);
	queue_push(q,(void *)&connfd);
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
        debug("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
	   // rw lock here
	   reader_lock(rw);	
	   // reader lock 		
            handle_get(conn);
	    reader_unlock(rw);
	    // reader unlock
        } else if (req == &REQUEST_PUT) {
	    // rw lock here
	    // writer lock 
	    writer_lock(rw);
            handle_put(conn);
	    writer_unlock(rw);
	    // writer unlock
        } else { // do we need a lock for this?
            handle_unsupported(conn);
        }
    }

    conn_delete(&conn);
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    debug("Handling GET request for %s", uri);
    const Response_t *res = NULL;
    (void) res;
    fprintf(stderr,"GET,/%s,200,1\n",uri);
    // What are the steps in here?

    // 1. Open the file.
    // If open() returns < 0, then use the result appropriately
    //   a. Cannot access -- use RESPONSE_FORBIDDEN
    //   b. Cannot find the file -- use RESPONSE_NOT_FOUND
    //   c. Other error? -- use RESPONSE_INTERNAL_SERVER_ERROR
    // (Hint: Check errno for these cases)!

    // 2. Get the size of the file.
    // (Hint: Checkout the function fstat())!

    // 3. Check if the file is a directory, because directories *will*
    // open, but are not valid.
    // (Hint: Checkout the macro "S_IFDIR", which you can use after you call fstat()!)

    // 4. Send the file
    // (Hint: Checkout the conn_send_file() function!)

    // 5. Close the file
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;
    debug("Handling PUT request for %s", uri);
	(void) res;
    // What are the steps in here?

    // 1. Check if file already exists before opening it.
    // (Hint: check the access() function)!

    // 2. Open the file.
    // If open() returns < 0, then use the result appropriately
    //   a. Cannot access -- use RESPONSE_FORBIDDEN
    //   b. File is a directory -- use RESPONSE_FORBIDDEN
    //   c. Cannot find the file -- use RESPONSE_FORBIDDEN
    //   d. Other error? -- use RESPONSE_INTERNAL_SERVER_ERROR
    // (Hint: Check errno for these cases)!

    // 3. Receive the file
    // (Hint: Checkout the conn_recv_file() function)!

    // 4. Send the response
    // (Hint: Checkout the conn_send_response() function)!

    // 5. Close the file
}

void handle_unsupported(conn_t *conn) {
    debug("Handling unsupported request");

    // Send responses
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}
