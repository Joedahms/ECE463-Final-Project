#ifndef UDP_H
#define UDP_H

#include <stdbool.h>
#include <sys/types.h>

#include "network_node.h"

void sendUdpMessage(int, struct sockaddr_in, char*, bool);
int setupUdpSocket(struct sockaddr_in, bool);
bool checkUdpSocket(int, struct sockaddr_in*, char*, bool);

#endif
