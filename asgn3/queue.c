/**
 * @File queue.h
 *
 * The header file that you need to implement for assignment 3.
 *
 * @author Andrew Quinn
 */

//#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>


#include "rwlock.h"
// added after
#include <stdio.h>
#include <assert.h>
//#include <atomic.h>
/** @struct queue_t
 *
 *  @brief This typedef renames the struct queue.  Your `c` file
 *  should define the variables that you need for your queue.
 */
typedef struct queue queue_t;

typedef struct queue_node {
	void *data;
	struct queue_node *next;
} queue_node_t;


typedef struct queue {
	queue_node_t *head;
	int cap;
	int count; // atomic_int
	//queue_node_t *tail;
} queue_t;


/** @brief Dynamically allocates and initializes a new queue with a
 *         maximum size, size
 *
 *  @param size the maximum size of the queue
 *
 *  @return a pointer to a new queue_t
 */
queue_t *queue_new(int size){
	queue_t *q = (queue_t *)malloc(sizeof(queue_t));
	if (q == NULL) return NULL;
	assert(size != 0 && "why would you init a queue of size 0 bozo");
	q->cap = size;
	q->count = 0;
	return q;
}

/** @brief Delete your queue and free all of its memory.
 *
 *  @param q the queue to be deleted.  Note, you should assign the
 *  passed in pointer to NULL when returning (i.e., you should set
 *  *q = NULL after deallocation).
 *
 */
void queue_delete(queue_t **q){
	if (q == NULL) return;
	if (*q == NULL) return; // don't free I think
	queue_node_t *next = (*q)->head;
	queue_node_t *current = (*q)->head;

	while (current != NULL){
		next = current->next;
		free(current);
		current = next;
	}
	free(*q);
	*q = NULL;

}

/** @brief push an element onto a queue
 *
 *  @param q the queue to push an element into.
 *
 *  @param elem th element to add to the queue
 *
 *  @return A bool indicating success or failure.  Note, the function
 *          should succeed unless the q parameter is NULL.
 */
bool queue_push(queue_t *q, void *elem){
	if (q == NULL) return false; // elem == NULL
	// allocate memory for node
	
	queue_node_t *new_node = (queue_node_t *)malloc(sizeof(queue_node_t));
	if (new_node == NULL) return false;
	
	// if successful, initalize values
	new_node->next = NULL;
	new_node->data = elem;

	// check if at capacity
	while (q->cap <= q->count) {}; // wait
	// if there's no head:	
	if (q->head == NULL){
		q->head = new_node;
		return true;
	}	
	queue_node_t *start = q->head;

	while (start->next != NULL){
		start = start->next;
	}
	start->next = new_node;
	return true;
}

/** @brief pop an element from a queue.
 *
 *  @param q the queue to pop an element from.
 *
 *  @param elem a place to assign the poped element.
 *
 *  @return A bool indicating success or failure.  Note, the function
 *          should succeed unless the q parameter is NULL.
 */
bool queue_pop(queue_t *q, void **elem){
	// create multithreaded implementation
	if (q == NULL) return false;

	// if empty --> block
	if (q->head == NULL) return false;
	// if empty should elem be NULL?

	while (q->count == 0){}; // use lock i guess
	queue_node_t *to_remove = q->head;
	q->head = to_remove->next; // set next head
	*elem = to_remove->data; // give elem value
	to_remove->next = NULL; // set pointer to NULL
	free(to_remove);
	return true;

}


int main(void){
	// initialize queue
	printf("hello\n");
	queue_t *q = NULL;
	/*queue_new(5);
	bool res = queue_push(q,(void *)4);
	//res = queue_push(q,(void *)3);

	int value = 0;
	printf("value before: %d\n", value);	
	res = queue_pop(q, (void **)(&value)); 
	res = queue_pop(q, (void **)(&value)); 
	printf("result: %d\n",res);*/
	queue_delete(&q);
	//printf("value now: %d\n", value);	

	return 0;
}
