#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "../common/network_node.h"
#include "../common/packet.h"
#include "client.h"
#include "clientPacket.h"

extern struct PacketDelimiters packetDelimiters;

/*
 * Purpose: Take a packet of unknown type and call its corrosponding handler function
 * Input:
 * - Packet to handle
 * - UDP socket of caller
 * - Address of server that sent the packet
 * - Debug flag
 * Output: None
 */
void handlePacket(char* packet,
                  int udpSocketDescriptor,
                  struct sockaddr_in serverAddress,
                  bool debugFlag) {
  struct PacketFields packetFields;
  memset(&packetFields, 0, sizeof(packetFields));
  readPacket(packet, &packetFields, debugFlag);

  int packetType = getPacketType(packetFields.type, debugFlag);
  switch (packetType) {
  // Connection
  case 0:
    if (debugFlag) {
      printf("Type of packet recieved is connection\n");
    }
    break;

  // Status
  case 1:
    if (debugFlag) {
      printf("Type of packet received is status\n");
    }
    handleStatusPacket(udpSocketDescriptor, serverAddress, debugFlag);
    break;

  // Resource
  case 2:
    if (debugFlag) {
      printf("Type of packet received is Resource\n");
    }
    handleResourcePacket(packetFields.data, debugFlag);
    break;

  case 3:
    if (debugFlag) {
      printf("Type of packet received is tcpinfo\n");
    }
    handleTcpInfoPacket(packetFields.data, debugFlag);
    break;

  case 4:
    if (debugFlag) {
      printf("Type of packet recieved is filereq\n");
    }
    handleFileReqPacket(packetFields.data, debugFlag);

  default:
  }
}

/*
 * Purpose: Send a resource packet to the server. This indicates that the client would
 * like to know all of the available resources on the network.
 * Input:
 * - Socket descriptor of caller
 * - Address of server to send the packet to
 * - Debug flag
 * Output: None
 */
void sendResourcePacket(int socketDescriptor,
                        struct sockaddr_in serverAddress,
                        bool debugFlag) {
  struct PacketFields packetFields;
  strcpy(packetFields.type, "resource");
  strcpy(packetFields.data, "dummyfield");

  sendUdpPacket(socketDescriptor, serverAddress, packetFields, debugFlag);
}

/*
 * Purpose: Print out all available resources in a sent resource packet
 * Input:
 * - Data field of the sent resource packet
 * - Debug flag
 * Output: None
 */
void handleResourcePacket(char* dataField, bool debugFlag) {
  char* username         = calloc(1, MAX_USERNAME);
  char* resourceSubfield = calloc(1, MAX_FILENAME);

  long unsigned int dataFieldLength = strlen(dataField);
  long unsigned int bytesRead       = 0;
  int fieldCount                    = 0;
  while (bytesRead != dataFieldLength) {
    memset(resourceSubfield, 0, strlen(resourceSubfield));
    dataField = readPacketSubfield(dataField, resourceSubfield, debugFlag);
    bytesRead += strlen(resourceSubfield) + packetDelimiters.subfieldLength;

    // First field
    if (fieldCount == 0) {
      strcpy(username, resourceSubfield);
      printf("Username: %s\n", username);
    }

    // Username
    if (fieldCount % 2 == 0) {
      // Don't duplicate username printout
      if (strcmp(username, resourceSubfield) == 0) {
        ;
      }
      else {
        strcpy(username, resourceSubfield);
        printf("Username: %s\n", username);
      }
    }
    // Filename
    else {
      printf("Filename: %s\n", resourceSubfield);
    }
    fieldCount++;
  }

  free(username);
  free(resourceSubfield);
}

/*
 * Purpose: Send a connection packet to the specified server.
 * Input:
 * - Socket descriptor of caller
 * - Socket address structure of the TCP socket on the caller
 * - Socket address structure of the server to send the connection packet to
 * - Debug flag
 * Output:
 * -1: Error constructing or sending the connection packet, the packet was not sent
 * 0: Packet successfully sent
 */
int sendConnectionPacket(int udpSocketDescriptor,
                         struct sockaddr_in hostTcpAddress,
                         struct sockaddr_in serverAddress,
                         bool debugFlag) {
  struct PacketFields packetFields;

  // Type
  strcpy(packetFields.type, "connection");

  // Username
  char* username = calloc(1, MAX_USERNAME);
  setUsername(username);
  strcpy(packetFields.data, username);
  strncat(packetFields.data, packetDelimiters.subfield, packetDelimiters.subfieldLength);
  free(username);

  // Tcp socket
  char ipAddress[64];
  sprintf(ipAddress, "%d", hostTcpAddress.sin_addr.s_addr);
  char port[64];
  sprintf(port, "%d", hostTcpAddress.sin_port);
  strcat(packetFields.data, ipAddress);
  strcat(packetFields.data, packetDelimiters.subfield);
  strcat(packetFields.data, port);
  strcat(packetFields.data, packetDelimiters.subfield);

  // Available resources
  char* availableResources = calloc(1, MAX_DATA);
  if (getAvailableResources(availableResources, "Public") == -1) {
    return -1;
  }
  strcat(packetFields.data, availableResources);
  free(availableResources);

  sendUdpPacket(udpSocketDescriptor, serverAddress, packetFields, debugFlag);

  return 0;
}

/*
 * Purpose: When the client receives a status packet, send one back. The data field of
 * this packet doesn't matter as the client just needs to respond to be considered still
 * connected to the server. Input:
 * - UDP socket of the caller
 * - Address of server to send the response status packet to
 * - Debug flag
 * Output: None
 */
void handleStatusPacket(int udpSocketDescriptor,
                        struct sockaddr_in serverAddress,
                        bool debugFlag) {
  struct PacketFields packetFields;
  memset(&packetFields, 0, sizeof(packetFields));
  strcpy(packetFields.type, "status");
  strcat(packetFields.data, "testing");

  sendUdpPacket(udpSocketDescriptor, serverAddress, packetFields, debugFlag);
}

/*
 * Purpose: This function is called when a client wants to know which client is hosting a
 * file it wants. The tcpinfo packet indicates to the server that the client would like to
 * know the TCP address+port on the hosting client
 * Input:
 * - UDP socket of the caller
 * - Address of the server to send the tcpinfo packet to
 * - Name of the file that the client is requesting
 * - Debug flag
 * Output: None
 */
void sendTcpInfoPacket(int udpSocketDescriptor,
                       struct sockaddr_in serverAddress,
                       char* fileName,
                       bool debugFlag) {
  struct PacketFields packetFields;
  strcpy(packetFields.type, "tcpinfo");
  strcpy(packetFields.data, fileName);

  sendUdpPacket(udpSocketDescriptor, serverAddress, packetFields, debugFlag);
}

/*
 * Purpose: When the client receives a tcpinfo packet, this function extracts
 * the ip address and port of the client on the network who is hosting the file that the
 * user requested
 * Input:
 * - Data field of the tcpinfo packet sent from the server
 * - Debug flag
 * Output: Socket address struct of the client hosting the file
 */
void handleTcpInfoPacket(char* dataField, bool debugFlag) {
  struct sockaddr_in fileHostAddress;
  memset(&fileHostAddress, 0, sizeof(fileHostAddress));
  char* end;
  char* tcpinfoSubfield = calloc(1, 64);

  // Address
  dataField    = readPacketSubfield(dataField, tcpinfoSubfield, debugFlag);
  long address = strtol(tcpinfoSubfield, &end, 10);
  fileHostAddress.sin_addr.s_addr = (unsigned int)address;

  // Port
  dataField                = readPacketSubfield(dataField, tcpinfoSubfield, debugFlag);
  long port                = strtol(tcpinfoSubfield, &end, 10);
  fileHostAddress.sin_port = (short unsigned int)port;

  if (debugFlag) {
    printf("file host address: %d\n", fileHostAddress.sin_addr.s_addr);
    printf("file host port: %d\n", fileHostAddress.sin_port);
  }

  // sendFileReqPacket(fileHostAddress, )
}

/*
 * Purpose: Send a file request packet to another client. This indicates that the sending
 * client would like a file hosted on the client receiving the packet
 * Input:
 * - TCP socket of the caller
 * - UDP socket of the caller
 * - Socket address structure of the client hosting the file
 * - Name of the file being requested
 * - Debug flag
 * Output: None
 */
void sendFileReqPacket(int tcpSocketDescriptor,
                       int udpSocketDescriptor,
                       struct sockaddr_in fileHost,
                       char* filename,
                       bool debugFlag) {
  // Start thread to receive file
  struct ReceiveThreadInfo receiveThreadInfo;
  receiveThreadInfo.socketDescriptor = tcpSocketDescriptor;
  strcpy(receiveThreadInfo.filename, filename);
  receiveThreadInfo.debugFlag = debugFlag;
  pthread_t processId;
  pthread_create(&processId, NULL, receiveFile, &receiveThreadInfo);

  // Requesting client
  struct sockaddr_in tcpSocketInfo;
  socklen_t tcpSocketInfoSize = sizeof(tcpSocketInfo);
  getsockname(tcpSocketDescriptor, (struct sockaddr*)&tcpSocketInfo, &tcpSocketInfoSize);

  // Request file from file host
  struct PacketFields packetFields;
  // Packet type
  strcpy(packetFields.type, "filereq");
  strcat(packetFields.data, packetDelimiters.subfield);

  // Filename
  strcat(packetFields.data, filename);
  strcat(packetFields.data, packetDelimiters.subfield);

  // Ip address of requesting client
  char address[64];
  sprintf(address, "%d", tcpSocketInfo.sin_addr.s_addr);
  strcat(packetFields.data, address);
  strcat(packetFields.data, packetDelimiters.subfield);

  // Port of requesting client
  char port[64];
  sprintf(port, "%d", tcpSocketInfo.sin_port);
  strcat(packetFields.data, port);
  strcat(packetFields.data, packetDelimiters.subfield);

  sendUdpPacket(udpSocketDescriptor, fileHost, packetFields, debugFlag);
}

/*
 * Purpose: This function indicates that another client wants a file that is hosted on the
 * client that called this function. The function processes the filereq packet and sends
 * the desired file
 * Input:
 * - Data field of the received filereq packet
 * - Debug flag
 * Output: None
 */
void handleFileReqPacket(char* dataField, bool debugFlag) {
  // Determine filename to send
  char* filename = calloc(1, MAX_FILENAME);
  dataField      = readPacketSubfield(dataField, filename, debugFlag);

  struct SendThreadInfo sendThreadInfo;
  strcpy(sendThreadInfo.filename, filename);

  char* end;
  char* requesterTcpInfo = calloc(1, 64);

  dataField    = readPacketSubfield(dataField, requesterTcpInfo, debugFlag);
  long address = strtol(requesterTcpInfo, &end, 10);
  sendThreadInfo.fileHost.sin_addr.s_addr = (unsigned int)address;

  dataField = readPacketSubfield(dataField, requesterTcpInfo, debugFlag);
  long port = strtol(requesterTcpInfo, &end, 10);
  sendThreadInfo.fileHost.sin_port = (short unsigned int)port;

  free(filename);

  // Send file
  pthread_t processId;
  pthread_create(&processId, NULL, sendFile, &sendThreadInfo);
}
