/**
 * @File push_order.c
 *
 * Order test for the queue.
 * Inputs: thread_count, push_per_thread
 * Output: 0 if success, 1 if failure
 * Compile: clang -Wall -Werror -Wextra -Wpedantic -Wstrict-prototypes
 *          [-DDEBUG] -o push_order push_order.c queue.c -lpthread
 *
 * @author Brian Zhao and Mitchell Elliott.
 *
 */

#include "queue.h"

#include <err.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t printmutex = PTHREAD_MUTEX_INITIALIZER;

#define ferror(...)                                                                                \
    do {                                                                                           \
        pthread_mutex_lock(&printmutex);                                                           \
        fprintf(stderr, "[%s:%s():%d]\t", __FILE__, __func__, __LINE__);                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
        fprintf(stderr, "\n");                                                                     \
        pthread_mutex_unlock(&printmutex);                                                         \
    } while (0);

#ifdef DEBUG
#define fdebug(...)                                                                                \
    do {                                                                                           \
        pthread_mutex_lock(&printmutex);                                                           \
        fprintf(stderr, "[%s:%s():%d]\t", __FILE__, __func__, __LINE__);                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
        fprintf(stderr, "\n");                                                                     \
        pthread_mutex_unlock(&printmutex);                                                         \
    } while (0);
#else
#define fdebug(x, ...) ((void) 0)
#endif

void *thread1(void *args) {
    queue_t *q = ((void **) args)[0];
    int64_t id = (int64_t) (((void **) args)[1]);
    int64_t push_per_thread = (int64_t) (((void **) args)[2]);

    for (int64_t i = 0; i < push_per_thread; i++) {
        if (!queue_push(q, (void *) (intptr_t) (i + (id << 8)))) {
            ferror("queue_push() failed");
            break;
        }

        fdebug("Pushed: %ld", i + (id << 8));
    }

    return NULL;
}

void *thread2(void *args) {
    queue_t *q = ((void **) args)[0];
    int64_t id = (int64_t) (((void **) args)[1]);
    int64_t push_per_thread = (int64_t) (((void **) args)[2]);
    int64_t clock[id];

    for (int64_t i = 0; i < id; i++) {
        clock[i] = 0;
    }

    void *rv;

    for (int64_t i = 0; i < id * push_per_thread; i++) {
        if (!queue_pop(q, &rv)) {
            ferror("queue_pop() failed");
            return (void *) 1;
        }

        fdebug("Popped: %ld", (int64_t) rv);

        if ((((int64_t) rv) & 0xff) < clock[((int64_t) rv) >> 8]) {
            ferror("Pop order violated: %ld", (intptr_t) rv);
            return (void *) 1;
        } else {
            clock[((int64_t) rv) >> 8] = (((int64_t) rv) & 0xff);
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        warnx("wrong arguments: %s", argv[0]);
        fprintf(stderr, "usage: %s <thread_count> <push_per_thread>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    int64_t thread_count = (int64_t) strtol(argv[1], &endptr, 10);

    if ((endptr && *endptr != '\0') || thread_count < 0) {
        warnx("invalid thread_count: %s", argv[1]);
        return EXIT_FAILURE;
    }

    int64_t push_per_thread = (int64_t) strtol(argv[2], &endptr, 10);

    if ((endptr && *endptr != '\0') || push_per_thread < 0) {
        warnx("invalid push_per_thread: %s", argv[2]);
        return EXIT_FAILURE;
    }

    fdebug("Queue test started with thread_count: %ld, push_per_thread: %ld", thread_count,
        push_per_thread);

    queue_t *q = queue_new(thread_count * push_per_thread);

    if (q == NULL) {
        warnx("queue_new() failed");
        return EXIT_FAILURE;
    }

    fdebug("Queue created");

    pthread_t t1, t2[thread_count];
    void *args1[3] = { q, (void *) (intptr_t) thread_count, (void *) (intptr_t) push_per_thread };
    void *args2[3 * thread_count];
    pthread_create(&t1, NULL, thread2, args1);

    for (int64_t i = 0; i < thread_count; i++) {
        args2[i * 3 + 0] = q;
        args2[i * 3 + 1] = (void *) (intptr_t) i;
        args2[i * 3 + 2] = (void *) (intptr_t) push_per_thread;
        pthread_create(t2 + i, NULL, thread1, args2 + (3 * i));
    }

    void *status;
    int rc = pthread_join(t1, &status);

    if (rc != 0) {
        ferror("pthread_join() failed: %ld", (intptr_t) rc);
        ferror("Return code: %d", EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    if (status != NULL) {
        ferror("Thread t1 joined with non-NULL status: %ld", (intptr_t) status);
        ferror("Return code: %d", EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    fdebug("Thread t1 joined");

    for (int64_t i = 0; i < thread_count; i++) {
        rc = pthread_join(t2[i], &status);

        if (rc != 0) {
            ferror("pthread_join() failed: %ld", (intptr_t) rc);
            ferror("Return code: %d", EXIT_FAILURE);
            return EXIT_FAILURE;
        }

        if (status != NULL) {
            ferror("Thread t2[%ld] joined with non-NULL status: %ld", i, (intptr_t) status);
            ferror("Return code: %d", EXIT_FAILURE);
            return EXIT_FAILURE;
        }

        fdebug("Thread t2[%ld] joined", i);
    }

    queue_delete(&q);
    fdebug("Queue deleted");
    fdebug("Return code: %d", EXIT_SUCCESS);
    return 0;
}
