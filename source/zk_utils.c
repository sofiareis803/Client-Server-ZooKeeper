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
#include <errno.h>
#include <zookeeper/zookeeper.h>

#include "zk_utils.h"

char* get_node_content(zhandle_t* zh, char* node_id) {
    char node_path[PATH_LEN] = "";
    strcat(node_path, CHAIN_PATH);
    strcat(node_path, "/");
    strcat(node_path, node_id);

    char *ip_port = malloc(ADDRESS_MAX_LEN * sizeof(char));
	int ip_port_len = ADDRESS_MAX_LEN;
    
    zoo_get(zh, node_path, 0, ip_port, &ip_port_len, NULL);
    return ip_port;
}

void free_children_list(zoo_string* children_list) {
    for (int i = 0; i < children_list->count; i++){
        free(children_list->data[i]);
    }
    free(children_list->data);
    free(children_list);
}

char** get_head_and_tail_ids(zoo_string* children_list){
    char** result = (char**)malloc(2 * sizeof(char*));

    int head_index = 0;
    int tail_index = 0;

    for (int i = 1; i < children_list->count; i++)  {
        if(strcmp(children_list->data[i], children_list->data[head_index]) < 0){
            head_index = i;
        }

        else if(strcmp(children_list->data[i], children_list->data[tail_index]) > 0){
            tail_index = i;
        }
    }

    result[0] = strdup(children_list->data[head_index]);
    result[1] = strdup(children_list->data[tail_index]);

    return result;
}

void free_head_and_tail_ids(char** head_and_tail){
    free(head_and_tail[0]);
    free(head_and_tail[1]);
    free(head_and_tail);
}
