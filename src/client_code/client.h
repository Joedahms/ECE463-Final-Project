#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <sys/socket.h>

void getUserInput(char*);
int handleUserInput(char*, struct sockaddr_in, bool);

void shutdownClient();
void receiveMessageFromServer();
int getAvailableResources(char*, const char*);

void setUsername(char*);

#endif
