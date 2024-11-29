
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
  bytesSent          = send(socketDescriptor, buffer, bufferSize, MSG_NOSIGNAL);
  printf("here");
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
  if (debugFlag) {
    printf("Sending file %s over TCP...\n", fileName);
  }
  char* fileContents = malloc(MAX_FILE_SIZE);
  int readFileReturn = readFile(fileName, fileContents, debugFlag);
  if (readFileReturn == -1) {
    printf("Put command error when reading file");
  }
  if (debugFlag) {
    printf("sending %ld bytes\n", strlen(fileContents));
    printf("File contents to send:\n%s", fileContents);
  }
  long int bytesSent =
      tcpSendBytes(socketDescriptor, fileContents, strlen(fileContents), debugFlag);
  if (debugFlag) {
    printf("Sent %ld bytes\n", bytesSent);
  }
}

// request test01.txt

/*
 * Purpose: Receive file from connection and store into local file
 * Input:
 * - Socket descriptor to receive the file on
 * - File name of requested file
 * - Debug flag
 * Output:
 * - -1: failure
 * - 0: Success
 */
long int tcpReceiveFile(int socketDescriptor, char* fileName, bool debugFlag) {
  printf("Receiving file...\n");

  // Allocate buffer for incoming file contents
  char* incomingFileContents = malloc(MAX_FILE_SIZE);
  if (!incomingFileContents) {
    perror("Error allocating memory for file contents");
    return -1;
  }

  long int totalBytesReceived = 0;

  // Receive data until the end of the file or buffer is full
  while (totalBytesReceived < MAX_FILE_SIZE) {
    long int bytesReceived =
        recv(socketDescriptor, incomingFileContents + totalBytesReceived,
             MAX_FILE_SIZE - totalBytesReceived, 0);

    if (bytesReceived == -1) {
      perror("Error receiving file");
      free(incomingFileContents);
      return -1;
    }

    if (bytesReceived == 0) {
      // Connection closed by peer
      break;
    }

    totalBytesReceived += bytesReceived;
  }

  if (debugFlag) {
    printf("Received file is %ld bytes\n", totalBytesReceived);
  }

  // Write received data to file
  int writeFileReturn = writeFile(fileName, incomingFileContents, totalBytesReceived, debugFlag);
  free(incomingFileContents); // Free allocated memory

  if (writeFileReturn == -1) {
    printf("Failed to write file %s\n", fileName);
    return -1;
  }

  printf("File successfully received and written: %s\n", fileName);
  return totalBytesReceived;
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
 * -1: No incoming TCP connections
 * Not -1: The connected TCP socket descriptor
 */
int checkTcpSocket(int listeningTcpSocketDescriptor,
                   struct sockaddr_in* incomingAddress,
                   uint8_t debugFlag) {
  if (debugFlag) {
    struct sockaddr_in* test = incomingAddress;
    if (test->sin_port) {
    }
  }
  //  socklen_t incomingAddressLength = sizeof(*incomingAddress);
  int connectedTcpSocketDescriptor =
      accept(listeningTcpSocketDescriptor, (struct sockaddr*)NULL, NULL);
  printf("%d\n", connectedTcpSocketDescriptor);
  int returnCheck       = connectedTcpSocketDescriptor;
  int nonBlockingReturn = handleErrorNonBlocking(returnCheck, "tcp");

  if (nonBlockingReturn == 0) { // Incoming TCP connection
    if (debugFlag) {
      printf("New TCP connection established\n");
      printf("Connected socket descriptor: %d\n", connectedTcpSocketDescriptor);
    }
    return connectedTcpSocketDescriptor; // Return 1
  }
  else {       // No data to be read
    return -1; // Return 0
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
