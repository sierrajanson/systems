/**
 * @File rwlock.h
 *
 * The header file that you need to implement for assignment 3.
 *
 * @author Sierra Janson
 */

#include <stdint.h>


#include <stdlib.h>
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

} rwlock_t;


/** @brief Dynamically allocates and initializes a new rwlock with
 *         priority p, and, if using N_WAY priority, n.
 *
 *  @param The priority of the rwlock
 *
 *  @param The n value, if using N_WAY priority
 *
 *  @return a pointer to a new rwlock_t
 */

rwlock_t *rwlock_new(PRIORITY p, uint32_t n){
	printf("hello from diff file!\n");
	rwlock_t *rw = (rwlock_t *)malloc(sizeof(rwlock_t));
	if (rw ==NULL) return NULL;
	rw->n = n;
	rw->p = p;
	return rw;
}

/** @brief Delete your rwlock and free all of its memory.
 *
 *  @param rw the rwlock to be deleted.  Note, you should assign the
 *  passed in pointer to NULL when returning (i.e., you should set *rw
 *  = NULL after deallocation).
 *
 */
void rwlock_delete(rwlock_t **rw){
	printf("goodbye\n");
	if (rw == NULL || rw == NULL) return;
	free(*rw);
	*rw = NULL;
}

/** @brief acquire rw for reading
 *
 */

void reader_lock(rwlock_t *rw){
	free(rw);
}

/** @brief release rw for reading--you can assume that the thread
 * releasing the lock has *already* acquired it for reading.
 *
 */
void reader_unlock(rwlock_t *rw){
	free(rw);

}

/** @brief acquire rw for writing
 *
 */
void writer_lock(rwlock_t *rw){
	free(rw);

}
/** @brief release rw for writing--you can assume that the thread
 * releasing the lock has *already* acquired it for writing.
 *
 */
void writer_unlock(rwlock_t *rw){
	free(rw);
}
