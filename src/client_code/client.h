#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>

void shutdownClient(int);
void getUserInput(char*);
void receiveMessageFromServer();
int getAvailableResources(char*, const char*);
int sendConnectionPacket(struct sockaddr_in, bool);
void sendResourcePacket(struct sockaddr_in, bool);
void handleResourcePacket(char*, bool);
void handleStatusPacket(struct sockaddr_in, bool);
void setUsername(char*);

#endif
