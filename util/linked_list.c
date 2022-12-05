#include "linked_list.h"

linked_list_t *ll_create() {
	linked_list_t *list = malloc(sizeof(*list));
	if (!list) {
		fprintf(stderr, "List malloc failed");
		return NULL;
	}
	list->head = 0;
	return list;
}

int ll_add_node(linked_list_t *list, void *info) {
	if (!list) {
		fprintf(stderr, "The list is null\n");
		return 0;
	}

	ll_node_t *next = list->head;
	ll_node_t *new = malloc(sizeof(*new));
	new->info = info;
	list->head = new;
	new->next = next;

	return 1;
}

ll_node_t *ll_remove_node(linked_list_t *list) {
	if (!list) {
		fprintf(stderr, "The list is null\n");
		return NULL;
	}

	if (!list->head) {
		return NULL;
	}

	if (!list->head->next) {
		ll_node_t *curr = list->head;
		list->head = NULL;
		return curr;
	}

	ll_node_t *curr = list->head;
	ll_node_t *prev = NULL;

	while (curr->next) {
		prev = curr;
		curr = curr->next;
	}

	prev->next = NULL;
	return curr;
}

int ll_is_empty(linked_list_t *list) {
	if (!list || !list->head)
		return 1;
	return 0;
}

void ll_free(linked_list_t *list, void(*free_data)(ll_node_t *)) {
	if (!list) {
		fprintf(stderr, "The list is null\n");
		return;
	}

	ll_node_t *curr = list->head;
	while (curr) {
		ll_node_t *aux = curr;
		curr = curr->next;
		free_data(aux);
	}

	free(list);
}
