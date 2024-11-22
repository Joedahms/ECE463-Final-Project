#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <sys/socket.h>

struct SendThreadInfo {
  char filename[MAX_FILENAME];
  struct sockaddr_in fileHost;
};

long int sendBytes(int, const char*, unsigned long int, uint8_t);
long int receiveBytes(int, char*, long unsigned int, uint8_t);
void putCommand(char*, bool);
long int getCommand(char*, bool);

void getUserInput(char*);
int handleUserInput(char*, struct sockaddr_in, bool);

void shutdownClient();
void receiveMessageFromServer();
int getAvailableResources(char*, const char*);

void* sendFile(void*);
void* receiveFile(void*);

// Send packets
int sendConnectionPacket(struct sockaddr_in, struct sockaddr_in, bool);
void sendResourcePacket(struct sockaddr_in, bool);
void sendTcpInfoPacket(struct sockaddr_in, char*, bool);
void sendFileReqPacket(struct sockaddr_in, char*, bool);

// Handle packets
void handlePacket(struct sockaddr_in, bool);
void handleResourcePacket(char*, bool);
void handleStatusPacket(struct sockaddr_in, bool);
void handleTcpInfoPacket(char*, bool);
void handleFileReqPacket(char*, bool);

void setUsername(char*);
struct sockaddr_in getTcpSocketInfo();

#endif
