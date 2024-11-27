#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "udp.h"

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
                    char* message,
                    bool debugFlag) {
  if (debugFlag) {
    printf("\nSending UDP message:\n");
    printf("%s\n", message);
    printf("To %d:%d\n", ntohl(destinationAddress.sin_addr.s_addr),
           ntohs(destinationAddress.sin_port));
  }

  long int sendtoReturn = 0;
  sendtoReturn =
      sendto(udpSocketDescriptor, message, strlen(message), 0,
             (struct sockaddr*)&destinationAddress, sizeof(destinationAddress));
  if (sendtoReturn == -1) {
    perror("UDP send error");
    exit(1);
  }
  else if (debugFlag) {
    printf("UDP message sent\n\n");
  }
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

  // Maybe bind UDP socket
  if (!bindFlag) {
    return udpSocketDescriptor;
  }
  printf("Binding UDP socket...\n");
  int bindReturnUDP = bind(udpSocketDescriptor, (struct sockaddr*)&serverAddress,
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
 * true: There is an incoming message
 * false: No incoming messages
 */
bool checkUdpSocket(int listeningUDPSocketDescriptor,
                    struct sockaddr_in* incomingAddress,
                    char* message,
                    bool debugFlag) {
  socklen_t incomingAddressLength = sizeof(incomingAddress);
  long int bytesReceived =
      recvfrom(listeningUDPSocketDescriptor, message, 250, 0,
               (struct sockaddr*)incomingAddress, &incomingAddressLength);
  int nonBlockingReturn = handleErrorNonBlocking((int)bytesReceived, "udp");

  // No incoming message
  if (nonBlockingReturn == 1) {
    return false;
  }

  // Incoming message
  printReceivedMessage(*incomingAddress, bytesReceived, message, debugFlag);
  return true;
}
