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
void checkCommandLineArguments(int argc, char **argv, bool *debugFlag) {
  char *programName = argv[0];
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
    } else {
      printf("Invalid usage of %s", programName);
    }
    break;

  // Invalid
  default:
    printf("Invalid usage of %s", programName);
  }
}

/*
 * Purpose: Send a message via UDP
 * Input:
 * - Socket to send the message out on
 * - Socket address to send the message to
 * - The message to send
 * - Debug flag
 * Output: None
 */
void sendUdpMessage(int udpSocketDescriptor,
                    struct sockaddr_in destinationAddress,
                    char *message,
                    bool debugFlag) {
  if (debugFlag) {
    printf("Sending UDP message:\n");
    printf("%s\n", message);
    printf("To %d:%d\n", ntohl(destinationAddress.sin_addr.s_addr),
           ntohs(destinationAddress.sin_port));
  }

  long int sendtoReturn = 0;
  sendtoReturn =
      sendto(udpSocketDescriptor, message, strlen(message), 0,
             (struct sockaddr *)&destinationAddress, sizeof(destinationAddress));
  if (sendtoReturn == -1) {
    perror("UDP send error");
    exit(1);
  } else if (debugFlag) {
    printf("UDP message sent\n");
  }
}

/*
 * Name: printReceivedMessage
 * Purpose: Print out a message along with where it came from
 * Input:
 * - Who sent the message
 * - How long the received message is
 * - The received message
 * - Debug flag
 * Output: None
 */
void printReceivedMessage(struct sockaddr_in sender,
                          long int bytesReceived,
                          char *message,
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
 * - Buffer to store the read contents
 * - Debug flag
 * Output:
 * - -1: Error
 * - 0: Success
 */
int readFile(char *fileName, char *buffer, bool debugFlag) {
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
int writeFile(char *fileName, char *fileContents, size_t fileSize) {
  int fileDesciptor;

  // Open file to write to
  // Create if doesn't exist. Read/write
  fileDesciptor = open(fileName, (O_CREAT | O_RDWR), S_IRWXU);
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
 * Name: setupUdpSocket
 * Purpose: Setup the UDP socket. Set it to non blocking. Bind it.
 * Input:
 * - Address structure to bind the socket to
 * - Flag indicating whether or not to bind the socket
 * Output: The setup UDP socket
 */
int setupUdpSocket(struct sockaddr_in serverAddress, bool bindFlag) {
  int udpSocketDescriptor;

  // Set up UDP socket
  printf("Setting up UDP socket...\n");
  udpSocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSocketDescriptor == -1) {
    perror("Error when setting up UDP socket");
    exit(1);
  }

  // Set non blocking
  int fcntlReturn =
      fcntl(udpSocketDescriptor, F_SETFL, O_NONBLOCK); // Set to non blocking
  if (fcntlReturn == -1) {
    perror("Error when setting UDP socket non blocking");
  }

  printf("UDP socket set up\n");

  if (!bindFlag) {              // If bind flag is not set
    return udpSocketDescriptor; // Return and do not bind the socket
  }
  // Bind UDP socket
  printf("Binding UDP socket...\n");
  int bindReturnUDP = bind(udpSocketDescriptor, (struct sockaddr *)&serverAddress,
                           sizeof(serverAddress)); // Bind
  if (bindReturnUDP == -1) {
    perror("Error when binding UDP socket");
    exit(1);
  }
  printf("UDP socket bound\n");
  return udpSocketDescriptor;
}

/*
 * Name: checkUdpSocket
 * Purpose: Check if there is message on a UDP port.
 * Input:
 * - Address of the UDP port that is receiving messages.
 * - If message is received, socket address data structure to store the senders
 * address in
 * - Buffer to read message into
 * - Debug flag
 * Output: None
 * 0: No incoming messages
 * 1: There is an incoming message
 */
int checkUdpSocket(int listeningUDPSocketDescriptor,
                   struct sockaddr_in *incomingAddress,
                   char *message,
                   bool debugFlag) {
  socklen_t incomingAddressLength = sizeof(incomingAddress);
  long int bytesReceived =
      recvfrom(listeningUDPSocketDescriptor, message, 250, 0,
               (struct sockaddr *)incomingAddress, &incomingAddressLength);
  int nonBlockingReturn = handleErrorNonBlocking((int)bytesReceived);

  // No incoming message
  if (nonBlockingReturn == 1) {
    return 0;
  }

  // Incoming message
  printReceivedMessage(*incomingAddress, bytesReceived, message, debugFlag);
  return 1;
}

/*
 * Name: handleErrorNonBlocking
 * Purpose: Check the return after checking a non blocking socket
 * Input: The return value from checking the socket
 * Output:
 * - 0: There is data waiting to be read
 * - 1: No data waiting to be read
 */
int handleErrorNonBlocking(int returnValue) {
  if (returnValue == -1) {
    // Errors occuring from no message on non blocking socket
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 1;
    } else { // Relevant error
      perror("Error when checking non blocking socket");
      exit(1);
      return 1;
    }
  } else { // Got a message
    return 0;
  }
}
