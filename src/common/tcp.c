
#include <errno.h>
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
  numberOfBytesReceived = recv(socketDescriptor, incomingFileContents, MAX_FILE_SIZE, 0);

  int writeFileReturn =
      writeFile(fileName, incomingFileContents, (long unsigned int)numberOfBytesReceived);
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
