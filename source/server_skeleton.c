/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <zookeeper/zookeeper.h>

#include "server_skeleton.h"
#include "server_skeleton-private.h"
#include "server_hashtable-private.h"
#include "table-private.h"
#include "inet.h"
#include "stats.h"
#include "sys/time.h"

#include "client_stub.h"
#include "client_stub-private.h"
#include "zk_utils.h"

struct statistics_t stats;
pthread_rwlock_t rwlock_table = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t rwlock_stats = PTHREAD_RWLOCK_INITIALIZER; 

struct rtable_t* previous_server = NULL;
struct rtable_t* next_server = NULL;
char next_server_id[PATH_LEN] = "";

char my_node[PATH_LEN] = "";
int just_created = 1;

void chain_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    if(just_created) {
        just_created = 0;
        zoo_wget_children(wzh, CHAIN_PATH, chain_watcher, watcher_ctx, NULL);
    } else {
        zoo_string* children_list =	(zoo_string *)malloc(sizeof(zoo_string));
    
        if (state == ZOO_CONNECTED_STATE) {
            if (type == ZOO_CHILD_EVENT) {
                if (ZOK != zoo_wget_children(wzh, CHAIN_PATH, chain_watcher, watcher_ctx, children_list)) {
                    fprintf(stderr, "Error setting watch at %s!\n", CHAIN_PATH);
                }

                // encontrar o server com id a seguir ao meu para saber
                // se é diferente do next server ou se é um novo tail
                int next = -1;
                for (int i = 0; i < children_list->count; i++)  {
                    if(strcmp(children_list->data[i], (my_node+strlen(CHAIN_PATH)+1)) > 0 && 
                      (next == -1 || strcmp(children_list->data[i], children_list->data[next]) < 0)){
                        next = i;
                    }
                }

                // se não foi encontrado nenhum server com id maior que o meu
                if(next == -1){
                    // se antes eu tinha um next_server
                    if(next_server != NULL){
                        rtable_disconnect(next_server);
                        next_server = NULL;
                        if(strcmp(next_server_id, "") != 0){
                            printf("I dont have a next_server anymore :( - Now I'm the tail_server\n");
                            strcpy(next_server_id, "");
                        }
                    }
                    fflush(stdout);
                } 
                // se foi encontrado um server com id maior que o meu, atualizar se antes eu não 
                // tinha um next_server ou se esse server é diferente do meu antigo next_server
                else if(next_server == NULL || strcmp(next_server_id, children_list->data[next]) != 0){
                    if(next_server != NULL){
                        rtable_disconnect(next_server);
                    }
                    
                    strcpy(next_server_id, children_list->data[next]);

                    char* ip_port_next = get_node_content(wzh, next_server_id);

                    if(next_server == NULL)
                        printf("Now I have a next_server (id = %s) - I'm no longer the tail_server\n", next_server_id);
                    else
                        printf("My next_server has changed (new id = %s) !!!\n", next_server_id);
                    
                    next_server = rtable_connect(ip_port_next);
                    free(ip_port_next);
                    
                    if (next_server == NULL) {
                        printf("Error connecting to next node server\n"); 
                        terminate(-1);    
                    }
                }
            } 
        }
        free_children_list(children_list);
    }
}

int insert_node_sync_table(zhandle_t *zh, char *port, struct table_t *table){
    char ip_port[ADDRESS_MAX_LEN] = "";
    strcat(ip_port, LOCAL_IP);
    strcat(ip_port, ":");
    strcat(ip_port, port);

    zoo_string* children_list =	(zoo_string *)malloc(sizeof(zoo_string));
    
    if (ZOK != zoo_wget_children(zh, CHAIN_PATH, chain_watcher, "watcher_ctx", children_list)) {
        fprintf(stderr, "Error setting watch at %s!\n", CHAIN_PATH);
        terminate(-1);
    }
    
    // se houver pelo menos um node no zookeeper, então obter o último e sincronizar a tabela com ele
    if(children_list->count > 0){
        int tail_index = 0;

        for (int i = 1; i < children_list->count; i++)  {
            if(strcmp(children_list->data[i], children_list->data[tail_index]) > 0){
                tail_index = i;
            }
        }

        char* ip_port_tail = get_node_content(zh, children_list->data[tail_index]);

        previous_server = rtable_connect(ip_port_tail);
        free(ip_port_tail);

        if (previous_server == NULL) {
            printf("Error connecting to tail server\n");
            return -1; 
        }

        if(sync_local_table(table) != 0){
            return -1;
        }

        rtable_disconnect(previous_server);
    }

    // criar node do servidor no zookeeper
    if (ZOK != zoo_create(zh, NODE_PATH, ip_port, (strlen(ip_port) + 1), & ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, my_node, PATH_LEN)) {
        fprintf(stderr, "Error creating znode from path %s!\n", NODE_PATH);
        exit(EXIT_FAILURE);
    }

    if(children_list->count > 0)
        printf("Node created on zookeeper (with id = %s) and table synchronized with others.\n\n", (my_node+strlen(CHAIN_PATH)+1));
    else
        printf("Node created on zookeeper (with id = %s).\n\n", (my_node+strlen(CHAIN_PATH)+1));
    
    free_children_list(children_list);

    return 0;
}

int sync_local_table(struct table_t *table){
    if(table == NULL){
        return -1;
    }

    struct entry_t** entries = rtable_get_table(previous_server);

    if(entries == NULL) {
        return -1;
    }

    int i = 0;
    while(entries[i] != NULL) {
        if(table_put(table, entries[i]->key, entries[i]->value) != 0){
            return -1;
        }
        i++;
    }
    rtable_free_entries(entries);
    return 0;
}

int lock_writer_table(){ return pthread_rwlock_wrlock(&rwlock_table); }
int lock_reader_table(){ return pthread_rwlock_rdlock(&rwlock_table); }
int unlock_table()     { return pthread_rwlock_unlock(&rwlock_table); }
int lock_writer_stats(){ return pthread_rwlock_wrlock(&rwlock_stats); }
int lock_reader_stats(){ return pthread_rwlock_rdlock(&rwlock_stats); }
int unlock_stats()     { return pthread_rwlock_unlock(&rwlock_stats); }

int inc_connected_clients(){
    if(lock_writer_stats() != 0) return -1;
    stats.connected_clients++;
    if(unlock_stats() != 0) return -1;
    return 0;
}

int dec_connected_clients(){
    if(lock_writer_stats() != 0) return -1;
    stats.connected_clients--;
    if(unlock_stats() != 0) return -1;
    return 0;
}

int destroy_locks(){
    int result = 0;

    if(pthread_rwlock_destroy(&rwlock_table) != 0) result = -1;
    if(pthread_rwlock_destroy(&rwlock_stats) != 0) result = -1;

    return result;
}

struct table_t *server_skeleton_init(int n_lists){
    if(n_lists <= 0){
        return NULL;
    }

    return table_create(n_lists);
}

int server_skeleton_destroy(struct table_t *table){
    if(table == NULL || 
       table->size <= 0 ||
       table->lists == NULL){
        return -1;
    }

    return table_destroy(table);
}

int invoke(MessageT *msg, struct table_t *table){
    if(table == NULL || 
       table->size <= 0 ||
       table->lists == NULL ||
       msg == NULL){
        return -1;
    }

    struct timeval start, end;
    long elapsed;
    gettimeofday(&start, NULL);
    
    switch (msg->opcode){
        case MESSAGE_T__OPCODE__OP_PUT:{
            int res_put = 0;

            void* data_copy = (void *)malloc(msg->entry->value.len);
            memcpy(data_copy, msg->entry->value.data, msg->entry->value.len);
            struct block_t* value = block_create((int)(msg->entry->value.len), data_copy);

            if(value == NULL){
                res_put = -1;
            } else {
                if(lock_writer_table() != 0) return -1;
                res_put = table_put(table, msg->entry->key, value);
                if(next_server != NULL){
                    int result;
                    struct entry_t *e = entry_create(strdup(msg->entry->key), value);
                    if((result = rtable_put(next_server, e)) != 0){
                        // em caso de erro... fazer oq?
                    }
                    entry_destroy(e);
                } else {
                    block_destroy(value);
                }
                if(unlock_table() != 0) return -1;
            }

            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            if(res_put == -1)
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            else
                msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
            break;
        }

        case MESSAGE_T__OPCODE__OP_GET:{
            if(lock_reader_table() != 0) return -1;
            struct block_t* res_get = table_get(table, msg->key);
            if(unlock_table() != 0) return -1;

            if(res_get == NULL){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            } else {
                msg->value.len = res_get->datasize;
                void* data = malloc(res_get->datasize);
                memcpy(data, res_get->data, res_get->datasize);
                msg->value.data = data;
                block_destroy(res_get);
                msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
            }
            break;
        }

        case MESSAGE_T__OPCODE__OP_DEL:{
            if(lock_writer_table() != 0) return -1;
            int res_del = table_remove(table, msg->key);
            if(next_server != NULL && rtable_del(next_server, msg->key) != 0){
                // em caso de erro... fazer oq?
            }
            if(unlock_table() != 0) return -1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            if(res_del == -1 || res_del == 1)
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            else
                msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
            break;
        }

        case MESSAGE_T__OPCODE__OP_SIZE:{
            if(lock_reader_table() != 0) return -1;
            int res_size = table_size(table);
            if(unlock_table() != 0) return -1;

            if(res_size == -1){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
                msg->result = res_size;
            }

            break;
        }

        case MESSAGE_T__OPCODE__OP_GETKEYS:{
            if(lock_reader_table() != 0) return -1;
            int num_keys = table_size(table);
            char** res_getkeys = table_get_keys(table);
            if(unlock_table() != 0) return -1;

            if(res_getkeys == NULL){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
                msg->keys = res_getkeys;
                msg->n_keys = num_keys;
            }
            break;
        }

        case MESSAGE_T__OPCODE__OP_GETTABLE:{
            if(lock_reader_table() != 0) return -1;
            int n_entries = table_size(table);
            char** keys = table_get_keys(table);

            if(keys == NULL){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                if(unlock_table() != 0) return -1;
                break;
            }

            EntryT **entries = (EntryT**)malloc(sizeof(EntryT*) * (n_entries + 1));
            
            if(entries == NULL){
                table_free_keys(keys);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                if(unlock_table() != 0) return -1;
                break;
            }

            entries[n_entries] = NULL;
            
            for(int i = 0; i < n_entries; i++){
                struct block_t* block = table_get(table, keys[i]);
        
                if (block == NULL) {
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    table_free_keys(keys);
                    
                    for (int x = 0; x < i; x++){
                        entry_t__free_unpacked(entries[x], NULL);
                    }
                    free(entries);
                    if(unlock_table() != 0) return -1;
                    break;
                }
                
                entries[i] = (EntryT*)malloc(sizeof(EntryT));
                
                if(entries[i] == NULL){
                    for (int x = 0; x < i; x++){
                        entry_t__free_unpacked(entries[x], NULL);
                    }
                    free(entries);

                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    table_free_keys(keys);
                    if(unlock_table() != 0) return -1;
                    break;
                }

                entry_t__init(entries[i]);
                entries[i]->key = strdup(keys[i]);  
                entries[i]->value.len = (size_t) block->datasize;
                entries[i]->value.data = malloc(block->datasize);

                if(entries[i]->value.data == NULL){
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    table_free_keys(keys);
                    for (int x = 0; x < i; x++){
                        entry_t__free_unpacked(entries[x], NULL);
                    }
                    free(entries);
                    if(unlock_table() != 0) return -1;
                    break;
                }

                memcpy(entries[i]->value.data, block->data, block->datasize);
                block_destroy(block);
            }

            table_free_keys(keys);
            msg->opcode = MESSAGE_T__OPCODE__OP_GETTABLE + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;
            msg->entries = entries;
            msg->n_entries = n_entries;
            if(unlock_table() != 0) return -1;
            break;
        }

        case MESSAGE_T__OPCODE__OP_STATS:{
            msg->stats = (StatsT*)malloc(sizeof(StatsT));

            if(msg->stats != NULL){
                stats_t__init(msg->stats);
                if(lock_reader_stats() != 0) return -1;
                msg->stats->connected_clients = stats.connected_clients;
                msg->stats->operations_counter = stats.operations_counter;
                msg->stats->processing_time = stats.processing_time;
                if(unlock_stats() != 0) return -1;
                msg->opcode = MESSAGE_T__OPCODE__OP_STATS + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }
            return 0;
        }

        default:
            break;
    }

    gettimeofday(&end, NULL);
    elapsed = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);

    if(lock_writer_stats() != 0) return -1;
    stats.processing_time += elapsed;
    stats.operations_counter++;
    if(unlock_stats() != 0) return -1;
    
    return 0;
}
