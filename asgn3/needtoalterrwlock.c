#include <pthread.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum { READERS, WRITERS, N_WAY } PRIORITY;

typedef struct rwlock {

    int n;

    PRIORITY p;

    int readers;

    int writers;

    int waiting_readers;

    int waiting_writers;

    pthread_mutex_t mtx;

    pthread_cond_t cond;

} rwlock_t;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {

    rwlock_t *rw = (rwlock_t *) malloc(sizeof(rwlock_t));

    if (rw == NULL)

        return NULL;

    pthread_mutex_init(&(rw->mtx), NULL);

    pthread_cond_init(&(rw->cond), NULL);

    rw->n = n;

    rw->p = p;

    rw->readers = 0;

    rw->writers = 0;

    rw->waiting_readers = 0;

    rw->waiting_writers = 0;

    return rw;
}

void rwlock_delete(rwlock_t **rw) {

    if (rw == NULL || *rw == NULL)

        return;

    pthread_mutex_destroy(&((*rw)->mtx));

    pthread_cond_destroy(&((*rw)->cond));

    free(*rw);

    *rw = NULL;
}

void reader_lock(rwlock_t *rw) {

    pthread_mutex_lock(&(rw->mtx));

    rw->waiting_readers++;

    while (rw->writers > 0 || (rw->p == N_WAY && rw->waiting_writers > 0 && rw->readers >= rw->n)) {

        pthread_cond_wait(&(rw->cond), &(rw->mtx));
    }

    rw->waiting_readers--;

    rw->readers++;

    pthread_mutex_unlock(&(rw->mtx));
}

void reader_unlock(rwlock_t *rw) {

    pthread_mutex_lock(&(rw->mtx));

    rw->readers--;

    if (rw->readers == 0) {

        pthread_cond_broadcast(&(rw->cond));
    }

    pthread_mutex_unlock(&(rw->mtx));
}

void writer_lock(rwlock_t *rw) {

    pthread_mutex_lock(&(rw->mtx));

    rw->waiting_writers++;

    while (rw->writers > 0 || rw->readers > 0) {

        pthread_cond_wait(&(rw->cond), &(rw->mtx));
    }

    rw->waiting_writers--;

    rw->writers = 1;

    pthread_mutex_unlock(&(rw->mtx));
}

void writer_unlock(rwlock_t *rw) {

    pthread_mutex_lock(&(rw->mtx));

    rw->writers = 0;

    pthread_cond_broadcast(&(rw->cond));

    pthread_mutex_unlock(&(rw->mtx));
}
