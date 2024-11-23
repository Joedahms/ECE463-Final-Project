#ifndef SERVER_H
#define SERVER_H

// Microseconds
#define STATUS_SEND_INTERVAL 3000000

// Maximum number of clients that can be connected to the server
#define MAX_CONNECTED_CLIENTS 100

#include <stdbool.h>

// Data about a client connected to the server
// Could improve by changing to a linked list
struct ConnectedClient {
  char username[MAX_USERNAME];
  struct sockaddr_in socketUdpAddress;
  struct sockaddr_in socketTcpAddress;
  bool status;          // If the client is connected or not
  bool requestedStatus; // If the server has sent a request asking if the client
                        // is still connnected
};

void* checkClientStatus(void*);
void shutdownServer();
int findEmptyConnectedClient(bool);
void printAllConnectedClients();
void addResourcesToDirectory(char*, long unsigned int, char*, bool);

#endif
