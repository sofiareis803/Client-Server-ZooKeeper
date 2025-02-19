/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _SERVER_SKELETON_PRIVATE_H
#define _SERVER_SKELETON_PRIVATE_H

#include <zookeeper/zookeeper.h>
#include "table-private.h"

// função para inserir um novo node no zookeeper que corresponde ao servidor passado como argumento
int insert_node_sync_table(zhandle_t *zh, char *port, struct table_t *table);

// callback function para quando houver alteracao na chain
void chain_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

// função para sincronizar a tabela local com as restantes tabelas do sistema
int sync_local_table(struct table_t *table);

// funções para manipular os locks da tabela
int lock_writer_table();
int lock_reader_table();
int unlock_table();

// funções para manipular os locks dos stats
int lock_writer_stats();
int lock_reader_stats();
int unlock_stats();

// incrementa o numero de clientes conectados com o servidor
int inc_connected_clients();

// decrementa o numero de clientes conectados com o servidor
int dec_connected_clients();

// responsável por destruir os locks
int destroy_locks();

#endif
