#ifndef CLIENT_H
#define CLIENT_H

#define MAX_CONNECTED_CLIENTS 100

#include <stdbool.h>
#include <sys/socket.h>

struct ConnectedClient {
  struct sockaddr_in socketUdpAddress;
  struct sockaddr_in socketTcpAddress;
  int parentToChildPipe[2];
  int childToParentPipe[2];
};

void getUserInput(char*);
void handleUserInput(char*, struct sockaddr_in, bool);

void shutdownClient();
void receiveMessageFromServer();
int getAvailableResources(char*, const char*);

void setUsername(char*);

int findEmptyConnectedClient(struct ConnectedClient*, bool);
#endif
