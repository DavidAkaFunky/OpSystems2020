#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include "fs/operations.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100
#define NORMAL_FLAGS 0

int numberThreads = 0;

/* Socket server related global variables */
int scsocket;
struct sockaddr_un server_addr;
socklen_t ser_addr_len;

/**
 * Called when invalid commands are processed and exits program.
 */
void errorParse() {
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/**
 * Initializes TecnicoFS.
 * Input:
 *  - argc: number of input arguments from stdin.
 */ 
void init_fs_aux(int argc) {
    /* Validate number of input arguments */ 
    if (argc != 3) {
        fprintf(stderr, "Usage: ./tecnicofs numberthreads socketname\n");
        exit(EXIT_FAILURE);
    }
    init_fs();
}

/* 
 * Prints TecnicoFS tree.
 * Input:
 *  - outputPath: string containing the output file path
 */
void print_tecnicofs_tree_aux(char* outputPath) {
    FILE* fp = fopen(outputPath, "w");
    if (!fp) {
        fprintf(stderr, "Output path invalid, please try again!\n");
        exit(EXIT_FAILURE);
    }
    print_tecnicofs_tree(fp);
    fclose(fp);
}

int setSocketAddressUn(char * path, struct sockaddr_un * addr) {
    if (addr == NULL) { return 0; }
    bzero((char *) addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);
    return SUN_LEN(addr);
}

void init_socket(char * socketPath) {

    if ((scsocket = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Server: Error opening socket\n");
        exit(EXIT_FAILURE);
    }

    /* Delete socketname from fs if it exists */
    unlink(socketPath);

    /* Erase all data in server address and initialize struct */
    ser_addr_len = setSocketAddressUn(socketPath, &server_addr); 

    /* Bind a name to the server socket */
    if (bind(scsocket, (struct sockaddr *) &server_addr, ser_addr_len) == -1) {
        fprintf(stderr, "Server: Error binding name to socket\n");
        exit(EXIT_FAILURE);
    }

    /* Give the socket writing permissions */
    if (chmod(socketPath, 222) == -1) {
        perror("Server: can't change permissions of socket");
        exit(EXIT_FAILURE);
    }

}

/**
 * Assigns number of threads inputted to global variable
 * numberThreads.
 */
void validateNumThreads(char * numThreads) {
    int n = atoi(numThreads);
    /* atoi error */
    if (n <= 0) {
        fprintf(stderr, "Invalid input of numThreads!\n");
        exit(EXIT_FAILURE);
    }
    numberThreads = n;
}

/**
 * Calls operations on different commands read from "inputCommands" buffer
 */
void applyCommands() {
    while (1) {
        struct sockaddr_un client_addr;
        socklen_t addr_len;

        char command[MAX_INPUT_SIZE];
        int opReturn; 
        int bytesReceived;

        addr_len = sizeof(struct sockaddr_un);
        bytesReceived = recvfrom(scsocket, command, sizeof(command), NORMAL_FLAGS,
                        (struct sockaddr *) &client_addr, &addr_len);
        
        if (bytesReceived < 0) {
            /* Case where peer has perform an orderly shutdown or error has occured */
            fprintf(stderr, "Server: error receiving message from client\n");
            exit(EXIT_FAILURE);
        }
        
        command[bytesReceived] = '\0';
        /* Update client socket - HARDCODED ? */
        addr_len = setSocketAddressUn("../client/csocket", &client_addr);
        
        char token;
        char name[MAX_INPUT_SIZE], arg[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %s", &token, name, arg);
        if (numTokens < 2) {
            fprintf(stderr, "Server: invalid command :%s: in Queue\n", command);
            exit(EXIT_FAILURE);
        }

        switch (token) {
            case 'c':
                switch (arg[0]) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        opReturn = create_aux(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        opReturn = create_aux(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                printf("Searching %s...\n", name);
                opReturn = lookup_aux(name);
                /* if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name); */
                break;
            case 'd':
                printf("Delete: %s\n", name);
                opReturn = delete_aux(name);
                break;
            case 'm':
                printf("Move: %s to %s\n", name, arg);
                opReturn = move_aux(name, arg);
                break;
            case 'p':
                print_tecnicofs_tree_aux(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        if (sendto(scsocket, &opReturn, sizeof(opReturn), NORMAL_FLAGS, (struct sockaddr *) &client_addr, addr_len) < 0) {
            perror("Server: error sending operation return to client");
            exit(EXIT_FAILURE);
        }
    }
}


/*
 * Initializes the thread pool.
 * Input:
 *  - inputPath: string containing the input file path
 */
void startThreadPool() {
    pthread_t tid[numberThreads];
    /* Create consuming threads */
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, (void *) applyCommands, NULL) != 0) {
            fprintf(stderr, "Thread %d failed to create\n", i);
            exit(EXIT_FAILURE);
        } else {
            printf("Thread %d created\n", i);
        }
    }
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Thread failed to join!\n");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[]) {

    /* Initializes TecnicoFS and i-node table */
    init_fs_aux(argc);
    init_socket(argv[2]);

    /* TecnicoFS execution */
    validateNumThreads(argv[1]);
    startThreadPool();

    /* Destroys TecnicoFS and i-node table */
    destroy_fs();

    exit(EXIT_SUCCESS);
}