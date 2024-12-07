/**
 * @File pop_validity.c
 *
 * Validity test for the queue.
 * Inputs: thread_count, push_per_thread, push_range
 * Output: 0 if success, 1 if failure
 * Compile: clang -Wall -Werror -Wextra -Wpedantic -Wstrict-prototypes
 *          [-DDEBUG] -o pop_validity pop_validity.c queue.c -lpthread
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

int64_t done = 0;

void *thread1(void *args) {
    queue_t *q = ((void **) args)[0];
    int64_t thread_count = (int64_t) (((void **) args)[1]);
    int64_t push_per_thread = (int64_t) (((void **) args)[2]);
    int64_t push_range = (int64_t) (((void **) args)[3]);
    int64_t *push_count = ((int64_t **) args)[4];
    int64_t total_thread_count = thread_count * push_per_thread;

    for (int64_t i = 0; i < total_thread_count; i++) {
        int64_t val = random() % push_range;
        push_count[val]++;

        if (!queue_push(q, (void *) (intptr_t) (val))) {
            ferror("queue_push() failed");
            return (void *) 1;
        }

        fdebug("Pushed: %ld", val);
    }

    done = 1;

    for (int64_t i = 0; i < 2 * total_thread_count / push_per_thread; i++) {
        if (!queue_push(q, NULL)) {
            ferror("queue_push() failed");
            return (void *) 1;
        }

        fdebug("Pushed: NULL");
    }

    push_count[0] += 2 * total_thread_count / push_per_thread;
    return NULL;
}

void *thread2(void *args) {
    queue_t *q = ((void **) args)[0];
    int64_t *pop_count = ((int64_t **) args)[1];
    int64_t push_range = (int64_t) (((void **) args)[2]);
    void *rv;

    while (!done) {
        if (!queue_pop(q, &rv)) {
            ferror("queue_pop() failed");
            return (void *) 1;
        }

        int64_t val = (int64_t) rv;
        fdebug("Popped: %ld", val);

        if (done) {
            break;
        }

        if (val < push_range) {
            pop_count[val]++;
        } else {
            ferror("queue_pop() produced wrong value: %ld", val);
            return (void *) 1;
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        warnx("wrong arguments: %s", argv[0]);
        fprintf(stderr, "usage: %s <thread_count> <push_per_thread> <push_range>\n", argv[0]);
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

    int64_t push_range = (int64_t) strtol(argv[3], &endptr, 10);

    if ((endptr && *endptr != '\0') || push_range < 0) {
        warnx("invalid push_range: %s", argv[3]);
        return EXIT_FAILURE;
    }

    fdebug("Queue test started with thread_count: %ld, push_per_thread: %ld, push_range: %ld",
        thread_count, push_per_thread, push_range);

    queue_t *q = queue_new(thread_count * push_per_thread * 3);

    if (q == NULL) {
        warnx("queue_new() failed");
        return EXIT_FAILURE;
    }

    fdebug("Queue created");

    pthread_t t1, t2[thread_count];
    int64_t push_count[push_range];
    int64_t pop_count[thread_count * push_range];
    void *args1[5] = { q, (void *) (intptr_t) thread_count, (void *) (intptr_t) push_per_thread,
        (void *) (intptr_t) push_range, push_count };
    void *args2[3 * thread_count];
    memset(push_count, 0, push_range * sizeof(int64_t));
    memset(pop_count, 0, thread_count * push_range * sizeof(int64_t));
    pthread_create(&t1, NULL, thread1, args1);

    for (int64_t i = 0; i < thread_count; i++) {
        args2[i * 3 + 0] = q;
        args2[i * 3 + 1] = pop_count + i * push_range;
        args2[i * 3 + 2] = (void *) (intptr_t) push_range;
        pthread_create(t2 + i, NULL, thread2, args2 + (3 * i));
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

    for (int64_t i = 0; i < push_range; i++) {
        int64_t sum = 0;

        for (int64_t j = 0; j < thread_count; j++) {
            sum += pop_count[i + j * push_range];
        }

        if (sum > push_count[i]) {
            ferror("Invalid sum: %ld", sum);
            ferror("Return code: %d", EXIT_FAILURE);
            return EXIT_FAILURE;
        }

        fdebug("Sum: %ld", sum);
    }

    queue_delete(&q);
    fdebug("Queue deleted");
    fdebug("Return code: %d", EXIT_SUCCESS);
    return EXIT_SUCCESS;
}
