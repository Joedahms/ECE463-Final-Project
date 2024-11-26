#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "network_node.h"

/*
 * Name: checkCommandLineArguments
 * Purpose: Check for command line arguments when starting up a network node.
 * At the moment it is only used for setting the debug flag.
 * Input:
 * - Number of command line arguments
 * - The command line arguments
 * - Debug flag
 * Output: None
 */
void checkCommandLineArguments(int argc, char** argv, bool* debugFlag) {
  char* programName = argv[0];
  programName += 2;

  switch (argc) {
    // Normal
  case 1:
    printf("Running %s in normal mode\n", programName);
    break;

    // Debug mode
  case 2:
    if (strcmp(argv[1], "-d") == 0) {
      *debugFlag = 1;
      printf("Running %s in debug mode\n", programName);
    }
    else {
      printf("Invalid usage of %s", programName);
    }
    break;

  // Invalid
  default:
    printf("Invalid usage of %s", programName);
  }
}

/*
 * Name: printReceivedMessage
 * Purpose: Print out a message along with where it came from
 * Input:
 * - Who sent the message
 * - How long the received message is in bytes
 * - The received message
 * - Debug flag
 * Output: None
 */
void printReceivedMessage(struct sockaddr_in sender,
                          long int bytesReceived,
                          char* message,
                          bool debugFlag) {
  if (debugFlag) {
    unsigned long senderAddress = ntohl(sender.sin_addr.s_addr);
    unsigned short senderPort   = ntohs(sender.sin_port);
    printf("Received %ld byte message from %ld:%d:\n", bytesReceived, senderAddress,
           senderPort);
    printf("%s\n", message);
  }
}

/*
 * Name: readFile
 * Purpose: Open a file and read from it
 * Input:
 * - File name
 * - Buffer to store the read contents in
 * - Debug flag
 * Output:
 * - -1: Error
 * - 0: Success
 */
int readFile(char* fileName, char* buffer, bool debugFlag) {
  // Open the file
  int fileDescriptor;
  printf("Opening file %s...\n", fileName);

  // Create if does not exist + read and write mode
  fileDescriptor = open(fileName, O_CREAT, O_RDWR);
  if (fileDescriptor == -1) {
    perror("Error opening file");
    return -1;
  }
  printf("File %s opened\n", fileName);

  // Get the size of the file in bytes
  struct stat fileInformation;
  if (stat(fileName, &fileInformation) == -1) {
    perror("Error getting file size");
    return -1;
  };
  long int fileSize = fileInformation.st_size;
  if (debugFlag) {
    printf("%s is %ld bytes\n", fileName, fileSize);
  }

  // Read out the contents of the file
  printf("Reading file...\n");
  ssize_t bytesReadFromFile = 0;
  bytesReadFromFile         = read(fileDescriptor, buffer, (long unsigned int)fileSize);
  if (bytesReadFromFile == -1) {
    perror("Error reading file");
    return -1;
  }
  if (debugFlag) {
    printf("%zd bytes read from %s\n", bytesReadFromFile, fileName);
  }
  printf("File read successfully\n");
  return 0;
}

/*
 * Name: writeFile
 * Purpose: Open a file and write to it
 * Input:
 * - Name of the file
 * - What to write to the file
 * - Length of data to be written to the file
 * Output:
 * - -1: Error
 * - 0: Success
 */
int writeFile(char* filename, char* fileContents, size_t fileSize, bool debugFlag) {
  int fileDesciptor;
  if (debugFlag) {
    printf("Writing to file: %s...\n", filename);
    printf("File size: %ld\n", fileSize);
  }

  // Open file to write to
  // Create if doesn't exist. Read/write
  fileDesciptor = open(filename, (O_CREAT | O_RDWR), S_IRWXU);
  if (fileDesciptor == -1) {
    perror("Error opening file");
    return -1;
  }

  // Write to the new file
  long int writeReturn = write(fileDesciptor, fileContents, (long unsigned int)fileSize);
  if (writeReturn == -1) {
    perror("File write error");
    return -1;
  }
  return 0;
}

/*
 * Name: handleErrorNonBlocking
 * Purpose: Check the return after checking a non blocking socket
 * Input: The return value from checking the socket
 * Output:
 * - 0: There is data waiting to be read
 * - 1: No data waiting to be read
 */
int handleErrorNonBlocking(int returnValue, char* info) {
  if (returnValue == -1) {
    // Errors occuring from no message on non blocking socket
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 1;
    }
    else { // Relevant error
      printf("%s\n", info);
      perror("Error when checking non blocking socket");
      exit(1);
      return 1;
    }
  }
  else { // Got a message
    return 0;
  }
}
