#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdio.h>

int scsocket;
struct sockaddr_un client_addr, server_addr;
socklen_t cli_addr_len, ser_addr_len;

int tfsCreate(char *filename, char nodeType) {
  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "c %s %c", filename, nodeType);
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) < 0) {
    perror("Client: Error sending in tfsCreate");
    exit(EXIT_FAILURE);
  }
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) < 0) {
    perror("Client: Error receiving in tfsCreate");
    exit(EXIT_FAILURE);
  }
  return opReturn;
}

int tfsDelete(char *path) {
  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "d %s", path);
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) < 0) {
    perror("Client: Error sending in tfsDelete");
    exit(EXIT_FAILURE);
  }
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) < 0) {
    perror("Client: Error receiving in tfsDelete");
    exit(EXIT_FAILURE);
  }
  return opReturn;
}

int tfsMove(char *from, char *to) {
  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "m %s %s", from, to);
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) < 0) {
    perror("Client: Error sending in tfsMove");
    exit(EXIT_FAILURE);
  }
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) < 0) {
    perror("Client: Error receiving in tfsMove");
    exit(EXIT_FAILURE);
  }
  return opReturn;
}

int tfsLookup(char *path) {
  char command[MAX_INPUT_SIZE];
  int opReturn;
  sprintf(command, "l %s", path);
  if (sendto(scsocket, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, ser_addr_len) < 0) {
    perror("Client: Error sending in tfsLookup");
    exit(EXIT_FAILURE);
  }
  if (recvfrom(scsocket, &opReturn, sizeof(opReturn), 0, 0, 0) < 0) {
    perror("Client: Error receiving in tfsLookup");
    exit(EXIT_FAILURE);
  }
  return opReturn;
}

int setSocketAddresUn(char * path, struct sockaddr_un * addr) {
  if (addr == NULL) { return 0; }
  bzero((char *) addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);
  return SUN_LEN(addr);
}

int tfsMount(char * sockPath) {
  char client_file[MAX_FILE_NAME] = {"csocket"};

  if ((scsocket = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    fprintf(stderr, "Client: Error opening server socket\n");
    return EXIT_FAILURE;
  }

  unlink(client_file);

  cli_addr_len = setSocketAddresUn(client_file, &client_addr);
  if (bind(scsocket, (struct sockaddr *) &client_addr, cli_addr_len) < 0) {
    fprintf(stderr, "Client: Error binding client socket\n");
    return EXIT_FAILURE;
  }

  if (chmod(client_file, 222) == -1) {
    perror("Client: Can't change socket's permissions");
    return EXIT_FAILURE;
  }

  ser_addr_len = setSocketAddresUn(sockPath, &server_addr);
  
  return EXIT_SUCCESS;
}

int tfsUnmount() {
  close(scsocket);
  return EXIT_SUCCESS;
}
