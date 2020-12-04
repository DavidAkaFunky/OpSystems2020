#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

int scsocket;
struct sockaddr_un client_addr, server_addr;
socklen_t cli_addr_len, ser_addr_len;

/**
 * Creates a file/directory.
 * Inputs:
 *  - name: The new file/directory's name.
 *  - nodeType: Used to choose what to create (file or directory).
 */
int tfsCreate(char *name, char nodeType) {

  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "c %s %c", name, nodeType);
  /* Send command to server */
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) == -1) {
    perror("Client: Error sending in tfsCreate");
    exit(EXIT_FAILURE);
  }
  /* Receive command operation return from server */
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) == -1) {
    perror("Client: Error receiving in tfsCreate");
    exit(EXIT_FAILURE);
  }

  return opReturn;
}

/**
 * Deletes a file/directory.
 * Input:
 *  - path: The file/directory to be deleted's path.
 */
int tfsDelete(char *path) {

  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "d %s", path);
  /* Send command to server */
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) == -1) {
    perror("Client: Error sending in tfsDelete");
    exit(EXIT_FAILURE);
  }
  /* Receive command operation return from server */
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) == -1) {
    perror("Client: Error receiving in tfsDelete");
    exit(EXIT_FAILURE);
  }
  return opReturn;
}

/**
 * Moves a file/directory.
 * Inputs:
 *  - from: Original location.
 *  - to: New location.
 */
int tfsMove(char *from, char *to) {

  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "m %s %s", from, to);
  /* Send command to server */
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) < 0 ) {
    perror("Client: Error sending in tfsMove");
    exit(EXIT_FAILURE);
  }
  /* Receive command operation return from server */
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) == -1) {
    perror("Client: Error receiving in tfsMove");
    exit(EXIT_FAILURE);
  }

  return opReturn;
}

/**
 * Searches for a file/directory.
 * Input:
 *  - path: The file/directory to be searched's path.
 */
int tfsLookup(char *path) {

  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "l %s", path);
  /* Send command to server */
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) == -1) {
    perror("Client: Error sending in tfsLookup");
    exit(EXIT_FAILURE);
  }
  /* Receive command operation return from server */
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) == -1) {
    perror("Client: Error receiving in tfsLookup");
    exit(EXIT_FAILURE);
  }

  return opReturn;
}

/**
 * Prints the whole file system's tree to a file.
 * Input:
 *  - path: The file/directory to be deleted's path.
 */
int tfsPrint(char * path) {

  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "p %s", path);
  /* Send command to server */
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) == -1) {
    perror("Client: Error sending in tfsPrint");
    exit(EXIT_FAILURE);
  }
  /* Receive command operation return from server */
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) == -1) {
    perror("Client: Error receiving in tfsPrint");
    exit(EXIT_FAILURE);
  }

  return opReturn;
}

/**
 * Resets the socket address and defines its family and given path.
 * Input:
 *  - path : address path.
 *  - addr : memory address of socket's address.
 */
int setSocketAddressUn(char * path, struct sockaddr_un * addr) {
  if (addr == NULL) { return 0; }
  bzero((char *) addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);
  return SUN_LEN(addr);
}

/**
 * Connects to the server socket.
 * Input:
 *  - sockPath: the server socket's path.
 */
int tfsMount(char * sockPath) {

  char client_file[MAX_FILE_NAME];
  sprintf(client_file, "/tmp/clientSocket-%d", getpid());

  if ((scsocket = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    perror("Client: Error opening server socket");
    return EXIT_FAILURE;
  }

  if (unlink(client_file) == -1) {
        if (errno != ENOENT) {
            perror("Error: Error unlinking server socket path");
            exit(EXIT_FAILURE);
        }
    }

  cli_addr_len = setSocketAddressUn(client_file, &client_addr);

  if (bind(scsocket, (struct sockaddr *) &client_addr, cli_addr_len) == -1) {
    perror("Client: Error binding client socket");
    return EXIT_FAILURE;
  }
  
  // No need to chmod because /tmp has read, write and execute rights for every file.

  ser_addr_len = setSocketAddressUn(sockPath, &server_addr);
  
  return EXIT_SUCCESS;

}

/**
 * Closes the client socket.
 */
int tfsUnmount() {
  close(scsocket);
  return EXIT_SUCCESS;
}
