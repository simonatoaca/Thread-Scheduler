#ifndef LIST_H_
#define LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ll_node_t {
    void* info;
    struct ll_node_t* next;
} ll_node_t;

typedef struct {
    ll_node_t* head;
} linked_list_t;

/*
	@brief creates an empty linked list
    @return return the newly created list
*/
linked_list_t *ll_create();

/*
	@brief adds a new node to the start of the list
    @param list - the list
	@param info - the info to be added as a new node
    @return 1 if the node didn't already exist, 0 otherwise
*/
int ll_add_node(linked_list_t *list, void *info);

/*
	@brief removes <node_name> from a list
    @param list - the list
	@param node_name - the name of the node to be removed
    @return the removed node, so it is freed
*/
ll_node_t *ll_remove_node(linked_list_t *list);

int ll_is_empty(linked_list_t *list);

/*
	@brief frees the list
    @param list - the list
	@param free_data - custom free function for the nodes
*/
void ll_free(linked_list_t *list, void(*free_data)(ll_node_t *));
#endif
