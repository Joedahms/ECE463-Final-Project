
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "network_node.h"
#include "tcp.h"

/*
 * Purpose: Send a desired number of bytes out on a Socket
 * Input:
 * - Socket Descriptor of the socket to send the bytes with
 * - Buffer containing the bytes to send
 * - Amount of bytes to send
 * - Debug flag
 * Output: Number of bytes sent
 */
long int tcpSendBytes(int socketDescriptor,
                      const char* buffer,
                      unsigned long int bufferSize,
                      uint8_t debugFlag) {
  if (debugFlag) {
    printf("Bytes to be sent:\n\n");
    unsigned long int i;
    for (i = 0; i < bufferSize; i++) {
      printf("%c", buffer[i]);
    }
    printf("\n\n");
  }

  long int bytesSent = 0;
  bytesSent          = send(socketDescriptor, buffer, bufferSize, 0);
  if (bytesSent == -1) {
    char* errorMessage = malloc(1024);
    strcpy(errorMessage, strerror(errno));
    printf("Byte send failed with error %s\n", errorMessage);
    exit(1);
  }
  else {
    return bytesSent;
  }
}

/*
 * Purpose: This function is for receiving a set number of bytes into
 * a buffer
 * Input:
 * - Socket Descriptor of the accepted transmission
 * - Buffer to put the received data into
 * - The size of the message to receive in bytes
 * Output:
 * - The number of bytes received into the buffer
 */
long int tcpReceiveBytes(int incomingSocketDescriptor,
                         char* buffer,
                         long unsigned int bufferSize,
                         uint8_t debugFlag) {
  long int numberOfBytesReceived = 0;
  numberOfBytesReceived          = recv(incomingSocketDescriptor, buffer, bufferSize, 0);
  if (debugFlag) {
    // Print out incoming message
    int i;
    printf("Bytes received: \n");
    for (i = 0; i < numberOfBytesReceived; i++) {
      printf("%c", buffer[i]);
    }
    printf("\n");
  }
  return numberOfBytesReceived;
}

/*
 * Purpose: Send a file to the server
 * Input: Name of the file to send
 * Output: None
 */
void tcpSendFile(int socketDescriptor, char* fileName, bool debugFlag) {
  char* fileContents = malloc(MAX_FILE_SIZE);
  int readFileReturn = readFile(fileName, fileContents, debugFlag);
  if (readFileReturn == -1) {
    printf("Put command error when reading file");
  }
  tcpSendBytes(socketDescriptor, fileContents, strlen(fileContents), debugFlag);
}

/*
 * Purpose: Receive file from server and write it into local directory
 * Input: File name of requested file
 * Output:
 * - -1: failure
 * - 0: Success
 */
long int tcpReceiveFile(int socketDescriptor, char* fileName, bool debugFlag) {
  printf("Receiving file...\n");
  char* incomingFileContents = malloc(MAX_FILE_SIZE); // Space for file contents
  long int numberOfBytesReceived;                     // How many bytes received
  while (numberOfBytesReceived == 0 || numberOfBytesReceived == -1) {
    printf("%ld\n", numberOfBytesReceived);
    numberOfBytesReceived =
        recv(socketDescriptor, incomingFileContents, MAX_FILE_SIZE, 0);
  }

  int writeFileReturn = writeFile(fileName, incomingFileContents,
                                  (long unsigned int)numberOfBytesReceived, debugFlag);
  if (writeFileReturn == -1) {
    return -1;
  }

  if (debugFlag) {
    printf("Received file is %ld bytes\n", numberOfBytesReceived);
    printf("Contents of received file:\n%s\n", incomingFileContents);
  }
  else {
    printf("Received file\n");
  }
  return 0;
}

/*
 * Name: setupTcpSocket
 * Purpose: Setup the TCP socket. Set it non blocking. Bind it. Set it to listen.
 * Input: Address structure to bind to.
 * Output: The setup TCP socket descriptor.
 */
int setupTcpSocket(struct sockaddr_in hostAddress) {
  int tcpSocketDescriptor;

  // Set up TCP socket
  printf("Setting up TCP socket...\n");
  tcpSocketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (tcpSocketDescriptor == -1) {
    perror("Error when setting up TCP socket");
    exit(1);
  }

  // Set non blocking
  int fcntlReturn = fcntl(tcpSocketDescriptor, F_SETFL, O_NONBLOCK);
  if (fcntlReturn == -1) {
    perror("Error when setting TCP socket to non blocking");
  }
  printf("TCP socket set up\n");

  // Bind TCP socket
  printf("Binding TCP socket...\n");
  int bindReturnTCP =
      bind(tcpSocketDescriptor, (struct sockaddr*)&hostAddress, sizeof(hostAddress));
  if (bindReturnTCP == -1) {
    perror("Error when binding TCP socket");
    exit(1);
  }
  printf("TCP socket bound\n");

  // Set socket to listen
  int listenReturn = listen(tcpSocketDescriptor, 10);
  if (listenReturn == -1) {
    perror("TCP socket listen error");
    exit(1);
  }
  return tcpSocketDescriptor;
}

struct sockaddr_in getTcpSocketInfo(int tcpSocketDescriptor) {
  struct sockaddr_in socketInfo;
  socklen_t socketInfoSize = sizeof(socketInfo);
  getsockname(tcpSocketDescriptor, (struct sockaddr*)&socketInfo, &socketInfoSize);
  return socketInfo;
}

/*
 * Name: checkTcpSocket
 * Purpose: Check if any new clients are trying to establish their TCP connection
 * Input:
 * - Data structure to store client info in if there is an incoming connection
 * - Debug flag
 * Output:
 * - 0: No incoming TCP connections
 * - 1: There is an incoming connection. Its info has been stored in incomingAddress.
 */
int checkTcpSocket(int tcpSocketDescriptor,
                   struct sockaddr_in* incomingAddress,
                   uint8_t debugFlag) {
  if (debugFlag) {
  }
  socklen_t incomingAddressLength = sizeof(incomingAddress);
  tcpSocketDescriptor   = accept(tcpSocketDescriptor, (struct sockaddr*)incomingAddress,
                                 &incomingAddressLength);
  int nonBlockingReturn = handleErrorNonBlocking(tcpSocketDescriptor);

  if (nonBlockingReturn == 0) { // Data to be read
    if (debugFlag) {
      printf("Incoming TCP connection\n");
    }
    return 1; // Return 1
  }
  else { // No data to be read
    if (debugFlag) {
      printf("No incoming TCP connection\n");
    }

    return 0; // Return 0
  }
}

/*
 * Purpose: Connect to another IPV4 socket via TCP
 * Input:
 * - Name of the node to connect to
 * - Socket on calling node process
 * - addrinfo structure containing information about node to connect to
 * Output:
 * - Connected socket descriptor
 */
int tcpConnect(const char* nodeName,
               int socketDescriptor,
               struct sockaddr* destinationAddress,
               socklen_t destinationAddressLength) {
  printf("Connecting to %s...\n", nodeName);
  int connectionStatus;
  connectionStatus =
      connect(socketDescriptor, destinationAddress, destinationAddressLength);
  // Check if connection was successful
  if (connectionStatus != 0) {
    char* errorMessage = malloc(1024);
    strcpy(errorMessage, strerror(errno));
    printf("Connection to %s failed with error %s\n", nodeName, errorMessage);
    exit(1);
  }
  printf("Connected to %s...\n", nodeName);
  return socketDescriptor;
}
