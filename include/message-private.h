/**
 * Grupo 32
 * 
 * Sofia Reis - 59880
 * Nuno Martins - 59863
 * Renan Silva - 59802
 */

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

int write_all(int sock, void *buf, int len);

int read_all(int sock, void *buf, int len);

#endif