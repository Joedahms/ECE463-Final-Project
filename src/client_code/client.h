#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>

void shutdownClient();
void receiveMessageFromServer();
int getAvailableResources(char*, const char*);

int sendConnectionPacket(struct sockaddr_in, struct sockaddr_in, bool);
void sendResourcePacket(struct sockaddr_in, bool);

void handlePacket(struct sockaddr_in, bool);
void handleResourcePacket(char*, bool);
void handleStatusPacket(struct sockaddr_in, bool);

void setUsername(char*);
struct sockaddr_in getTcpSocketInfo();

#endif
