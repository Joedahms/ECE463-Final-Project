#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "../common/network_node.h"
#include "../common/packet.h"
#include "server.h"
#include "resource.h"

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

  bool debugFlag = false; // Can add conditional statements with this flag to print out extra info

  // Make sure all connectedClients are set to 0
  int i; 
  for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
    memset(&(connectedClients[i].socketUdpAddress), 0, sizeof(connectedClients[i].socketUdpAddress));
  }

  headResource = malloc(sizeof(struct Resource));
  headResource->next = NULL;

  // UDP socket clients should connect to
  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;                 // IPV4
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(PORT);
  udpSocketDescriptor = setupUdpSocket(serverAddress, 1);
  
  checkCommandLineArguments(argc, argv, &debugFlag);
  
  bool packetAvailable = false;
  int packetType = 0;

  // pthread to check client connection status
  pthread_t processId;
  pthread_create(&processId, NULL, checkClientStatus, &debugFlag);

  packet = calloc(1, MAX_PACKET);

  // Continously listen for new UDP packets and new TCP connections
  while (1) {
    struct sockaddr_in clientUDPAddress;
    packetAvailable = checkUdpSocket(udpSocketDescriptor, &clientUDPAddress, packet, debugFlag);  // Check the UDP socket

    if (!packetAvailable) {
      continue;
    }

    if (debugFlag) {
      printf("Packet received\n");
    }
    struct PacketFields packetFields;
    memset(&packetFields, 0, sizeof(packetFields));
    readPacket(packet, &packetFields, debugFlag);
    packetType = getPacketType(packetFields.type, debugFlag);

    switch(packetType) {
      // Connection packet
      case 0:
      if (debugFlag) {
        printf("Type of packet recieved is connection\n");
      }
      handleConnectionPacket(packetFields.data, clientUDPAddress, debugFlag);
      break;

      // Status packet
      case 1:
      if (debugFlag) {
        printf("Type of packet received is status\n");
      }
      handleStatusPacket(clientUDPAddress);
      break;

      // Resource packet
      case 2:
      if (debugFlag) {
        printf("Type of packet received is Resource\n");
      }
      handleResourcePacket(clientUDPAddress, debugFlag);
      break;

      default:
    }
    memset(packet, 0, strlen(packet));
  } // while(1)
  return 0;
} // main

/*
  * Purpose: Check if clients are still connected to the server. Send every connected client
  * a packet asking if they are still connected. If they send a response within the set time frame,
  * they are considered to still be connected. If they do not send a packet back, they are considered
  * to be no longer connected. If a client is no longer connected, its information is erased from the user
  * directory (connectedClients).
  * Input: None
  * Output: None
  * Notes: This function is run in a thread spawned from the main process. It is alive for the entire
  * duration of the main process. It only exits when the server shuts down.
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
  while(1) {
    for (clientIndex = 0; clientIndex < 100; clientIndex++) {
      client = &connectedClients[clientIndex];
      clientUdpAddress = client->socketUdpAddress;

      // Check that client is initialized and connected
      if (clientUdpAddress.sin_addr.s_addr == 0 && clientUdpAddress.sin_port == 0) {
        continue;
      }
      if (client->status == false) {
        continue;
      }

      client->status = false;         // Assume client is disconnected and will not respond
      client->requestedStatus = true; // Requested a response from the client
      sendUdpMessage(udpSocketDescriptor, clientUdpAddress, statusPacket, debugFlag);
      if (debugFlag) {
        printf("Status packet sent to client %d\n", clientIndex);
      }
    }

    // Give clients a chance to send responses
    usleep(STATUS_SEND_INTERVAL);

    // If a response was requested and the client didn't send a response, remove them from the "user directory"
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
void shutdownServer(int signal) {
  free(packet);
  close(udpSocketDescriptor);
  printf("\n");
  exit(0);
}

/*
  * Purpose: Loop through the connectedClients array until an empty spot is found. Looks for unset
  * UDP port.
  * Input: debugFlag
  * Output:
  * - -1: All spots in the connected client array are full
  * - Anything else: Index of the empty spot in the array
*/
int findEmptyConnectedClient(bool debugFlag) {
  int connectedClientsIndex;
  for (connectedClientsIndex = 0; connectedClientsIndex < MAX_CONNECTED_CLIENTS; connectedClientsIndex++) {
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
  char* username = calloc(1, MAX_USERNAME);

  int i;
  for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
    udpAddress = ntohl(connectedClients[i].socketUdpAddress.sin_addr.s_addr);
    udpPort = ntohs(connectedClients[i].socketUdpAddress.sin_port);
    if (udpAddress == 0 && udpPort == 0) {
      i++;
      continue;
    }
    strcpy(username, connectedClients[i].username);
    printf("CONNECTED CLIENT %d\n", i);
    printf("USERNAME: %s\n", username);
    memset(username, 0, MAX_USERNAME);
    printf("UDP ADDRESS: %ld\n", udpAddress);
    printf("UDP PORT: %d\n", udpPort);
  }
  free(username);
  printf("\n");
}

/*
  * Purpose: Traverse the data field in a packet and add the resources in the data field
  * to the resource directory. It is assumed that the username has already been read off the data field
  * when this function is called.
  * Input: 
  * - Data field of a packet
  * - How long the data field is
  * - Username of the client who sent the packet
  * - Debug flag
  * Output: None
*/
void addResourcesToDirectory(char* dataField, int dataFieldLength, char* username, bool debugFlag) {
  char* resource = calloc(1, MAX_DATA);
  char* resourceBeginning = resource;

  int bytesRead = 0;
  while (bytesRead != dataFieldLength) {
    dataField = readPacketSubfield(dataField, resource, debugFlag);
    bytesRead += strlen(resource) + packetDelimiters.subfieldLength;
    headResource = addResource(headResource, username, resource);
    memset(resource, 0, strlen(resource));
  }
  free(resourceBeginning);
}

/*
  * Purpose: When the server receives a connection packet, this function handles the data
  * in that packet. It finds an empty connected client and enters the packet sender's information
  * into that empty spot.
  * Input: 
  * - The connection packet that was sent
  * - The address of the client who sent the packet
  * Output: None
*/
void handleConnectionPacket(char* packetData, struct sockaddr_in clientUDPAddress, bool debugFlag) {
  int emptyClientIndex = findEmptyConnectedClient(debugFlag);
  struct ConnectedClient* emptyClient = &connectedClients[emptyClientIndex];

  // Connection info and status
  emptyClient->socketUdpAddress.sin_addr.s_addr = clientUDPAddress.sin_addr.s_addr;
  emptyClient->socketUdpAddress.sin_port = clientUDPAddress.sin_port;
  emptyClient->status = true;

  // Username
  char* username = calloc(1, MAX_USERNAME);
  char* usernameBeginning = username;
  packetData = readPacketSubfield(packetData, username, debugFlag);
  strcpy(emptyClient->username, username);

  addResourcesToDirectory(packetData, strlen(packetData), username, debugFlag);

  free(usernameBeginning);

  if (debugFlag) {
    printAllConnectedClients();
    printAllResources(headResource);
  }
}

/*
  * Purpose: When the server receives a status packet, this function handles the data
  * in that packet. It loops through all the connected clients and finds the one who sent
  * the status packet. The status of the client who sent the packet is set to indicate
  * that the client is still connected.
  * Input: 
  * - The address of the client who sent the status packet
  * Output: None
*/
void handleStatusPacket(struct sockaddr_in clientUdpAddress) {
  int clientIndex;
  struct ConnectedClient* currentClient;

  for (clientIndex = 0; clientIndex < MAX_CONNECTED_CLIENTS; clientIndex++) {
    currentClient = &connectedClients[clientIndex];

    // Client
    unsigned long currentClientAddress = connectedClients[clientIndex].socketUdpAddress.sin_addr.s_addr;
    unsigned short currentClientPort = connectedClients[clientIndex].socketUdpAddress.sin_port;

    // Packet sender
    unsigned long incomingAddress = clientUdpAddress.sin_addr.s_addr;
    unsigned short incomingPort = clientUdpAddress.sin_port;

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
  * Purpose: Servers actions upon receiving a resource packet. When a resource packet is received, it
  * means a client requested the available resources in the network. This function gathers those available
  * resources and sends them to the client that requested them.
  * Input:
  * - Client that inquired about the available resources
  * - Debug flag
  * Output: 0
*/
int handleResourcePacket(struct sockaddr_in clientUdpAddress, bool debugFlag) {
  struct PacketFields packetFields;
  memset(&packetFields, 0, sizeof(packetFields));
  strcpy(packetFields.type, "resource");

  char* resourceString = calloc(1, MAX_DATA);
  resourceString = makeResourceString(resourceString, headResource, packetDelimiters.subfield);
  strcpy(packetFields.data, resourceString);
  free(resourceString);

  char* packet = calloc(1, MAX_PACKET);
  buildPacket(packet, packetFields, debugFlag);
  sendUdpMessage(udpSocketDescriptor, clientUdpAddress, packet, debugFlag);
  free(packet);

  return 0;
}
