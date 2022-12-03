#ifndef PRIO_QUEUE_H_
#define PRIO_QUEUE_H_

#include <stdio.h>
#include <stdlib.h>

#include "queue.h"
#include "so_scheduler.h"
#include "utils.h"

typedef struct {
    unsigned int max_prio;
    queue_t **queues;
} prio_queue_t;

prio_queue_t *
pq_create(unsigned int max_prio);

unsigned int
pq_get_size(prio_queue_t *pq);

unsigned int
pq_is_empty(prio_queue_t *pq);

void *
pq_front(prio_queue_t *pq);

void *
pq_dequeue(prio_queue_t *pq);

void
pq_enqueue(prio_queue_t *pq, void *new_data, int prio);

void
pq_clear(prio_queue_t *pq);

void pq_free(prio_queue_t *pq, void(*free_data)(ll_node_t *));

#endif