#ifndef TCP_H
#define TCP_H

#include <stdbool.h>

int setupTcpSocket(struct sockaddr_in);

long int tcpSendBytes(int, const char*, unsigned long int, uint8_t);
long int tcpReceiveBytes(int, char*, long unsigned int, uint8_t);

void tcpSendFile(int, char*, bool);
long int tcpReceiveFile(int, char*, bool);

struct sockaddr_in getTcpSocketInfo(int);

int checkTcpSocket(int, struct sockaddr_in*, uint8_t);
int tcpConnect(const char*, int, struct sockaddr*, socklen_t);

#endif
