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
#include "../common/tcp.h"
#include "../common/udp.h"
#include "client.h"
#include "clientPacket.h"

// Global so that signal handler can free resources
int udpSocketDescriptor;
int listeningTcpSocketDescriptor;
int connectedTcpSocketDescriptor;
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
  hostTcpAddress.sin_port      = 0; // Wildcard
  listeningTcpSocketDescriptor = setupTcpSocket(hostTcpAddress);

  hostTcpAddress = getTcpSocketInfo(listeningTcpSocketDescriptor);

  bool debugFlag = false;
  checkCommandLineArguments(argc, argv, &debugFlag);

  if (sendConnectionPacket(udpSocketDescriptor, hostTcpAddress, serverAddress,
                           debugFlag) == -1) {
    printf("Error sending connection packet\n");
  }

  struct sockaddr_in incomingTcpConnection;
  struct sockaddr_in incomingUdpConnection;
  packet          = calloc(1, MAX_PACKET);
  char* userInput = calloc(1, MAX_USER_INPUT);

  // Loop to handle user input and incoming packets
  while (1) {
    int userInputReturn = getUserInput(userInput, 100);
    if (userInputReturn == -1) {
      printf("Error getting user input\n");
    }
    else if (userInputReturn == 1) {
      handleUserInput(userInput, serverAddress, debugFlag);
    }

    connectedTcpSocketDescriptor =
        checkTcpSocket(listeningTcpSocketDescriptor, &incomingTcpConnection, debugFlag);
    if (connectedTcpSocketDescriptor != -1) { // New TCP connection established
      if (debugFlag) {
        printf("File being requested, new TCP connection established\n");
      }
      pid_t processId;
      if ((processId = fork()) == -1) { // Fork error
        perror("Error when forking a process for a new client");
      }
      else if (processId == 0) { // Child process
        // wait for file request packet
        // send requested file
        if (debugFlag) {
          printf("Checking UDP socket for new packets...\n");
        }
        while (checkUdpSocket(udpSocketDescriptor, &incomingUdpConnection, packet,
                              debugFlag) == 0) {
          ;
        }
        handlePacket(packet, connectedTcpSocketDescriptor, udpSocketDescriptor,
                     incomingUdpConnection, debugFlag);
        exit(0);
      }
      else { // Parent process
      }
    }

    if (checkUdpSocket(udpSocketDescriptor, &incomingUdpConnection, packet, debugFlag)) {
      // Message in UDP queue
      if (debugFlag) {
        printf("UDP packet received\n");
      }
      recvfrom(udpSocketDescriptor, packet, MAX_PACKET, 0, NULL, NULL);
      handlePacket(packet, listeningTcpSocketDescriptor, udpSocketDescriptor,
                   serverAddress, debugFlag);
    }
  }
  return 0;
}

/*
 * Purpose: Get user input from standard in and remove the newline
 * Input: String to store user input in
 * Output: None
 */
int getUserInput(char* userInput, int timeoutUs) {
  /*
  fgets(userInput, MAX_USER_INPUT, stdin);
  userInput[strcspn(userInput, "\n")] = 0;
  return 1;
  */

  fd_set readfds;
  struct timeval tv;

  // Set up the file descriptor set
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  // Set up the timeout
  tv.tv_sec  = 0;
  tv.tv_usec = timeoutUs;

  // Wait for input
  int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

  if (ready == -1) {
    // Error occurred
    return -1;
  }
  else if (ready == 0) {
    // Timeout occurred
    return 0;
  }

  // Input is available, read it
  if (fgets(userInput, MAX_USER_INPUT, stdin) == NULL) {
    return -1;
  }

  userInput[strcspn(userInput, "\n")] = 0;
  return 1;
}

/*
 * Purpose: Decide what to do next depending on what the user entered
 * Input:
 * - The user's input
 * - The socket address of the server
 * - Debug flag
 * Output: None
 */
void handleUserInput(char* userInput, struct sockaddr_in serverAddress, bool debugFlag) {
  if (strcmp(userInput, "resources") == 0) {
    sendResourcePacket(udpSocketDescriptor, serverAddress, debugFlag);
  }
  else if (strncmp(userInput, "request", 7) == 0) {
    userInput += 8;
    sendClientInfoPacket(udpSocketDescriptor, serverAddress, userInput, debugFlag);
  }
  else {
    printf("Invalid command, please try again\n");
    printf("Valid commands:\n");
    printf("resources: See what resources are available on the network\n");
    printf("request:   Request a resource on the network by filename\n");
  }
}

/*
 * Purpose: Free all resources associated with the client
 * Input: Signal received
 * Output: None
 */
void shutdownClient() {
  free(packet);
  close(udpSocketDescriptor);
  close(listeningTcpSocketDescriptor);
  close(connectedTcpSocketDescriptor);
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
    while (getUserInput(userInput, 100) == 0) {
      ;
    }

    // System
    if (strcmp(userInput, "0") == 0) {
      strcpy(username, getenv("USER"));
      printf("System username chosen, welcome %s\n", username);
    }
    // Custom
    else if (strcmp(userInput, "1") == 0) {
      memset(userInput, 0, MAX_USER_INPUT);
      printf("Custom username chosen, please enter custom username:\n");
      while (getUserInput(userInput, 100) == 0) {
        ;
      }
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
