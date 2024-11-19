// Template for function comments
/*
 * Purpose:
 * Input:
 * Output:
 */

#ifndef NETWORK_NODE_H
#define NETWORK_NODE_H

// Port server is listening on
#define PORT                 3941

#define MAX_FILENAME         50

#define MAX_FILE_SIZE        5000

#define INITIAL_MESSAGE_SIZE 200

// Max size of message a user can input
#define MAX_USER_INPUT 40

// Max size of a username
#define MAX_USERNAME 20

// Max size of a string containing the available resources on a client
#define MAX_RESOURCE_ARRAY 2000

#define STATUS_SIZE        10

#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

void checkCommandLineArguments(int, char **, bool *);
void sendUdpMessage(int, struct sockaddr_in, char *, bool);
void printReceivedMessage(struct sockaddr_in, long int, char *, bool);

// File I/O
int readFile(char *, char *, bool);
int writeFile(char *, char *, size_t);

int setupUdpSocket(struct sockaddr_in, bool);
int checkUdpSocket(int, struct sockaddr_in *, char *, bool);
int handleErrorNonBlocking(int);

#endif
