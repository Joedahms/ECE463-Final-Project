#ifndef SERVER_H
#define SERVER_H

#define STATUS_SEND_INTERVAL 3000000  // Microseconds
#define MAX_CONNECTED_CLIENTS 100     // Maximum number of clients that can be connected to the server

#include <stdbool.h>

// Data about a client connected to the server
// Could improve by changing to a linked list
struct ConnectedClient {
  char username[MAX_USERNAME];
  struct sockaddr_in socketUdpAddress;
  bool status;          // If the client is connected or not
  bool requestedStatus; // If the server has sent a request asking if the client is still connnected
};

void* checkClientStatus(void*);
void shutdownServer(int);
int findEmptyConnectedClient(bool);
void printAllConnectedClients();
void addResourcesToDirectory(char*, int, char*,  bool);
void handleConnectionPacket(char*, struct sockaddr_in, bool);
void handleStatusPacket(struct sockaddr_in);
int handleResourcePacket(struct sockaddr_in, bool);

#endif
