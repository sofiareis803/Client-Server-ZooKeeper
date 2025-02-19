/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _SERVER_NETWORK_PRIVATE_H
#define _SERVER_NETWORK_PRIVATE_H

#include "table.h"
#include "server_network.h"

#define MAX_THREADS 100

// struct que define os parametros de uma thread
struct thread_parameters {
	struct table_t *table;
	int client_socket;
};

// funcao que será executada por cada thread
void* thread_for_client(void* params);

// funcao responsavel por terminar e fazer join de todas
// as threads que ainda nao foram finalizadas por pedido do 
// cliente, nomeadamente no caso de SIGINT no servidor
int teminate_threads();

#endif
