#include "prio_queue.h"

prio_queue_t *pq_create(unsigned int data_size, unsigned int max_prio) {
    prio_queue_t *pq = malloc(sizeof(*pq));
    DIE(!pq, "Failed priority queue allocation");

    pq->max_prio = max_prio;
    pq->queues = malloc(max_prio * sizeof(*pq->queues));

    if (!pq->queues) {
        free(pq);
        return NULL;
    }

    for (unsigned int prio = 0; prio < max_prio; prio++) {
        pq->queues[prio] = q_create(data_size);
        if (!pq->queues[prio]) {
            for (unsigned int i = 0; i < prio; i++) {
                free(pq->queues[i]);
            }
            free(pq->queues);
            free(pq);
            return NULL;
        }
    }

    return pq;
}

unsigned int pq_get_size(prio_queue_t *pq) {
    unsigned int size = 0;
    for (unsigned int i; i < pq->max_prio; i++) {
        size += q_get_size(pq->queues[i]);
    }

    return size;
}

unsigned int pq_is_empty(prio_queue_t *pq) {
    if (!pq_get_size(pq))
        return 1;
    return 0;
}

void *pq_front(prio_queue_t *pq) {
    DIE(!pq, "The priority queue does not exist\n");
    for (unsigned int prio = pq->max_prio - 1; prio >= 0; prio--) {
        if (!q_is_empty(pq->queues[prio])) {
            return q_front(pq->queues[prio]);
        }
    }

    return NULL;
}

void pq_dequeue(prio_queue_t *pq) {
    DIE(!pq, "The priority queue does not exist\n");
	for (unsigned int prio = pq->max_prio - 1; prio >= 0; prio--) {
        if (!q_is_empty(pq->queues[prio])) {
            q_dequeue(pq->queues[prio]);
            return;
        }
    }
}

void pq_enqueue(prio_queue_t *pq, void *new_data, int prio) {
    DIE(!pq, "The priority queue does not exist\n");
	if (!new_data) {
		fprintf(stderr, "The data is NULL\n");
        return;
	}

    q_enqueue(pq->queues[prio - 1], new_data);
}

void pq_clear(prio_queue_t *pq) {
    DIE(!pq, "The priority queue does not exist\n");
    for (int i = 0; i < pq->max_prio; i++) {
        q_clear(pq->queues[i]);
    }
}

void pq_free(prio_queue_t *pq) {
    if (!pq) return;
    for (int i = 0; i < pq->max_prio; i++) {
        q_free(pq->queues[i]);
    }
    free(pq->queues);
    free(pq);
}