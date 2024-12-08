/**
 * @File queue.c
 *
 * @author Sierra Janson
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "rwlock.h"

// headers i addeed
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

typedef struct queue queue_t;

typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node_t;

typedef struct queue {
    queue_node_t *head;
    pthread_mutex_t mtx;
    int cap;
    int count; // atomic_int
} queue_t;

queue_t *queue_new(int size) {
    // initialize a new queue
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (q == NULL)
        return NULL;
    assert(size != 0 && "why would you init a queue of size 0 bozo");
    q->cap = size;
    q->count = 0;
    pthread_mutex_init(&(q->mtx), NULL);
    return q;
}

void queue_delete(queue_t **q) {
    // delete queue
    if (q == NULL)
        return;
    if (*q == NULL)
        return; // don't free I think
    queue_node_t *next = (*q)->head;
    queue_node_t *current = (*q)->head;
    pthread_mutex_destroy(&((*q)->mtx));
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(*q);
    *q = NULL;
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL)
        return false; // elem == NULL
    queue_node_t *new_node = (queue_node_t *) malloc(sizeof(queue_node_t));
    if (new_node == NULL)
        return false;

    // if successful, initalize values
    new_node->next = NULL;
    new_node->data = elem;

    // check if at capacity
    pthread_mutex_lock(&(q->mtx));
    while (q->cap <= q->count) {
        pthread_mutex_unlock(&(q->mtx));
        pthread_mutex_lock(&(q->mtx));
    }
    if (q->head == NULL) {
        q->head = new_node;
        q->count++;
        pthread_mutex_unlock(&(q->mtx));
        return true;
    }
    queue_node_t *start = q->head;

    while (start->next != NULL) {
        start = start->next;
    }
    start->next = new_node;
    q->count++;
    pthread_mutex_unlock(&(q->mtx));
    return true;
}
bool queue_pop(queue_t *q, void **elem) {
    // create multithreaded implementation
    if (q == NULL)
        return false;
    pthread_mutex_lock(&(q->mtx));
    while (q->count == 0) {
        pthread_mutex_unlock(&(q->mtx));
        pthread_mutex_lock(&(q->mtx));
    }
    queue_node_t *to_remove = q->head;
    q->head = to_remove->next; // set next head
    *elem = to_remove->data; // give elem value
    to_remove->next = NULL; // set pointer to NULL
    free(to_remove);
    q->count--;
    pthread_mutex_unlock(&(q->mtx));
    return true;
}

bool queue_is_empty(queue_t *q) {
    pthread_mutex_lock(&(q->mtx));
    if (q->count == 0) {
        pthread_mutex_unlock(&(q->mtx));
        return true;
    } else {
        pthread_mutex_unlock(&(q->mtx));
        return false;
    }
}
