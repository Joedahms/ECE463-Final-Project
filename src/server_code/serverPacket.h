#ifndef SERVER_PACKET_H
#define SERVER_PACKET_H

#include <stdbool.h>

void handlePacket(
    char*, int, struct sockaddr_in, struct ConnectedClient*, struct Resource*, bool);
void handleConnectionPacket(
    struct sockaddr_in, struct ConnectedClient*, struct Resource*, char*, bool);
void handleStatusPacket(struct sockaddr_in, struct ConnectedClient*);
int handleResourcePacket(int, struct sockaddr_in, struct Resource*, bool);
void handleClientInfoPacket(
    int, struct sockaddr_in, struct ConnectedClient*, struct Resource*, char*, bool);

#endif
