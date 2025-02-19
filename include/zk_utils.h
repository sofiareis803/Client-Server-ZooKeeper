/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _ZK_UTILS_H
#define _ZK_UTILS_H

#include <zookeeper/zookeeper.h>

#define ZDATALEN 1024 * 1024
#define CHAIN_PATH "/chain"
#define NODE_PATH "/chain/node"
#define NODE_LEN 1024
#define PATH_LEN 128
#define ADDRESS_MAX_LEN 22 // "255.255.255.255:65535"
#define LOCAL_IP "127.0.0.1"
typedef struct String_vector zoo_string;

// função para obter o conteudo de um node no zookeeper
char* get_node_content(zhandle_t* zh, char* node_id);

// função para libertar a memória alocada para children_list
void free_children_list(zoo_string* children_list);

char** get_head_and_tail_ids(zoo_string* children_list);

void free_head_and_tail_ids(char** head_and_tail);

#endif 
