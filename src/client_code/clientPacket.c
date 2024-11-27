#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/network_node.h"
#include "../common/packet.h"
#include "../common/tcp.h"
#include "../common/udp.h"
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
                  int tcpSocketDescriptor,
                  int udpSocketDescriptor,
                  struct sockaddr_in serverAddress,
                  struct ConnectedClient* connectedClients,
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
    handleClientInfoPacket(tcpSocketDescriptor, udpSocketDescriptor, packetFields.data,
                           connectedClients, debugFlag);
    break;

  case 4:
    if (debugFlag) {
      printf("Type of packet recieved is filereq\n");
    }
    handleFileReqPacket(tcpSocketDescriptor, packetFields.data, connectedClients,
                        debugFlag);

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
void sendClientInfoPacket(int udpSocketDescriptor,
                          struct sockaddr_in serverAddress,
                          char* fileName,
                          bool debugFlag) {
  struct PacketFields packetFields;
  strcpy(packetFields.type, "tcpinfo");
  strcpy(packetFields.data, fileName);

  sendUdpPacket(udpSocketDescriptor, serverAddress, packetFields, debugFlag);
}

/*
 * Purpose: When the client receives a packet of this type, this function extracts
 * the ip address and port of the client on the network who is hosting the file that the
 * user requested
 * Input:
 * - Data field of the packet sent from the server
 * - Debug flag
 * Output: Socket address struct of the client hosting the file
 */
void handleClientInfoPacket(int tcpSocketDescriptor,
                            int udpSocketDescriptor,
                            char* dataField,
                            struct ConnectedClient* connectedClients,
                            bool debugFlag) {
  int availableConnectedClient = findEmptyConnectedClient(connectedClients, debugFlag);

  if (tcpSocketDescriptor) {
  }

  if (udpSocketDescriptor) {
  }

  char* end;
  char subfield[64];
  memset(subfield, 0, sizeof(subfield));

  // Filename
  char* filename = calloc(1, MAX_FILENAME);
  dataField      = readPacketSubfield(dataField, filename, debugFlag);

  // TCP address
  dataField       = readPacketSubfield(dataField, subfield, debugFlag);
  long tcpAddress = strtol(subfield, &end, 10);
  connectedClients[availableConnectedClient].socketTcpAddress.sin_addr.s_addr =
      (unsigned int)tcpAddress;

  char* end2;
  char subfield2[64];
  memset(subfield2, 0, sizeof(subfield2));

  // TCP port
  dataField    = readPacketSubfield(dataField, subfield2, debugFlag);
  long tcpPort = strtol(subfield2, &end2, 10);
  connectedClients[availableConnectedClient].socketTcpAddress.sin_port =
      (short unsigned int)tcpPort;

  char* end3;
  char subfield3[64];
  memset(subfield3, 0, sizeof(subfield3));

  // UDP address
  dataField       = readPacketSubfield(dataField, subfield3, debugFlag);
  long udpAddress = strtol(subfield3, &end3, 10);
  connectedClients[availableConnectedClient].socketUdpAddress.sin_addr.s_addr =
      (unsigned int)udpAddress;

  char* end4;
  char subfield4[64];
  memset(subfield4, 0, sizeof(subfield4));

  // UDP port
  dataField    = readPacketSubfield(dataField, subfield4, debugFlag);
  long udpPort = strtol(subfield4, &end4, 10);
  connectedClients[availableConnectedClient].socketUdpAddress.sin_port =
      (short unsigned int)udpPort;

  // Pipes for communication between parent and child process
  pipe(connectedClients[availableConnectedClient].parentToChildPipe);
  pipe(connectedClients[availableConnectedClient].childToParentPipe);

  // Fork a new process for the client
  pid_t processId;
  if ((processId = fork()) == -1) { // Fork error
    perror("Error when forking a process for a new client");
  }
  else if (processId == 0) { // Child process
    close(connectedClients[availableConnectedClient]
              .parentToChildPipe[1]); // Close write on parent -> child.
    close(connectedClients[availableConnectedClient]
              .childToParentPipe[0]); // Close read on child -> parent.

    int childTcpListenDescriptor;

    // New TCP socket to receive file
    struct sockaddr_in childTcpListenAddress;
    memset(&childTcpListenAddress, 0, sizeof(childTcpListenAddress));
    childTcpListenAddress.sin_port = 0; // Wildcard
    childTcpListenDescriptor       = setupTcpSocket(childTcpListenAddress);

    if (debugFlag) {
      printf("Connecting to client...\n");
      printf("address: %d",
             connectedClients[availableConnectedClient].socketTcpAddress.sin_addr.s_addr);
      printf("port: %d",
             ntohs(connectedClients[availableConnectedClient].socketTcpAddress.sin_port));
    }

    // TCP connection to receive
    childTcpListenDescriptor = tcpConnect(
        "client", childTcpListenDescriptor,
        (struct sockaddr*)&connectedClients[availableConnectedClient].socketTcpAddress,
        sizeof(connectedClients[availableConnectedClient].socketTcpAddress));

    struct PacketFields packetFields;
    // Packet type
    strcpy(packetFields.type, "filereq");

    // Filename
    strcpy(packetFields.data, filename);
    strcat(packetFields.data, packetDelimiters.subfield);

    char address[64];
    sprintf(address, "%d", childTcpListenAddress.sin_addr.s_addr);
    strcat(packetFields.data, address);
    strcat(packetFields.data, packetDelimiters.subfield);

    char port[64];
    sprintf(port, "%d", childTcpListenAddress.sin_port);
    strcat(packetFields.data, port);
    strcat(packetFields.data, packetDelimiters.subfield);

    char* message = calloc(1, 200);
    buildPacket(message, packetFields, debugFlag);

    // Tell parent to request file
    write(connectedClients[availableConnectedClient]
              .parentToChildPipe[availableConnectedClient],
          message, (strlen(message) + 1));

    tcpReceiveFile(childTcpListenDescriptor, filename, debugFlag);
    exit(0);
  }
  else { // Parent process
    // Wait for child to send message through pipe
    close(connectedClients[availableConnectedClient]
              .parentToChildPipe[0]); // Close read on parent -> child. Write on
                                      // this pipe
    close(connectedClients[availableConnectedClient]
              .childToParentPipe[1]); // Close write on child -> parent. Read on this pipe
    char* dataFromChild = calloc(1, 100);
    read(connectedClients[availableConnectedClient].childToParentPipe[0], dataFromChild,
         100);
    // When child sends message, request transmission of file
    sendUdpMessage(udpSocketDescriptor,
                   connectedClients[availableConnectedClient].socketUdpAddress,
                   dataFromChild, debugFlag);
  }
}

// request test01.txt

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
                       struct sockaddr_in fileHostUdpAddress,
                       char* filename,
                       bool debugFlag) {
  if (tcpSocketDescriptor) {
  }
  if (udpSocketDescriptor) {
  }

  // Request file from file host
  struct PacketFields packetFields;
  // Packet type
  strcpy(packetFields.type, "filereq");

  // Filename
  strcpy(packetFields.data, filename);
  strcat(packetFields.data, packetDelimiters.subfield);

  sendUdpPacket(udpSocketDescriptor, fileHostUdpAddress, packetFields, debugFlag);
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
void handleFileReqPacket(int socketDescriptor,
                         char* dataField,
                         struct ConnectedClient* connectedClients,
                         bool debugFlag) {
  if (socketDescriptor) {
  }

  if (debugFlag) {
    printf("Handling file request packet\n");
  }

  char* filename = calloc(1, MAX_FILENAME);
  dataField      = readPacketSubfield(dataField, filename, debugFlag);

  char* tcpAddress = calloc(1, 64);
  dataField        = readPacketSubfield(dataField, tcpAddress, debugFlag);

  char* tcpPort = calloc(1, 64);
  dataField     = readPacketSubfield(dataField, tcpPort, debugFlag);

  int i;
  for (i = 0; i < 100; i++) {
    char clientAddress[64];
    char clientPort[64];

    sprintf(clientAddress, "%d", connectedClients[i].socketTcpAddress.sin_addr.s_addr);
    sprintf(clientPort, "%d", connectedClients[i].socketTcpAddress.sin_port);

    if (strcmp(clientAddress, tcpAddress) == 0) {
      if (strcmp(clientPort, tcpPort) == 0) {
        write(connectedClients[i].parentToChildPipe[1], filename, (strlen(filename) + 1));
      }
    }
  }
}
