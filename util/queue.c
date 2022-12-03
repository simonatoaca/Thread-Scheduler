#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#define MAX_STRING_SIZE	256

#include "linked_list.h"
#include "queue.h"

queue_t *q_create()
{
	queue_t *q = malloc(sizeof(*q));
	DIE(!q, "Failed queue allocation\n");
	q->list = ll_create();
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
	if (ll_is_empty(q->list))
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
	
	return curr_node->info;
}

void *q_dequeue(queue_t *q)
{
	DIE(!q, "The queue does not exist\n");
	if (q_get_size(q)) {
		ll_node_t *curr_node = ll_remove_node(q->list);
		return curr_node;
	}
	return NULL;
}

void q_enqueue(queue_t *q, void *new_data)
{
	DIE(!q, "The queue does not exist\n");
	if (!new_data) {
		fprintf(stderr, "The data is NULL\n");
	}
	ll_add_node(q->list, new_data);
}

void q_free(queue_t *q, void(*free_data)(ll_node_t *))
{
	DIE(!q, "The queue does not exist\n");
	ll_free(q->list, free_data);
	free(q);
}