/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H /* Módulo TABLE_PRIVATE */

#include "list.h"

struct table_t {
	struct list_t **lists;
	int size;
};

/* Função que calcula o índice da lista a partir da chave
 */
int hash_code(char *key, int n);

#endif
