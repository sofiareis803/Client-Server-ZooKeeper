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

#include "inet.h"
#include "client_stub-private.h"
#include "client_network.h"
#include "stats.h"

struct rtable_t *rtable_connect(char *address_port){
    if(address_port == NULL){
        return NULL;
    }
    
    char* ip   = strtok(address_port, ":");
    char* port = strtok(NULL, ":");
    
    struct rtable_t* r = (struct rtable_t*)malloc(sizeof(struct rtable_t));
    r->server_address = strdup(ip);
    r->server_port = atoi(port);

    if(network_connect(r) == -1){
        rtable_disconnect(r);
        return NULL;
    }

    return r;
}

int rtable_disconnect(struct rtable_t *rtable){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0){
        return -1;
    }

    if(network_close(rtable) == -1){
        return -1;
    }

    free(rtable->server_address);
    free(rtable);

    return 0;
}

int rtable_put(struct rtable_t *rtable, struct entry_t *entry){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0 ||
       entry == NULL ||
       entry->key == NULL ||
       entry->value == NULL ||
       entry->value->data == NULL){
        return -1;
    }

    MessageT* msg = (MessageT*)malloc(sizeof(MessageT));
    message_t__init(msg);
    msg->opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg->c_type = MESSAGE_T__C_TYPE__CT_ENTRY;
    msg->entry = (EntryT*)malloc(sizeof(EntryT));

    if(msg->entry == NULL){
        free(msg);
        return -1;
    }

    entry_t__init(msg->entry);
    msg->entry->key = strdup(entry->key);
    msg->entry->value.len = entry->value->datasize;
    void* copy = malloc(entry->value->datasize);
    memcpy(copy, entry->value->data, entry->value->datasize);
    msg->entry->value.data = copy;
    
    MessageT* answer_msg;
    answer_msg = network_send_receive(rtable, msg);
    message_t__free_unpacked(msg, NULL);

    if(answer_msg == NULL){
        return -1;
    }

    int result;

    if(answer_msg->opcode == MESSAGE_T__OPCODE__OP_PUT + 1)
        result = 0;
    else
        result = -1;

    message_t__free_unpacked(answer_msg, NULL);
    return result;
}

struct block_t *rtable_get(struct rtable_t *rtable, char *key){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0 ||
       key == NULL){
        return NULL;
    }

    MessageT* msg = (MessageT*)malloc(sizeof(MessageT));
    message_t__init(msg);
    msg->opcode = MESSAGE_T__OPCODE__OP_GET;
    msg->c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg->key = strdup(key);

    MessageT* answer_msg;
    answer_msg = network_send_receive(rtable, msg);
    message_t__free_unpacked(msg, NULL);

    if(answer_msg == NULL){
        return NULL;
    }

    struct block_t* result;

    if(answer_msg->opcode == MESSAGE_T__OPCODE__OP_GET + 1){
        void *data_copy = malloc(answer_msg->value.len);
        memcpy(data_copy, answer_msg->value.data, answer_msg->value.len);
        
        if((result = block_create(answer_msg->value.len, data_copy)) == NULL){
            free(data_copy);
        }
    } else {
        result = NULL;
    }

    message_t__free_unpacked(answer_msg, NULL);
    return result;
}

int rtable_del(struct rtable_t *rtable, char *key){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0 ||
       key == NULL){
        return -1;
    }

    MessageT* msg = (MessageT*)malloc(sizeof(MessageT));
    message_t__init(msg);
    msg->opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg->c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg->key = strdup(key);

    MessageT* answer_msg;
    answer_msg = network_send_receive(rtable, msg);
    message_t__free_unpacked(msg, NULL);

    if(answer_msg == NULL){
        return -1;
    }

    int result;

    if(answer_msg->opcode == MESSAGE_T__OPCODE__OP_DEL + 1)
        result = 0;
    else
        result = -1;

    message_t__free_unpacked(answer_msg, NULL);
    return result;
}

int rtable_size(struct rtable_t *rtable){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0){
        return -1;
    }

    MessageT* msg = (MessageT*)malloc(sizeof(MessageT));
    message_t__init(msg);
    msg->opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* answer_msg;
    answer_msg = network_send_receive(rtable, msg);
    message_t__free_unpacked(msg, NULL);

    if(answer_msg == NULL){
        return -1;
    }

    int result;

    if(answer_msg->opcode == MESSAGE_T__OPCODE__OP_SIZE + 1)
        result = answer_msg->result;
    else
        result = -1;

    message_t__free_unpacked(answer_msg, NULL);
    return result;
}

char **rtable_get_keys(struct rtable_t *rtable){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0){
        return NULL;
    }

    MessageT* msg = (MessageT*)malloc(sizeof(MessageT));
    message_t__init(msg);
    msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* answer_msg;
    answer_msg = network_send_receive(rtable, msg);
    message_t__free_unpacked(msg, NULL);

    if(answer_msg == NULL){
        return NULL;
    }

    char** keys;
    int n_keys = answer_msg->n_keys;

    if(answer_msg->opcode == MESSAGE_T__OPCODE__OP_GETKEYS + 1) {
        keys = malloc((n_keys + 1) * sizeof(char *));

        if (keys != NULL) {
            keys[n_keys] = NULL;

            for (int i = 0; i < n_keys; i++) {
                keys[i] = strdup(answer_msg->keys[i]);
                
                if (keys[i] == NULL) {
                    for (int x = 0; x < i; x++) {
                        free(keys[x]);
                    }
                    free(keys);
                    keys = NULL;
                    break;
                }
            }
        }
    } else {
        keys = NULL;
    }

    message_t__free_unpacked(answer_msg, NULL);
    return keys;
}

void rtable_free_keys(char **keys){
    if(keys == NULL){
        return;
    }
    
    int i = 0;

    while(keys[i] != NULL){
        free(keys[i]);
        i++;
    }

    free(keys[i]);
    free(keys);
}

struct entry_t **rtable_get_table(struct rtable_t *rtable){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0){
        return NULL;
    }

    MessageT* msg = (MessageT*)malloc(sizeof(MessageT));
    message_t__init(msg);
    msg->opcode = MESSAGE_T__OPCODE__OP_GETTABLE;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* answer_msg;
    answer_msg = network_send_receive(rtable, msg);
    message_t__free_unpacked(msg, NULL);

    if(answer_msg == NULL){
        return NULL;
    }

    struct entry_t** entries;
    int n_entries = answer_msg->n_entries;

    if(answer_msg->opcode == MESSAGE_T__OPCODE__OP_GETTABLE + 1){
        entries = malloc((n_entries + 1) * sizeof(char *));

        if (entries != NULL) {
            entries[n_entries] = NULL;

            for (int i = 0; i < n_entries; i++) {
                int len = answer_msg->entries[i]->value.len;
                void* copy = malloc(len);

                if(copy == NULL){
                    rtable_free_entries(entries);
                    entries = NULL;
                    break;
                }

                memcpy(copy, answer_msg->entries[i]->value.data, len);
                struct block_t* block = block_create(len, copy);

                if(block == NULL){
                    free(copy);
                    rtable_free_entries(entries);
                    entries = NULL;
                    break;
                }

                struct entry_t* entry = entry_create(strdup(answer_msg->entries[i]->key), block);

                if(entry == NULL){
                    block_destroy(block);
                    rtable_free_entries(entries);
                    entries = NULL;
                    break;
                }
                
                entries[i] = entry;
            }
        }
    } else {
        entries = NULL;
    }

    message_t__free_unpacked(answer_msg, NULL);
    return entries;
}

void rtable_free_entries(struct entry_t **entries){
    if(entries == NULL){
        return;
    }
    
    int i = 0;

    while(entries[i] != NULL){
        entry_destroy(entries[i]);
        i++;
    }

    free(entries[i]);
    free(entries);
}

struct statistics_t *rtable_stats(struct rtable_t *rtable){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0){
        return NULL;
    }

    MessageT* msg = (MessageT*)malloc(sizeof(MessageT));
    message_t__init(msg);
    msg->opcode = MESSAGE_T__OPCODE__OP_STATS;
    msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;

    MessageT* answer_msg;
    answer_msg = network_send_receive(rtable, msg);
    message_t__free_unpacked(msg, NULL);

    if(answer_msg == NULL){
        return NULL;
    }

    struct statistics_t* result;

    if(answer_msg->opcode == MESSAGE_T__OPCODE__OP_STATS + 1){
        result = (struct statistics_t*)malloc(sizeof(struct statistics_t));

        if(result != NULL){
            result->operations_counter = answer_msg->stats->operations_counter;
            result->processing_time = answer_msg->stats->processing_time;
            result->connected_clients = answer_msg->stats->connected_clients;
        }
    } else {
        result = NULL;
    }

    message_t__free_unpacked(answer_msg, NULL);
    return result;
}
