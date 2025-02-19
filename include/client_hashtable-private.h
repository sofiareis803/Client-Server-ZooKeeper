/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _CLIENT_HASHTABLE_PRIVATE_H
#define _CLIENT_HASHTABLE_PRIVATE_H

#include "client_stub-private.h"

char* trim(char* str);

void terminate(int signum);

int command_cicle();

// callback function para a ligação com o zookeeper
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

// callback function para quando houver uma ligação na chain
static void chain_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

#endif
