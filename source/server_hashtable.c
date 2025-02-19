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
#include <signal.h>
#include <errno.h>
#include <zookeeper/zookeeper.h>

#include "server_hashtable-private.h"
#include "server_network-private.h"
#include "server_skeleton-private.h"
#include "inet.h"
#include "client_stub-private.h"
#include "zk_utils.h"

static char *host_port;
static zhandle_t *zh;
static int is_connected;
char *my_port;

int server_socket;
struct table_t *table;

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}

void terminate(int signum){
    is_connected = 0;

    printf("\nTerminating server\n");
    
    if(teminate_threads() == -1){
        perror("Error on a thread join\n");
    }

    if(destroy_locks() == -1){
        perror("Error destroying locks\n");
    }
    
    if(server_skeleton_destroy(table) == -1) {
        perror("Error destroying table\n");
    }

    if(server_network_close(server_socket) == -1) {
        perror("Error closing socket\n");
    }

    zookeeper_close(zh);

    printf("All done!\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    
    if(argc != 4) {
        printf("Usage: %s <ip-zookeeper>:<port-zookeeper> <port> <n_lists>\n", argv[0]);
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
    
    if(is_connected){

        // criar node /chain se ainda nao existir (no caso do servidor for o primeiro)
        if (ZNONODE == zoo_exists(zh, CHAIN_PATH, 0, NULL)) {
            if (ZOK == zoo_create(zh, CHAIN_PATH, NULL, -1, & ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
                fprintf(stderr, "%s created!\n", CHAIN_PATH);
            } else {
                fprintf(stderr,"Error Creating %s!\n", CHAIN_PATH);
                exit(EXIT_FAILURE);
            } 
		}

        my_port = strdup(argv[2]);
        server_socket = server_network_init((short) atoi(my_port));
        if(server_socket == -1) {
            printf("Error initializing socket\n");
            return -1;
        }

        if(atoi(argv[3]) <= 0) {
            printf("Number of lists must be greater than 0\n");
            return -1;
        }

        table = server_skeleton_init(atoi(argv[3]));
        if(table == NULL) {
            printf("Error creating table\n");
            return -1;
        }

        if(insert_node_sync_table(zh, my_port, table) != 0){
            free(my_port);
            terminate(-1);
        }

        free(my_port);

        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("Erro ao configurar SO_REUSEADDR");
            close(server_socket);
            return -1;
        }

        signal(SIGINT, terminate);
        printf("Server ready.\n");
        if(network_main_loop(server_socket, table) == -1) {
            printf("Error in network main loop\n");
            terminate(SIGINT);
        }

        if(server_network_close(server_socket) == -1) {
            printf("Error closing socket\n");
            return -1;
        }
    }

    return 0;
}