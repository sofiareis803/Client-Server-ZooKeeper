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
#include "message-private.h"

int write_all(int sock, void *buf, int len){
    int l = len;
    void* p = buf;
    while(l > 0) {
        int res = write(sock, p, l); 
        
        if(res <= 0) {
            return -1;
        }
        
        p += res;
        l -= res;
    }
    return len;
}


int read_all(int sock, void *buf, int len){
    int l = len;
    void* p = buf;
    while(l > 0) {
        int res = read(sock, p, l); 
        
        if(res <= 0) {
            return -1;
        }
        
        p += res;
        l -= res;
    }
    return len;
}
