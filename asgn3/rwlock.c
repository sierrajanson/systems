// author: sierra janson

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// priority types initialization
typedef enum { READERS, WRITERS, N_WAY } PRIORITY;

typedef struct rwlock {
    int n; // for NWAY priority
    PRIORITY p;
    int readers; // current # of readers
    int writers; // current # of writers
    int waiting_readers; // readers waiting to join
    int waiting_writers; // writers waiting to join
    pthread_mutex_t mtx; // mtx variable
    pthread_cond_t cond; // conditional variable

} rwlock_t;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rw = (rwlock_t *) malloc(sizeof(rwlock_t));
    if (rw == NULL) // return NULL upon failure
        return NULL;

    pthread_mutex_init(&(rw->mtx), NULL); // initialize mutex
    pthread_cond_init(&(rw->cond), NULL); // initialize conditional variable

    // initialize all starter variables
    rw->n = n;
    rw->p = p;
    rw->readers = 0;
    rw->writers = 0;
    rw->waiting_readers = 0;
    rw->waiting_writers = 0;

    return rw;
}

// delete the rwlock
void rwlock_delete(rwlock_t **rw) {
	
    // if pointer is NULL, return
    if (rw == NULL || *rw == NULL)
        return;

    // free memory for mutex and conditional variable
    pthread_mutex_destroy(&((*rw)->mtx));
    pthread_cond_destroy(&((*rw)->cond));

    // free pointer
    // rest of memory should be freed dynamically
    free(*rw);
    // initialize to null so hanging ptr isn't lost
    *rw = NULL;
}

void reader_lock(rwlock_t *rw) {

    pthread_mutex_lock(&(rw->mtx)); // lock 
    rw->waiting_readers++; // increment # of waiting readers

    // while there are writers or if N_WAY priority and there are waiting writers and the number of current readers is greater than n, wait
    while (rw->writers > 0 || (rw->p == N_WAY && rw->waiting_writers > 0 && rw->readers >= rw->n)) {
        pthread_cond_wait(&(rw->cond), &(rw->mtx)); // wait to be signalled to let go (maintaining order)
    }
	// enter critical section
    rw->waiting_readers--; // decrease the number of waiting readers
    rw->readers++; // go
	//unlock
    pthread_mutex_unlock(&(rw->mtx));
}

void reader_unlock(rwlock_t *rw) {
	// lock to protect inside	
    pthread_mutex_lock(&(rw->mtx));
    // decrease # of readers
    rw->readers--;

    if (rw->readers == 0) { // if the number of readers is 0, broadcast so writers/readers can go depending on priority
        pthread_cond_broadcast(&(rw->cond));
    }
    pthread_mutex_unlock(&(rw->mtx));
}

void writer_lock(rwlock_t *rw) {
	// lock to protect inside
    pthread_mutex_lock(&(rw->mtx));
    rw->waiting_writers++; // increment number of waiting writers
    while (rw->writers > 0 || rw->readers > 0) { // while there is any writer or number of readers is greater than 0
        pthread_cond_wait(&(rw->cond), &(rw->mtx)); // wait!! (to be signalled)
    }

    rw->waiting_writers--; // enter critical section
    rw->writers = 1;
    pthread_mutex_unlock(&(rw->mtx));
}

void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&(rw->mtx)); // protect inside
    rw->writers = 0; // set writers equal to 0
    pthread_cond_broadcast(&(rw->cond)); // broadcast so waiting readers or another writer can go depending on priority
    pthread_mutex_unlock(&(rw->mtx)); // release lock
}
