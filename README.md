# ECE463 Intro to Computer Communication Networks project
# Phase 3: Decentralized P2P Network

## Directories/files
src:        Directory containing all source code
.gitignore: File listings all files and directories to ignore when committing changes via Git
makefile:   See Compilation section below
Flowcharts: Directory containing flowcharts that illustrate the logical flow of our project.  
The flowcharts were made and can be modified with [draw.io](https://app.diagrams.net/).

## Usage (May be different for this phase)
Both client and server support the use of the -d flag for debugging purposes. If used, it will
print extra information about the operation of the program to stdout.
### Server
After compilation, change to the server_test_directory and run the server executable.
### Client
After compilation, change to the client_test_directory and run the client executable. Any files that you want
to make available for file sharing should be put in the Public folder.

## Compilation
This project uses make for compilation. Enter "make" to compile the program, "make clean" to remove all object  
files and executables. Any files/directories that are created when compiling are listed in the gitignore.

## Requirements
- Server provides user directory. Contains all user usernames, IP addresses, and statuses.
- Server provides resource directory. Contains resource names and owners.
- Server sends hello message to client to check its liveness. Done periodically to check "liveness"
- All communication between client and server is done through UDP.
- Do not implement communication between clients. Will be implemented in the next stage.
- Optional: Clients can take a script for testing purposes.

## Code Style Conventions
- Types camel case with first character uppercase
- Variables and functions camel case
- Single space after commas
- Single space before and after =

