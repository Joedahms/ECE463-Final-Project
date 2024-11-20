#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/network_node.h"
#include "../common/packet.h"
#include "client.h"

// Global so that signal handler can free resources
int udpSocketDescriptor;
int tcpSocketDescriptor;
char* packet;

// Packet delimiters that are constant for all packets
// See packet.h & packet.c
extern struct PacketDelimiters packetDelimiters;

// Main
int main(int argc, char* argv[]) {
  // Assign callback function to handle ctrl-c
  signal(SIGINT, shutdownClient);

  // Address of server (UDP)
  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family      = AF_INET;
  serverAddress.sin_port        = htons(PORT);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Local UDP port
  struct sockaddr_in hostUdpAddress;
  memset(&hostUdpAddress, 0, sizeof(hostUdpAddress));
  bool bindFlag       = false;
  udpSocketDescriptor = setupUdpSocket(hostUdpAddress, bindFlag);

  // Local TCP port
  struct sockaddr_in hostTcpAddress;
  memset(&hostTcpAddress, 0, sizeof(hostTcpAddress));
  hostTcpAddress.sin_port = 0; // Wildcard
  tcpSocketDescriptor     = setupTcpSocket(hostTcpAddress);

  hostTcpAddress = getTcpSocketInfo();

  bool debugFlag = false;
  checkCommandLineArguments(argc, argv, &debugFlag);

  if (sendConnectionPacket(hostTcpAddress, serverAddress, debugFlag) == -1) {
    printf("Error sending connection packet\n");
  }

  fd_set read_fds;
  packet = calloc(1, MAX_PACKET);

  // Loop to handle user input and incoming packets
  while (1) {
    // Use select to handle user input and server messages simultaneously
    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);                   // 0 is stdin (for user input)
    FD_SET(udpSocketDescriptor, &read_fds); // The socket for receiving server messages

    int activity = select(udpSocketDescriptor + 1, &read_fds, NULL, NULL, NULL);

    if (activity < 0 && errno != EINTR) {
      perror("select error");
    }

    // User input
    if (FD_ISSET(0, &read_fds)) {
      char* userInput = calloc(1, MAX_USER_INPUT);
      getUserInput(userInput);

      if (strcmp(userInput, "resources") == 0) {
        sendResourcePacket(serverAddress, debugFlag);
      }

      // User just pressed return
      if (strlen(userInput) == 0) {
        free(userInput);
        continue;
      }
      free(userInput);
    }

    // No message in UDP queue
    if (!(FD_ISSET(udpSocketDescriptor, &read_fds))) {
      continue;
    }

    // Message in UDP queue
    if (debugFlag) {
      printf("Packet received\n");
    }
    recvfrom(udpSocketDescriptor, packet, MAX_PACKET, 0, NULL, NULL);
    handlePacket(serverAddress, debugFlag);
  }
  return 0;
}

/*
 * Purpose: Free all resources associated with the client
 * Input: Signal received
 * Output: None
 */
void shutdownClient() {
  free(packet);
  close(udpSocketDescriptor);
  close(tcpSocketDescriptor);
  printf("\n");
  exit(0);
}

/*
 * Purpose: Get the available resources on the client and add them to the available
 * resources string.
 * Input:
 * - String to store the available resources in
 * - Path to the directory where the available resources are located
 * Output:
 * - -1: Error
 * - 0: Success
 */
int getAvailableResources(char* availableResources, const char* directoryName) {
  DIR* directoryStream = opendir(directoryName);
  if (directoryStream == NULL) {
    return -1;
    perror("Error opening resource directory");
  }

  // Loop through entire directory
  struct dirent* directoryEntry;
  while ((directoryEntry = readdir(directoryStream)) != NULL) {
    const char* entryName = directoryEntry->d_name;
    // Ignore current directory
    if (strcmp(entryName, ".") == 0) {
      continue;
    }
    // Ignore parent directory
    if (strcmp(entryName, "..") == 0) {
      continue;
    }
    strcat(availableResources, entryName);
    strcat(availableResources, packetDelimiters.subfield);
  }
  return 0;
}

/*
 * Purpose: Send a connection packet to the specified server.
 * Input:
 * - Socket address structure of the server to send the connection packet to
 * - Debug flag
 * Output:
 * -1: Error constructing or sending the connection packet, the packet was not sent
 * 0: Packet successfully sent
 */
int sendConnectionPacket(struct sockaddr_in hostTcpAddress,
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
 * Purpose: Send a resource packet to the server. This indicates that the client would
 * like to know all of the available resources on the network.
 * Input:
 * - Address of server to send the packet to
 * - Debug flag
 * Output: None
 */
void sendResourcePacket(struct sockaddr_in serverAddress, bool debugFlag) {
  struct PacketFields packetFields;
  strcpy(packetFields.type, "resource");
  strcpy(packetFields.data, "dummyfield");

  sendUdpPacket(udpSocketDescriptor, serverAddress, packetFields, debugFlag);
}

/*
 * Purpose: Take a packet of unknown type and call its corrosponding handler function
 * Input:
 * - Address of server that sent the packet
 * - Debug flag
 * Output: None
 */
void handlePacket(struct sockaddr_in serverAddress, bool debugFlag) {
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
    handleStatusPacket(serverAddress, debugFlag);
    break;

  // Resource
  case 2:
    if (debugFlag) {
      printf("Type of packet received is Resource\n");
    }
    handleResourcePacket(packetFields.data, debugFlag);
    break;

  default:
  }
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
      } else {
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
 * Purpose: When the client receives a status packet, send one back. The data field of
 * this packet doesn't matter as the client just needs to respond to be considered still
 * connected to the server. Input:
 * - Address of server to send the response status packet to
 * - Debug flag
 * Output: None
 */
void handleStatusPacket(struct sockaddr_in serverAddress, bool debugFlag) {
  struct PacketFields packetFields;
  memset(&packetFields, 0, sizeof(packetFields));
  strcpy(packetFields.type, "status");
  strcat(packetFields.data, "testing");

  sendUdpPacket(udpSocketDescriptor, serverAddress, packetFields, debugFlag);
}

/*
 * Purpose: Ask the user what username they would like to use when connecting to the
 * server. Allows the user to use their default username on the OS, or choose their own.
 * Input: Username string to copy the user's username choice into
 * Output: None
 */
void setUsername(char* username) {
  printf("\nWhat username would you like to use?\n");
  printf("Press 0 for system username\n");
  printf("Press 1 for custom username\n");

  bool validUsername = false;
  while (validUsername == false) {
    validUsername   = true;
    char* userInput = calloc(1, MAX_USER_INPUT);
    getUserInput(userInput);

    // System
    if (strcmp(userInput, "0") == 0) {
      strcpy(username, getenv("USER"));
      printf("System username chosen, welcome %s\n", username);
    }
    // Custom
    else if (strcmp(userInput, "1") == 0) {
      memset(userInput, 0, MAX_USER_INPUT);
      printf("Custom username chosen, please enter custom username:\n");
      getUserInput(userInput);
      strcpy(username, userInput);
      printf("Welcome %s\n", username);
    }
    // Invalid
    else {
      printf("Invalid option chosen, please try again\n");
      validUsername = false;
    }
    free(userInput);
  }
}

struct sockaddr_in getTcpSocketInfo() {
  struct sockaddr_in socketInfo;
  socklen_t socketInfoSize = sizeof(socketInfo);
  getsockname(tcpSocketDescriptor, (struct sockaddr*)&socketInfo, &socketInfoSize);
  return socketInfo;
}
