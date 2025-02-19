/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H /* Módulo LIST_PRIVATE */

#include "entry.h"

struct node_t {
	struct entry_t *entry;
	struct node_t  *next; 
};

struct list_t {
	int size;
	struct node_t *head;
};


#endif
