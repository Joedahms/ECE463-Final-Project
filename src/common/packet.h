#ifndef PACKET_H
#define PACKET_H

#define MAX_PACKET 220
#define NUM_PACKET_TYPES 3
#define MAX_PACKET_TYPE 20
#define MAX_DATA 200

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdbool.h>

#include "network_node.h"

static const char* packetTypes[NUM_PACKET_TYPES] = {"connection", "status", "resource"};

struct PacketDelimiters {
  int fieldLength;
  char field[1];
  int subfieldLength;
  char subfield[1];
  int endLength;
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

#endif
