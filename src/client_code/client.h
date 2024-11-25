#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <sys/socket.h>

struct SendFileThreadInfo {
  int socketDescriptor;
  struct sockaddr_in fileRequester;
  char filename[MAX_FILENAME];
};

struct ReceiveFileThreadInfo {
  int socketDescriptor;
  char filename[MAX_FILENAME];
  bool debugFlag;
};

void getUserInput(char*);
int handleUserInput(char*, struct sockaddr_in, bool);

void shutdownClient();
void receiveMessageFromServer();
int getAvailableResources(char*, const char*);

void* sendFile(void*);
void* receiveFile(void*);

void setUsername(char*);
struct sockaddr_in getTcpSocketInfo();

#endif
