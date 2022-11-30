#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#define MAX_STRING_SIZE	256

#include "linked_list.h"
#include "queue.h"
#define MAX_STRING_SIZE	256
#define MIN(x, y) ((x) < (y) ? (x) : (y))

queue_t *q_create(unsigned int data_size)
{
	queue_t *q = malloc(sizeof(*q));
	DIE(!q, "Failed queue allocation\n");
	q->list = ll_create(data_size);
	if (!q->list) {
		fprintf(stderr, "Failed queue allocation.\n");
		free(q);
		return NULL;
	}

	return q;
}

unsigned int q_get_size(queue_t *q)
{
	if (!q->list)
		return 0;

	unsigned int q_len = 0;
	ll_node_t *curr_node = q->list->head;
	while (curr_node) {
		curr_node = curr_node->next;
		q_len++;
	}

	return q_len;
}

unsigned int q_is_empty(queue_t *q)
{
	if (!q_get_size(q))
		return 1;
	return 0;
}

void *q_front(queue_t *q)
{
	DIE(!q, "The queue does not exist\n");
	unsigned int q_size = q_get_size(q);
	DIE(!q_size, "The list is empty\n");

	ll_node_t *curr_node = q->list->head;
	for (int i = 0; i < q_size - 1; i++) {
		curr_node = curr_node->next;
	}
	
	return curr_node->data;
}

void q_dequeue(queue_t *q)
{
	DIE(!q, "The queue does not exist\n");
	if (q_get_size(q)) {
		ll_node_t *curr_node = ll_remove_nth_node(q->list, q->list->size - 1);
		free(curr_node->data);
		free(curr_node);
	}
}

void q_enqueue(queue_t *q, void *new_data)
{
	DIE(!q, "The queue does not exist\n");
	if (!new_data) {
		fprintf(stderr, "The data is NULL\n");
	}
	ll_add_nth_node(q->list, 0, new_data);
}

void q_clear(queue_t *q)
{
	DIE(!q, "The queue does not exist\n");

	unsigned int st_len = q_get_size(q);
	for (int i = 0; i < st_len; i++) {
		q_dequeue(q);
	}
}

void q_free(queue_t *q)
{
	DIE(!q, "The queue does not exist\n");
	q_clear(q);
	ll_free(&q->list);
	free(q);
}