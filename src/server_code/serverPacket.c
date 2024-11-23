#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/packet.h"
#include "resource.h"
#include "server.h"
#include "serverPacket.h"

// packet.h
extern struct PacketDelimiters packetDelimiters;

/*
 * Purpose: When the server recieves a packet, this function determines what to do with
 * it.
 * Input:
 * - The packet to handle
 * - UDP socket descriptor on the server
 * - UDP address structure of client who sent the packet
 * - Clients connected to the server
 * - Available resources in the network
 * - Debug flag
 * Output: None
 */
void handlePacket(char* packet,
                  int udpSocketDescriptor,
                  struct sockaddr_in clientUdpAddress,
                  struct ConnectedClient* connectedClients,
                  struct Resource* headResource,
                  bool debugFlag) {
  if (debugFlag) {
    printf("Packet received\n");
  }

  struct PacketFields packetFields;
  memset(&packetFields, 0, sizeof(packetFields));
  readPacket(packet, &packetFields, debugFlag);
  int packetType = getPacketType(packetFields.type, debugFlag);

  switch (packetType) {
  // Connection packet
  case 0:
    if (debugFlag) {
      printf("Type of packet recieved is connection\n");
    }
    handleConnectionPacket(clientUdpAddress, connectedClients, headResource,
                           packetFields.data, debugFlag);
    break;

  // Status packet
  case 1:
    if (debugFlag) {
      printf("Type of packet received is status\n");
    }
    handleStatusPacket(clientUdpAddress, connectedClients);
    break;

  // Resource packet
  case 2:
    if (debugFlag) {
      printf("Type of packet received is Resource\n");
    }
    handleResourcePacket(udpSocketDescriptor, clientUdpAddress, headResource, debugFlag);
    break;

  // TCP info packet
  case 3:
    if (debugFlag) {
      printf("Type of packet received is tcpinfo\n");
    }
    handleTcpInfoPacket(udpSocketDescriptor, clientUdpAddress, connectedClients,
                        headResource, packetFields.data, debugFlag);
    break;

  default:
  }
  memset(packet, 0, strlen(packet));
}

/*
 * Purpose: When the server receives a connection packet, this function handles
 * the data in that packet. It finds an empty connected client and enters the
 * packet sender's information into that empty spot.
 * Input:
 * - The connection packet that was sent
 * - The address of the client who sent the packet
 * Output: None
 */
void handleConnectionPacket(struct sockaddr_in clientUdpAddress,
                            struct ConnectedClient* connectedClients,
                            struct Resource* headResource,
                            char* packetData,
                            bool debugFlag) {
  int emptyClientIndex                = findEmptyConnectedClient(debugFlag);
  struct ConnectedClient* emptyClient = &connectedClients[emptyClientIndex];

  // Connection info and status
  emptyClient->socketUdpAddress.sin_addr.s_addr = clientUdpAddress.sin_addr.s_addr;
  emptyClient->socketUdpAddress.sin_port        = clientUdpAddress.sin_port;
  emptyClient->status                           = true;

  // Username
  char* username          = calloc(1, MAX_USERNAME);
  char* usernameBeginning = username;
  packetData              = readPacketSubfield(packetData, username, debugFlag);
  strcpy(emptyClient->username, username);

  char* tcpInfo = calloc(1, 64);
  char* end;

  packetData   = readPacketSubfield(packetData, tcpInfo, debugFlag);
  long address = strtol(tcpInfo, &end, 10);
  emptyClient->socketTcpAddress.sin_addr.s_addr = (unsigned int)address;

  memset(tcpInfo, 0, 64);

  packetData = readPacketSubfield(packetData, tcpInfo, debugFlag);
  long port  = strtol(tcpInfo, &end, 10);
  emptyClient->socketTcpAddress.sin_port = (unsigned short)port;

  free(tcpInfo);

  addResourcesToDirectory(packetData, strlen(packetData), username, debugFlag);

  free(usernameBeginning);

  if (debugFlag) {
    printAllConnectedClients();
    printAllResources(headResource);
  }
}

/*
 * Purpose: When the server receives a status packet, this function handles the
 * data in that packet. It loops through all the connected clients and finds the
 * one who sent the status packet. The status of the client who sent the packet
 * is set to indicate that the client is still connected.
 * Input:
 * - The address of the client who sent the status packet
 * Output: None
 */
void handleStatusPacket(struct sockaddr_in clientUdpAddress,
                        struct ConnectedClient* connectedClients) {
  int clientIndex;
  struct ConnectedClient* currentClient;

  for (clientIndex = 0; clientIndex < MAX_CONNECTED_CLIENTS; clientIndex++) {
    currentClient = &connectedClients[clientIndex];

    // Client
    unsigned long currentClientAddress =
        connectedClients[clientIndex].socketUdpAddress.sin_addr.s_addr;
    unsigned short currentClientPort =
        connectedClients[clientIndex].socketUdpAddress.sin_port;

    // Packet sender
    unsigned long incomingAddress = clientUdpAddress.sin_addr.s_addr;
    unsigned short incomingPort   = clientUdpAddress.sin_port;

    // Empty client
    if (currentClientAddress == 0 && currentClientPort == 0) {
      continue;
    }

    // Packet sender is client. They sent a response and are still connected
    if (currentClientAddress == incomingAddress && currentClientPort == incomingPort) {
      currentClient->status = 1;
    }
  }
}

/*
 * Purpose: Servers actions upon receiving a resource packet. When a resource
 * packet is received, it means a client requested the available resources in
 * the network. This function gathers those available resources and sends them
 * to the client that requested them.
 * Input:
 * - Client that inquired about the available resources
 * - Debug flag
 * Output: 0
 */
int handleResourcePacket(int udpSocketDescriptor,
                         struct sockaddr_in clientUdpAddress,
                         struct Resource* headResource,
                         bool debugFlag) {
  struct PacketFields packetFields;
  memset(&packetFields, 0, sizeof(packetFields));
  strcpy(packetFields.type, "resource");

  char* resourceString = calloc(1, MAX_DATA);
  resourceString =
      makeResourceString(resourceString, headResource, packetDelimiters.subfield);
  strcpy(packetFields.data, resourceString);
  free(resourceString);

  char* returnPacket = calloc(1, MAX_PACKET);
  buildPacket(returnPacket, packetFields, debugFlag);
  sendUdpMessage(udpSocketDescriptor, clientUdpAddress, returnPacket, debugFlag);
  free(returnPacket);

  return 0;
}

/*
 * Purpose: When the server recieves a tcpinfo packet, this function is called. If a
 * tcpinfo packet is received by the server it means a client is looking for a file in the
 * network. Find the client that is hosting the file. Send back the filename along with
 * socket address structures containing info about the TCP and UDP socket open on the
 * hosting client.
 * Input:
 * - UDP socket descriptor on the server
 * - UDP socket address of the client who sent the tcpinfo packet
 * - Clients connected to the server
 * - Resources available in the network
 * - Data field of the received tcpinfo packet
 * - Debug flag
 * Output: None
 */
void handleTcpInfoPacket(int udpSocketDescriptor,
                         struct sockaddr_in clientUdpAddress,
                         struct ConnectedClient* connectedClients,
                         struct Resource* headResource,
                         char* packetData,
                         bool debugFlag) {
  struct PacketFields packetFields;
  strcpy(packetFields.type, "tcpinfo");

  char* username = calloc(1, MAX_USERNAME);
  bool fileFound = searchResourcesByFilename(headResource, username, packetData);
  if (fileFound) {
    // Send back tcp info
    int i = 0;
    for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
      if (strcmp(connectedClients[i].username, username) == 0) {
        // Filename
        strcat(packetFields.data, packetData);
        strcat(packetFields.data, packetDelimiters.subfield);

        // UDP address
        char address[64];
        sprintf(address, "%d", connectedClients[i].socketUdpAddress.sin_addr.s_addr);
        strcat(packetFields.data, address);
        strcat(packetFields.data, packetDelimiters.subfield);

        // UDP port
        char port[64];
        sprintf(port, "%d", connectedClients[i].socketUdpAddress.sin_port);
        strcat(packetFields.data, port);
        strcat(packetFields.data, packetDelimiters.subfield);

        printf("packet data: %s\n", packetFields.data);
        sendUdpPacket(udpSocketDescriptor, clientUdpAddress, packetFields, debugFlag);
      }
    }
  }
  else {
    strcpy(packetFields.data, "filenotfound");
    sendUdpPacket(udpSocketDescriptor, clientUdpAddress, packetFields, debugFlag);
  }
}
