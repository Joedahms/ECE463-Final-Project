#ifndef CLIENT_PACKET_H
#define CLIENT_PACKET_H

#include <stdbool.h>

void sendResourcePacket(int, struct sockaddr_in, bool);
void handleResourcePacket(char*, bool);

#endif
