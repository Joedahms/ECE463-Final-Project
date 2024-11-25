#ifndef CLIENT_PACKET_H
#define CLIENT_PACKET_H

#include <stdbool.h>

void handlePacket(char*, int, int, struct sockaddr_in, bool);

void sendResourcePacket(int, struct sockaddr_in, bool);
void handleResourcePacket(char*, bool);

int sendConnectionPacket(int, struct sockaddr_in, struct sockaddr_in, bool);
void handleStatusPacket(int, struct sockaddr_in, bool);

void sendTcpInfoPacket(int, struct sockaddr_in, char*, bool);
void handleTcpInfoPacket(int, int, char*, bool);

void sendFileReqPacket(int, int, struct sockaddr_in, char*, bool);
void handleFileReqPacket(int, char*, bool);

#endif
