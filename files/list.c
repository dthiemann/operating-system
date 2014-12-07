// Author: Dylan Thiemann
// Class: Operating Systems
// Assignment: 1
// Problem: 4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		// For strcmp
#include "list.h"

// Creates a new slist_t structure with a given character string
slist_t *init() {
    slist_t *list = (slist_t *) malloc(sizeof(slist_t));
	list->head = NULL;
	list->tail = NULL;

    return list;
}

// Returns the number of elements in the linked list
int get_num_elements(slist_t *list) {
	if (list->head == NULL) {
		return 0;
	} else {
		int count = 0;
		list_item_t *currentLink = list->head;
		while(currentLink->next != NULL) {
			count++;
			currentLink = currentLink->next;
		}
		count++;
		return count;
	}
}

/* Get list_item given index number */
list_item_t *get_list_item_with_handler(slist_t *list, int handler) {
    list_item_t *current = list->head;
    while (current != NULL) {
        if (current->value == handler) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Adds an element to a current list, *list, wtih value, *string
void add(slist_t *list, list_item_t *list_item) {
	list_item_t *last = list->tail;
	// The list is empty
	if (last == NULL) {
		list_item->next = NULL;
		list_item->prev = NULL;
		list->head = list_item;
		list->tail = list_item;
	} else {
		list_item->next = NULL;
		list_item->prev = last;
		last->next = list_item;
		list->tail = list_item;
	}
}

// Clears the list and frees up all allocated space taken from nodes
void empty(slist_t *list) {
	if (list->head == NULL) return;
	else {
		list_item_t *currentLink = list->head;
		list_item_t *temp;
		while(currentLink->next != NULL) {
			temp = currentLink;
			currentLink = currentLink->next;
			free(temp);
		}
		free(currentLink);
		list->head = NULL;
		list->tail = NULL;
	}
}

// Sequentially prints each node from head --> tail
void print(slist_t *list) {

	int numElements = get_num_elements(list); 
	if (numElements == 0) {
		return;
	}

	list_item_t *currentLink;
	currentLink = list->head;
	// Traverse list by assigning currentLink it's "next" element untill we reach NULL
	while (currentLink->next != NULL) {
		printf("%d\n", currentLink->value);
		currentLink = currentLink->next;
	}

	printf("%d\n", currentLink->value);
}

// Sorts entire nodes by pointer adjustment
void bubblesort(slist_t *list) {

	int N = get_num_elements(list);
	if (N == 0) {
		return;
	}
	list_item_t *currentLink = list->head;

	// Outer oop of bubblesort
	while (N != 0) {
		//inner loop of bubble sort
		for (int i = 0; i < N; i++) {
			if (currentLink->next == NULL) {
				break;
			}

			// If x > y, swap them
			if (currentLink->value > currentLink->next->value) {
			//if (currentLink->value[0] > currentLink->next->value[0]) {

				// Temp variable for the node we are swapping with currentLink
				list_item_t *nextOne = currentLink->next;
				
				if (get_num_elements(list) == 2) {

					list->head = nextOne;
					list->tail = currentLink;

					nextOne->next = currentLink;
					currentLink->prev = nextOne;

					currentLink->next = NULL;
					nextOne->prev = NULL;
				} 
				// If we are at the head element
				else if (currentLink->prev == NULL) {

					list->head = nextOne;

					currentLink->next = nextOne->next;
					nextOne->next->prev = currentLink;

					nextOne->next = currentLink;
					currentLink->prev = nextOne;

					nextOne->prev = NULL;

				} 
				// or we are at the tail element
				else if (nextOne->next == NULL) {

					list->tail=currentLink;

					currentLink->next = NULL;
					nextOne->prev = currentLink->prev;
					currentLink->prev->next = nextOne;

					nextOne->next = currentLink;
					currentLink->prev = nextOne;

				// If the two elements are surrounded by other nodes (i.e. not head and/or tail)
				} else {

					currentLink->prev->next = nextOne;
					nextOne->prev = currentLink->prev;

					nextOne->next->prev = currentLink;
					currentLink->next = nextOne->next;

					currentLink->prev = nextOne;
					nextOne->next = currentLink;

				}
			// Otherwise proceed
			} else {
				currentLink = currentLink->next;
			}
		}
		currentLink = list->head;
		N--;
	}
}