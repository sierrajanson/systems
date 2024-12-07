#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>

typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node_t;

typedef struct queue {
    queue_node_t *head;
    queue_node_t *tail; // New field for O(1) push
    pthread_mutex_t mtx;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int cap;
    int count;
} queue_t;

queue_t *queue_new(int size) {
    printf("Queue created\n");
    assert(size > 0 && "Queue size must be greater than 0");

    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (q == NULL)
        return NULL;

    q->cap = size;
    q->count = 0;
    q->head = NULL;
    q->tail = NULL;

    pthread_mutex_init(&(q->mtx), NULL);
    pthread_cond_init(&(q->not_empty), NULL);
    pthread_cond_init(&(q->not_full), NULL);

    return q;
}

void queue_delete(queue_t **q) {
    if (q == NULL || *q == NULL)
        return;

    pthread_mutex_destroy(&((*q)->mtx));
    pthread_cond_destroy(&((*q)->not_empty));
    pthread_cond_destroy(&((*q)->not_full));

    queue_node_t *current = (*q)->head;
    while (current != NULL) {
        queue_node_t *next = current->next;
        free(current);
        current = next;
    }

    free(*q);
    *q = NULL;
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL || elem == NULL)
        return false;

    queue_node_t *new_node = (queue_node_t *) malloc(sizeof(queue_node_t));
    if (new_node == NULL)
        return false;

    new_node->data = elem;
    new_node->next = NULL;

    pthread_mutex_lock(&(q->mtx));

    // Wait until there's space in the queue
    while (q->count >= q->cap) {
        pthread_cond_wait(&(q->not_full), &(q->mtx));
    }

    if (q->tail == NULL) {
        // Queue is empty
        q->head = new_node;
        q->tail = new_node;
    } else {
        // Append to the end
        q->tail->next = new_node;
        q->tail = new_node;
    }

    q->count++;
    pthread_cond_signal(&(q->not_empty)); // Signal that the queue is no longer empty

    pthread_mutex_unlock(&(q->mtx));
    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL || elem == NULL)
        return false;

    pthread_mutex_lock(&(q->mtx));

    // Wait until there's an element in the queue
    while (q->count == 0) {
        pthread_cond_wait(&(q->not_empty), &(q->mtx));
    }

    queue_node_t *to_remove = q->head;
    *elem = to_remove->data;

    q->head = to_remove->next;
    if (q->head == NULL) {
        // Queue is now empty
        q->tail = NULL;
    }

    free(to_remove);
    q->count--;
    pthread_cond_signal(&(q->not_full)); // Signal that the queue is no longer full

    pthread_mutex_unlock(&(q->mtx));
    return true;
}
