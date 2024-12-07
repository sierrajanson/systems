/**
 * @File rwlock.h
 *
 * The header file that you need to implement for assignment 3.
 *
 * @author Sierra Janson
 */

#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/** @struct rwlock_t
 *
 *  @brief This typedef renames the struct rwlock.  Your `c` file
 *  should define the variables that you need for your reader/writer
 *  lock.
 */
typedef struct rwlock rwlock_t;

typedef enum { READERS, WRITERS, N_WAY } PRIORITY;

typedef struct rwlock {
    int n;
    PRIORITY p;
    int readers;
    int writers;
    int waiting_readers;
    int waiting_writers;
    pthread_mutex_t mtx;

} rwlock_t;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rw = (rwlock_t *) malloc(sizeof(rwlock_t));

    if (rw == NULL)
        return NULL;
    pthread_mutex_init(&(rw->mtx), NULL);
    rw->n = n;
    rw->readers = 0;
    rw->writers = 0;
    rw->waiting_readers = 0;
    rw->waiting_writes = 0;
    rw->p = p;
    return rw;
}
void rwlock_delete(rwlock_t **rw) {
    if (rw == NULL || rw == NULL)
        return;
    pthread_mutex_destroy(&((*rw)->mtx));
    free(*rw);
    *rw = NULL;
}

void reader_lock(rwlock_t *rw) {
    if (rw->p == READERS) {
        pthread_mutex_lock(&(rw->mtx));
        rw->readers = rw->readers + 1;
        pthread_mutex_unlock(&(rw->mtx));
    } else if (rw->p == WRITERS) {
        // if writing
        pthread_mutex_lock(&(rw->mtx));
        while (rw->writers > 0) { // check if there is more than one writer
            pthread_mutex_unlock(&(rw->mtx)); // yield if so
            pthread_mutex_lock(&(rw->mtx));
        }
        rw->readers = rw->readers + 1; // otherwise increment # of readers
        pthread_mutex_unlock(&(rw->mtx));
    } else { // if n-way
        // should be similar to #1
    }
}
void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->mtx));
    rw->readers = rw->readers - 1;
    pthread_mutex_unlock(&(rw->mtx));
}
void writer_lock(rwlock_t *rw) {
    if (rw->p == READERS) {
        pthread_mutex_lock(&(rw->mtx));
        while (rw->readers > 0 && rw->writers != 0) {
            pthread_mutex_unlock(&(rw->mtx));
            pthread_mutex_lock(&(rw->mtx));
        }
        rw->writers = rw->writers + 1;
        pthread_mutex_unlock(&(rw->mtx));
    } else if (rw->p == WRITERS) {
        pthread_mutex_lock(&(rw->mtx));
        while (rw->writers != 0) {
            pthread_mutex_unlock(&(rw->mtx));
            pthread_mutex_lock(&(rw->mtx));
        }
        rw->writers = rw->writers + 1;
        pthread_mutex_unlock(&(rw->mtx));
    } else {
    }
}
void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->mtx));
    rw->writers = 0;
    pthread_mutex_unlock(&(rw->mtx));
}
