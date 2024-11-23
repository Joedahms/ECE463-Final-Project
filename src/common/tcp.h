#ifndef TCP_H
#define TCP_H

#include <stdbool.h>

long int tcpSendBytes(int, const char*, unsigned long int, uint8_t);
long int tcpReceiveBytes(int, char*, long unsigned int, uint8_t);

void tcpSendFile(int, char*, bool);
long int tcpReceiveFile(int, char*, bool);

#endif
