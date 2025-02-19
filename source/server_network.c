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

#include "server_network.h"
#include "server_network-private.h"
#include "server_skeleton.h"
#include "server_skeleton-private.h"
#include "message-private.h"
#include "inet.h"

pthread_t threads[MAX_THREADS];
struct thread_parameters all_parameters[MAX_THREADS];
int thread_counter = 0;

int server_network_init(short port){
    if(port < 0){
        return -1;
    }

    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("Error creating socket");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("Error binding");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 0) < 0){
        perror("Error listening");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int network_main_loop(int listening_socket, struct table_t *table){
    if(table == NULL){
        return -1;
    }
    
    int client_socket;
    struct sockaddr_in client;
    socklen_t size_client = sizeof(struct sockaddr_in);

    while ((client_socket = accept(listening_socket,(struct sockaddr *) &client, &size_client)) != -1) {
        all_parameters[thread_counter].client_socket = client_socket;
        all_parameters[thread_counter].table = table;
        
        if (pthread_create(&threads[thread_counter], NULL, &thread_for_client, (void *)&all_parameters[thread_counter]) != 0){
			printf("\nThread %d não criada.\n", thread_counter);
			exit(EXIT_FAILURE);
		}
        thread_counter++;
    }

    return -1;
}

void* thread_for_client(void* params) {
    inc_connected_clients();
    
    struct thread_parameters *tp = (struct thread_parameters*) params;
    MessageT* msg;

    while((msg = network_receive(tp->client_socket)) != NULL){
        if(invoke(msg, tp->table) == -1){
            perror("Error invoking table method");
            break;
        }

        if(network_send(tp->client_socket, msg) == -1){
            perror("Error serializing or sending message to client");
            break;
        }
    }

    server_network_close(tp->client_socket);

    dec_connected_clients();

    return NULL;
}

MessageT *network_receive(int client_socket){
    uint8_t buf[MAX_MSG];
    uint16_t len;

    if(read_all(client_socket, &len, sizeof(uint16_t)) != sizeof(uint16_t)){
        return NULL;
    }

    len = ntohs(len);
    
    if(read_all(client_socket, buf, len) != len){
        return NULL;
	}

    MessageT* msg = message_t__unpack(NULL, len, buf);
    
    if(msg == NULL){
        server_network_close(client_socket);
        message_t__free_unpacked(msg, NULL);
        close(client_socket);
        return NULL;
    }

    return msg;
}

int network_send(int client_socket, MessageT *msg){
    if(msg == NULL){
        server_network_close(client_socket);
        return -1;
    }

    size_t len = message_t__get_packed_size(msg);
    void* buf = (void*)malloc(len);

    if(buf == NULL){
        server_network_close(client_socket);
        return -1;
    }

    if(message_t__pack(msg, buf) != len){
        free(buf);
        server_network_close(client_socket);
        return -1;
    }

    len = htons(len);

    if(write_all(client_socket, &len, sizeof(short)) != sizeof(short)){
        server_network_close(client_socket);
        return -1;
    }

    len = ntohs(len);

    if(write_all(client_socket, buf, len) != len){
        server_network_close(client_socket);
        return -1;
	}

    free(buf);
    message_t__free_unpacked(msg, NULL);

    return 0;
}

int teminate_threads(){
    int result = 0;

    for (int i = 0; i < thread_counter; i++){
        shutdown(all_parameters[i].client_socket, SHUT_RDWR);
    }

    for (int i = 0; i < thread_counter; i++){
        if (pthread_join(threads[i], NULL) != 0){
            result = -1;
        }
    }

    return result;
}

int server_network_close(int socket){
    return close(socket) < 0 ? -1 : 0;
}
