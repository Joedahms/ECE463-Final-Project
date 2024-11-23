#ifndef PACKET_H
#define PACKET_H

#define MAX_PACKET       220
#define NUM_PACKET_TYPES 5
#define MAX_PACKET_TYPE  20
#define MAX_DATA         200

#include <netdb.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "network_node.h"

struct PacketDelimiters {
  unsigned int fieldLength;
  char field[1];
  long unsigned int subfieldLength;
  char subfield[1];
  unsigned int endLength;
  char end[9];
};

struct PacketFields {
  char type[MAX_PACKET_TYPE];
  char data[MAX_DATA];
};

int getPacketType(char*, bool);
void buildPacket(char*, struct PacketFields, bool);

int readPacket(char*, struct PacketFields*, bool);
char* readPacketField(char*, char*, bool);
char* readPacketSubfield(char*, char*, bool);

void sendUdpPacket(int, struct sockaddr_in, struct PacketFields, bool);

#endif
