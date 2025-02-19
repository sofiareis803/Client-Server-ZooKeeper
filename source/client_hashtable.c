/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <zookeeper/zookeeper.h>
#include <poll.h>

#include "client_hashtable-private.h"
#include "client_stub.h"
#include "stats.h"
#include "zk_utils.h"

static char *host_port;
static zhandle_t *zh;
static int is_connected;
char head_id[PATH_LEN] = "";
char tail_id[PATH_LEN] = "";
struct rtable_t* head_rtable = NULL;
struct rtable_t* tail_rtable = NULL;

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}

static void chain_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
	zoo_string* children_list =	(zoo_string *)malloc(sizeof(zoo_string));
    
	if (state == ZOO_CONNECTED_STATE) {
		if (type == ZOO_CHILD_EVENT) {
 		    if (ZOK != zoo_wget_children(zh, CHAIN_PATH, chain_watcher, watcher_ctx, children_list)) {
 				fprintf(stderr, "Error setting watch at %s!\n", CHAIN_PATH);
                terminate(1);
 			}

            // se não houver servidores ligados
            if(children_list->count == 0) {
                printf("###\nNo server available, terminating...\n");
                fflush(stdout);

                free_children_list(children_list);
                terminate(0);
            } 
            // se houver uma alteração nos filhos de chain
            else {
                char** head_and_tail_ids = get_head_and_tail_ids(children_list);
                char* new_head_id = head_and_tail_ids[0];
                char* new_tail_id = head_and_tail_ids[1];

                // se houver um novo servidor head
                if(strcmp(new_head_id, head_id) != 0){
                    strcpy(head_id, new_head_id);

                    if(rtable_disconnect(head_rtable) == -1) {
                        printf("Error disconnecting to head server\n");
                        terminate(0);
                    }

                    char* ip_port_head = get_node_content(zh, head_id);

                    head_rtable = rtable_connect(ip_port_head);
                    free(ip_port_head);

                    if(head_rtable == NULL) {
                        printf("Error connecting to head server\n");
                        terminate(0);
                    }

                    printf(" ### \nhead_server was changed (new id = %s).\n", head_id);
                    printf("Command: ");
                    fflush(stdout);
                }

                // se houver um novo servidor tail
                if(strcmp(new_tail_id, tail_id) != 0){
                    strcpy(tail_id, new_tail_id);
                    
                    if(rtable_disconnect(tail_rtable) == -1) {
                        printf("Error disconnecting to tail server\n");
                        terminate(0);
                    }

                    char* ip_port_tail = get_node_content(zh, tail_id);

                    tail_rtable = rtable_connect(ip_port_tail);
                    free(ip_port_tail);
                    
                    if(tail_rtable == NULL) {
                        printf("Error connecting to tail server\n");
                        terminate(0);
                    }

                    printf(" ### \ntail_server was changed (new id = %s).\n", tail_id);
                    printf("Command: ");
                    fflush(stdout);
                }

                free_head_and_tail_ids(head_and_tail_ids);
                free_children_list(children_list);
            }
        } 
    }
}

char* trim(char* str) {
    while (*str == ' ') {
        str++;
    }

    if (*str == '\0') {
        return str; 
    }

    char* end = str + strlen(str) - 1;
    while (end > str && *end == ' ') {
        end--;
    }

    *(end + 1) = '\0';

    return str;
}

void terminate(int signum) {
    is_connected = 0;

    int fecho1 = rtable_disconnect(head_rtable);
    int fecho2 = rtable_disconnect(tail_rtable);

    if (fecho1 == -1 || fecho2 == -1) {
        printf("Error in rtable_disconnect\n");
    }

    if(ZOK != zookeeper_close(zh)){
        printf("Error closing the zookeeper handler\n");    
    }

    printf("\nConnection ended! Bye bye\n");
    exit(0);
}

int command_cicle() {
    char input[1024];
    const char delimeter1[] = " \n";
    const char delimeter2[] = "\n";

    int printed = 0;
    struct pollfd pfd;
    pfd.fd = fileno(stdin);
    pfd.events = POLLIN;

    while (is_connected) {
        if(!printed){
            printf("Command: ");
            fflush(stdout);
            printed = 1;
        }

        int poll_res = poll(&pfd, 1, 1000);

        if (poll_res < 0) {
            perror("Error in poll()");
            continue;
        } else if (poll_res == 0) {
            continue;
        }

        if (pfd.revents & POLLIN) {
            if(fgets(input, 1024, stdin) == NULL) {
                printf("Error reading the input\n");
                continue;
            }

            printed = 0;

            char *command = strtok(input, delimeter1);
            if (command == NULL) {
                printf("Invalid command\n");
                continue;
            }

            if(strcmp(command, "put") == 0 || strcmp(command, "p") == 0) {
                char* key = strtok(NULL, delimeter1);
                char* value = strtok(NULL, delimeter2);
                if(key == NULL || value == NULL) {
                    printf("Error: Invalid arguments. Usage: put <key> <value>\n");
                    continue;
                }

                key = strdup(key);
                value = strdup(value);

                if(key == NULL || value == NULL) {
                    printf("Error: Memory allocation failed.\n");
                    continue;
                }

                struct block_t* block = block_create(strlen(value), value);
                if(block == NULL) {
                    printf("Error: it was not possible to create the 'value'.\n");
                    continue;
                }

                struct entry_t* entry = entry_create(key, block);
                if(entry == NULL) {
                    printf("Error: it was not possible to create the 'entry'.\n");
                    block_destroy(block);
                    continue;
                }

                int answer = rtable_put(head_rtable, entry);
                entry_destroy(entry);

                if(answer != 0) {
                    printf("Error in rtable_put\n");
                }
            }
            
            else if(strcmp(command, "get") == 0 || strcmp(command, "g") == 0){
                char* key = strtok(NULL, delimeter2);

                if(key == NULL) {
                    printf("Error: Invalid arguments. Usage: get <key>\n");
                    continue;
                }

                key = trim(key);

                struct block_t* answer_block = rtable_get(tail_rtable, key);

                if(answer_block == NULL) {
                    printf("Error in rtable_get or key not in table\n");
                } else {
                    char output[answer_block->datasize + 1];
                    memcpy(output, answer_block->data, answer_block->datasize);
                    output[answer_block->datasize] = '\0';
                    printf("%s\n", output);
                    block_destroy(answer_block);
                }
            }
            
            else if(strcmp(command, "del") == 0 || strcmp(command, "d") == 0){
                char* key = strtok(NULL, delimeter2);

                key = trim(key);            

                if(key == NULL) {
                    printf("Error: Invalid arguments. Usage: del <key>\n");
                    continue;
                }

                if(rtable_del(head_rtable, key) != 0) {
                    printf("Error in rtable_del or key not in table\n");
                }
            }

            else if(strcmp(command, "size") == 0 || strcmp(command, "s") == 0){
                int size = rtable_size(tail_rtable);

                if(size == -1) {
                    printf("Error in rtable_size\n");
                } else {
                    printf("Table size: %d\n", size);
                }
            }
            
            else if(strcmp(command, "getkeys") == 0 || strcmp(command, "k") == 0){
                char** keys = rtable_get_keys(tail_rtable);

                if(keys == NULL) {
                    printf("Error in rtable_get_keys\n");
                } else {
                    int i = 0;
                    while(keys[i] != NULL) {
                        printf("%s\n", keys[i]);
                        i++;
                    }
                    rtable_free_keys(keys);
                }
            }

            else if(strcmp(command, "gettable") == 0 || strcmp(command, "t") == 0){
                struct entry_t** entries = rtable_get_table(tail_rtable);

                if(entries == NULL) {
                    printf("Error in rtable_get_table\n");
                } else {
                    int i = 0;
                    while(entries[i] != NULL) {
                        char output[entries[i]->value->datasize + 1];
                        memcpy(output, entries[i]->value->data, entries[i]->value->datasize);
                        output[entries[i]->value->datasize] = '\0';
                        printf("%s :: %s\n", entries[i]->key, output);
                        i++;
                    }
                    rtable_free_entries(entries);
                }
            }

            else if(strcmp(command, "quit") == 0 || strcmp(command, "q") == 0){
                terminate(0);
            }

            else if(strcmp(command, "stats") == 0) {
                struct statistics_t* stats = rtable_stats(tail_rtable);

                if(stats == NULL) {
                    printf("Error in rtable_get_stats\n");
                } else {
                    printf("Number of operations: %d\n", stats->operations_counter);
                    printf("Processing time: %d\n", stats->processing_time);
                    printf("Number of connected clients: %d\n", stats->connected_clients);
                    free(stats);
                }
            }
            
            else {
                printf("Invalid command.\n");
                printf("Usage: stats | p[ut] <key> <value> | g[et] <key> | d[el] <key> | s[ize] | [get]k[eys] | [get]t[able] | q[uit]\n");
            }
        }
    }
    return 0;
}


int main(int argc, char *argv[]) { 
    signal(SIGPIPE, SIG_IGN);

    if(argc != 2) {
        printf("Usage: %s <ip-zookeeper>:<port-zookeeper>\n", argv[0]);
        return -1;
    }

    host_port = strdup(argv[1]);
    zh = zookeeper_init(host_port, connection_watcher, 2000, 0, 0, 0);
    if (zh == NULL) {
		fprintf(stderr,"Error connecting to ZooKeeper server[%d]!\n", errno);
		exit(EXIT_FAILURE);
	}
    free(host_port);
    
    sleep(1);
    printf("\n\n------------------------------------------------------------------------------------------\n\n\n");
    printf("Connecting with zookeeper...\n");
    sleep(1); 
    printf("Connection with zookeeper established.\n");
    
    zoo_string* children_list =	(zoo_string *)malloc(sizeof(zoo_string));
    if (ZOK != zoo_wget_children(zh, CHAIN_PATH, chain_watcher, "watcher_ctx", children_list)) {
        fprintf(stderr, "Error setting watch at %s!\n", CHAIN_PATH);
        zookeeper_close(zh);
        return -1;
    }

    // se não houver servidores ligados
    if(children_list->count == 0) {
        free_children_list(children_list);
        zookeeper_close(zh);
        printf("No server available, terminating...\n");
        return 0;
    }

    char** head_and_tail_ids = get_head_and_tail_ids(children_list);

    strcpy(head_id, head_and_tail_ids[0]);
    strcpy(tail_id, head_and_tail_ids[1]);

    free_head_and_tail_ids(head_and_tail_ids);

    char* ip_port_head = get_node_content(zh, head_id);
    char* ip_port_tail = get_node_content(zh, tail_id);

    head_rtable = rtable_connect(ip_port_head);
    if (head_rtable == NULL) {
        printf("Error connecting to head server\n");
        return -1; 
    }
    printf("Connection with head_server established (id = %s).\n", head_id);

    tail_rtable = rtable_connect(ip_port_tail);
    if (tail_rtable == NULL) {
        printf("Error connecting to tail server\n");
        return -1; 
    }
    printf("Connection with tail_server established (id = %s).\n", tail_id);
    
    free(ip_port_head);
    free(ip_port_tail);
    free_children_list(children_list);

    signal(SIGINT, terminate);
    return command_cicle();
}
