/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _SERVER_HASHTABLE_PRIVATE_H
#define _SERVER_HASHTABLE_PRIVATE_H

#include "server_network.h"
#include "server_skeleton.h"
#include "table-private.h"
#include "table.h"

#include <zookeeper/zookeeper.h>

void terminate(int signum);

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

// callback function para a ligação com o zookeeper
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

#endif
