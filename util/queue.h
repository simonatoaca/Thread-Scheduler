#ifndef QUEUE_H_
#define QUEUE_H_

#include "linked_list.h"

typedef struct {
	linked_list_t *list;
} queue_t;

queue_t *
q_create();

unsigned int
q_get_size(queue_t *q);

unsigned int
q_is_empty(queue_t *q);

void *
q_front(queue_t *q);

void *
q_dequeue(queue_t *q);

void
q_enqueue(queue_t *q, void *new_data);

void
q_clear(queue_t *q);

void
q_free(queue_t *q, void(*free_data)(ll_node_t *));

#endif /* QUEUE_H_ */
