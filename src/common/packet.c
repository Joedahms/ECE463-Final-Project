#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>

#include "packet.h"

struct PacketDelimiters packetDelimiters = {
  1,
  "$",          // field
  1,
  "&",          // subfield
  9,
  "endpacket"   // end
};

/*
  * Purpose: Check if at the end of the data field of a packet
  * Input: 
  * - The packet to check if at the end of
  * Output:
  * 0: Not at the end of data
  * 1: At the end of data
*/
static bool checkEnd(char* packet) {
  if (strncmp(packet, packetDelimiters.end, packetDelimiters.endLength) == 0) {
    return true;
  }
  return false;
}

/*
  * Purpose: Take in the type of the packet as a string and return an integer associated
  * with that packet type.
  * Input:
  * - The type of the packet as a string
  * Output: Integer representing the type of the packet
  * -1 = invalid
  * 0 = connection
  * 1 = status
  * 2 = resource
  * Notes: Might look at using an enum for packet type
*/
int getPacketType(char* packetType, bool debugFlag) {
  int returnVal = -1;
  int i = 0;
  for (i = 0; i < NUM_PACKET_TYPES; i++) {
    if (strcmp(packetType, packetTypes[i]) == 0) {
      returnVal = i;
    }
  }
  return returnVal;
}

/*
  * Purpose: Using a passed in struct containing the fields of a packet, build a
  * packet in the form of a string.
  * Input: 
  * - String where the completed packet is to go
  * - struct containing the information the packet is to contain
  * - Debug flag
  * Output: None
*/
void buildPacket(char* builtPacket, struct PacketFields packetFields, bool debugFlag) {
  // Type
  strcpy(builtPacket, packetFields.type);
  strncat(builtPacket, packetDelimiters.field, packetDelimiters.fieldLength);
  if (debugFlag) {
    printf("Packet after adding type: %s\n", builtPacket);
  }

  // Data
  strcat(builtPacket, packetFields.data);
  strncat(builtPacket, packetDelimiters.field, packetDelimiters.fieldLength);
  if (debugFlag) {
    printf("Packet after adding data: %s\n", builtPacket);
  }

  // End
  strncat(builtPacket, packetDelimiters.end, packetDelimiters.endLength);
  if (debugFlag) {
    printf("Entire packet: %s\n", builtPacket);
  }
}

/*
  * Purpose: Read a packet that has been sent by another network node. Take the packet in as a string and extract
  * its fields into the the the packet fields data type
  * Input:
  * - String containing the packet that was sent
  * - Struct to put the read out fields into
  * Output:
  * - 0
*/
int readPacket(char* packetToBeRead, struct PacketFields* packetFields, bool debugFlag) {
  char* packet = calloc(1, MAX_PACKET);
  char* packetStart = packet;
  strcpy(packet, packetToBeRead);

  // Type. Advance after reading type (set packet to return val) so that data can be read.
  packet = readPacketField(packet, packetFields->type, debugFlag);

  // Data
  readPacketField(packet, packetFields->data, debugFlag);

  free(packetStart);
  return 0;
}

/*
  * Purpose: Read a single field from a packet. Reads the packet string until the field
  * delimiter is hit. Field is returned in second argument. 
  * Input: 
  * - The packet to read the field from
  * - Memory allocated for the field
  * - Debug flag
  * Output: Packet after reading a field from it
*/
char* readPacketField(char* packet, char* field, bool debugFlag) {
  if (debugFlag) {
    printf("Reading field from packet: %s\n", packet);
  }

  while(strncmp(packet, packetDelimiters.field, packetDelimiters.fieldLength) != 0) {
    strncat(field, packet, 1);
    packet++;
  }
  packet += packetDelimiters.fieldLength;

  if (debugFlag) {
    printf("Field read: %s\n", field);
  }
  return packet;
}

/*
  * Purpose: Read a subfield from a packet.
  * Input: 
  * - Field to read the subfield from
  * - Memory allocated for the subfield
  *
  * Output: Field after reading a subfield from it
*/
char* readPacketSubfield(char* field, char* subfield, bool debugFlag) {
  if (debugFlag) {
    printf("Reading subfield from packet field: %s\n", field);
  }

  while(strncmp(field, packetDelimiters.subfield, packetDelimiters.subfieldLength) != 0) {
    strncat(subfield, field, 1);
    field++;
  }
  field += packetDelimiters.subfieldLength;

  if (debugFlag) {
    printf("Subfield read: %s\n", subfield);
  }
  return field;
}
