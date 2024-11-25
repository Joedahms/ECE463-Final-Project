#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/network_node.h"
#include "../common/packet.h"
#include "../common/udp.h"
#include "resource.h"
#include "server.h"
#include "serverPacket.h"

// Global so that signal handler can free resources
int udpSocketDescriptor;
char* packet;

// User and resource "directories"
struct ConnectedClient connectedClients[MAX_CONNECTED_CLIENTS];
struct Resource* headResource;

// packet.h
extern struct PacketDelimiters packetDelimiters;

// Main fucntion
int main(int argc, char* argv[]) {
  // Assign callback function for Ctrl-c
  signal(SIGINT, shutdownServer);

  bool debugFlag = false; // Can add conditional statements with this flag to
                          // print out extra info

  // Make sure all connectedClients are set to 0
  int i;
  for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
    memset(&(connectedClients[i].socketUdpAddress), 0,
           sizeof(connectedClients[i].socketUdpAddress));
  }

  headResource       = malloc(sizeof(struct Resource));
  headResource->next = NULL;

  // UDP socket clients should connect to
  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family      = AF_INET; // IPV4
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port        = htons(PORT);
  udpSocketDescriptor           = setupUdpSocket(serverAddress, 1);

  checkCommandLineArguments(argc, argv, &debugFlag);

  bool packetAvailable = false;

  // pthread to check client connection status
  pthread_t processId;
  pthread_create(&processId, NULL, checkClientStatus, &debugFlag);

  packet = calloc(1, MAX_PACKET);

  // Continously listen for new UDP packets and new TCP connections
  while (1) {
    struct sockaddr_in clientUdpAddress;
    packetAvailable = checkUdpSocket(udpSocketDescriptor, &clientUdpAddress, packet,
                                     debugFlag); // Check the UDP socket

    if (!packetAvailable) {
      continue;
    }

    handlePacket(packet, udpSocketDescriptor, clientUdpAddress, &connectedClients[0],
                 headResource, debugFlag);

  } // while(1)
  return 0;
} // main

/*
 * Purpose: Check if clients are still connected to the server. Send every
 * connected client a packet asking if they are still connected. If they send a
 * response within the set time frame, they are considered to still be
 * connected. If they do not send a packet back, they are considered to be no
 * longer connected. If a client is no longer connected, its information is
 * erased from the user directory (connectedClients).
 * Input: None
 * Output: None
 * Notes: This function is run in a thread spawned from the main process. It is
 * alive for the entire duration of the main process. It only exits when the
 * server shuts down.
 */
void* checkClientStatus(void* input) {
  // Have to cast then dereference the input to use it
  bool debugFlag = *((bool*)input);

  // Status packet
  struct PacketFields packetFields;
  strcpy(packetFields.type, "status");
  strcpy(packetFields.data, "testing");
  char* statusPacket = calloc(1, MAX_PACKET);
  buildPacket(statusPacket, packetFields, debugFlag);

  struct ConnectedClient* client;
  struct sockaddr_in clientUdpAddress;

  int clientIndex;
  while (1) {
    for (clientIndex = 0; clientIndex < 100; clientIndex++) {
      client           = &connectedClients[clientIndex];
      clientUdpAddress = client->socketUdpAddress;

      // Check that client is initialized and connected
      if (clientUdpAddress.sin_addr.s_addr == 0 && clientUdpAddress.sin_port == 0) {
        continue;
      }
      if (client->status == false) {
        continue;
      }

      client->status = false; // Assume client is disconnected and will not respond
      client->requestedStatus = true; // Requested a response from the client
      sendUdpMessage(udpSocketDescriptor, clientUdpAddress, statusPacket, debugFlag);
      if (debugFlag) {
        printf("Status packet sent to client %d\n", clientIndex);
      }
    }

    // Give clients a chance to send responses
    usleep(STATUS_SEND_INTERVAL);

    // If a response was requested and the client didn't send a response, remove
    // them from the "user directory"
    for (clientIndex = 0; clientIndex < 100; clientIndex++) {
      client = &connectedClients[clientIndex];
      if (client->requestedStatus == true && client->status == false) {
        if (debugFlag) {
          printf("Client %d disconnected\n", clientIndex);
        }
        headResource = removeUserResources(client->username, headResource, debugFlag);
        memset(client, 0, sizeof(*client));
      }
    }
  }
  free(statusPacket);
}

/*
 * Purpose: Gracefully shutdown the server when the user enters
 * ctrl-c. Closes the sockets and frees addrinfo data structure
 * Input: The signal raised
 * Output: None
 */
void shutdownServer() {
  free(packet);
  close(udpSocketDescriptor);
  printf("\n");
  exit(0);
}

/*
 * Purpose: Loop through the connectedClients array until an empty spot is
 * found. Looks for unset UDP port.
 * Input: debugFlag
 * Output:
 * - -1: All spots in the connected client array are full
 * - Anything else: Index of the empty spot in the array
 */
int findEmptyConnectedClient(bool debugFlag) {
  int connectedClientsIndex;
  for (connectedClientsIndex = 0; connectedClientsIndex < MAX_CONNECTED_CLIENTS;
       connectedClientsIndex++) {
    int port = ntohs(connectedClients[connectedClientsIndex].socketUdpAddress.sin_port);

    // Port not set, empty spot
    if (port == 0) {
      return connectedClientsIndex;
      if (debugFlag) {
        printf("%d is empty\n", connectedClientsIndex);
      }
    }
    else {
      if (debugFlag) {
        printf("%d is not empty\n", connectedClientsIndex);
      }
    }
  }

  // All spots filled
  return -1;
}

/*
 * Purpose: Print all the connected clients in a readable format
 * Input: None
 * Output: None
 */
void printAllConnectedClients() {
  printf("\n*** PRINTING ALL CONNECTED CLIENTS ***\n");
  unsigned long udpAddress;
  unsigned short udpPort;
  unsigned long tcpAddress;
  unsigned short tcpPort;
  char* username = calloc(1, MAX_USERNAME);

  int i;
  for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
    udpAddress = ntohl(connectedClients[i].socketUdpAddress.sin_addr.s_addr);
    udpPort    = ntohs(connectedClients[i].socketUdpAddress.sin_port);
    if (udpAddress == 0 && udpPort == 0) {
      i++;
      continue;
    }
    tcpAddress = ntohl(connectedClients[i].socketTcpAddress.sin_addr.s_addr);
    tcpPort    = ntohs(connectedClients[i].socketTcpAddress.sin_port);
    strcpy(username, connectedClients[i].username);

    printf("CONNECTED CLIENT %d\n", i);
    printf("USERNAME: %s\n", username);
    memset(username, 0, MAX_USERNAME);
    printf("UDP ADDRESS: %ld\n", udpAddress);
    printf("UDP PORT: %d\n", udpPort);
    printf("TCP ADDRESS: %ld\n", tcpAddress);
    printf("TCP PORT: %d\n", tcpPort);
  }
  free(username);
  printf("\n");
}

/*
 * Purpose: Traverse the data field in a packet and add the resources in the
 * data field to the resource directory. It is assumed that the username has
 * already been read off the data field when this function is called.
 * Input:
 * - Data field of a packet
 * - How long the data field is
 * - Username of the client who sent the packet
 * - Debug flag
 * Output: None
 */
void addResourcesToDirectory(char* dataField,
                             long unsigned int dataFieldLength,
                             char* username,
                             bool debugFlag) {
  char* resource          = calloc(1, MAX_DATA);
  char* resourceBeginning = resource;

  long unsigned int bytesRead = 0;
  while (bytesRead != dataFieldLength) {
    dataField = readPacketSubfield(dataField, resource, debugFlag);
    bytesRead += strlen(resource) + packetDelimiters.subfieldLength;
    headResource = addResource(headResource, username, resource);
    memset(resource, 0, strlen(resource));
  }
  free(resourceBeginning);
}
