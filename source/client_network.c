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
#include "client_network.h"
#include "client_stub-private.h"
#include "message-private.h"

int network_connect(struct rtable_t *rtable){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0){
        return -1;
    }

    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error creating the TCP socket\n");
        return -1;
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(rtable->server_port);
    if (inet_pton(AF_INET, rtable->server_address, &server.sin_addr) < 1) {
        printf("Error converting the IP address\n");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Error connecting to the server\n");
        close(sockfd);
        return -1;
    }

    rtable->sockfd = sockfd;
    return 0;
}

MessageT *network_send_receive(struct rtable_t *rtable, MessageT *msg){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0 ||
       msg == NULL){
        return NULL;
    }

    int sockfd = rtable->sockfd;
    size_t out_len = message_t__get_packed_size(msg);
    void* out_buf = (void*)malloc(out_len);
    
    if (message_t__pack(msg, out_buf) != out_len){
        free(out_buf);
        return NULL;
    }

    short net_out_len = htons(out_len);

    if(write_all(sockfd, &net_out_len, sizeof(short)) != sizeof(short)){
        free(out_buf);
        return NULL;
    }

    if(write_all(sockfd, out_buf, out_len) != out_len){
        free(out_buf);
        return NULL;
    }

    free(out_buf);
    short in_len;
    
    if(read_all(sockfd, &in_len, sizeof(short)) != sizeof(short)){
        return NULL;
    }
    
    in_len = ntohs(in_len);
    void* in_buf = (void*)malloc(in_len);
    
    if(read_all(sockfd, in_buf, in_len) != in_len){
        free(in_buf);
        return NULL;
    }

    MessageT* des_msg = message_t__unpack(NULL, in_len, in_buf);
    free(in_buf);

    return des_msg;
}

int network_close(struct rtable_t *rtable){
    if(rtable == NULL || 
       rtable->server_address == NULL || 
       rtable->server_port < 0){
        return -1;
    }

    return close(rtable->sockfd) == 0 ? 0 : -1;
}
