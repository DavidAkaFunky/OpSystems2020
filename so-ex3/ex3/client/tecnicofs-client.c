#include <stdio.h>
#include <stdlib.h>
#include "tecnicofs-client-api.h"
#include "../tecnicofs-api-constants.h"

FILE* inputFile;
char* serverName;

/**
 * Shows how to run the client program.
 * Input:
 *  - appName: the program's name.
 */
static void displayUsage (const char* appName) {

    printf("Usage: %s inputfile server_socket_name\n", appName);
    exit(EXIT_FAILURE);

}

/**
 * Asserts the correct definition of the program's arguments
 * and attributes values to the global variables.
 */ 
static void parseArgs (long argc, char* const argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }

    serverName = argv[2];

    inputFile = fopen(argv[1], "r");

    if (inputFile == NULL) {
        fprintf(stderr, "Error: cannot open input file\n");
        exit(EXIT_FAILURE);
    }

}

void errorParse(){

    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);

}

/**
 * Reads all lines from the input file and calls functions
 * that send the correct command to the server socket.
 */
void * processInput() {

    char line[MAX_INPUT_SIZE];

    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {

        char op;
        char arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
        int res;

        int numTokens = sscanf(line, "%c %s %s", &op, arg1, arg2);

        /* Perform minimal validation */
        if (numTokens < 2) {
            continue;
        }

        switch (op) {
            case 'c': /* Create */
                if(numTokens != 3) {
                    errorParse();
                    break;
                }
                switch (arg2[0]) {
                    case 'f':
                        res = tfsCreate(arg1, 'f');
                        if (!res)
                          printf("Created file: %s\n", arg1);
                        else
                          printf("Unable to create file: %s\n", arg1);
                        break;
                    case 'd':
                        res = tfsCreate(arg1, 'd');
                        if (!res)
                          printf("Created directory: %s\n", arg1);
                        else
                          printf("Unable to create directory: %s\n", arg1);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                }
                break;
            case 'l': /* Lookup */
                if(numTokens != 2)
                    errorParse();
                res = tfsLookup(arg1);
                if (res >= 0)
                    printf("Search: %s found\n", arg1);
                else
                    printf("Search: %s not found\n", arg1);
                break;
            case 'd': /* Delete */
                if(numTokens != 2)
                    errorParse();
                res = tfsDelete(arg1);
                if (!res)
                  printf("Deleted: %s\n", arg1);
                else
                  printf("Unable to delete: %s\n", arg1);
                break;
            case 'm': /* Move */
                if(numTokens != 3)
                    errorParse();
                res = tfsMove(arg1, arg2);
                if (!res)
                  printf("Moved: %s to %s\n", arg1, arg2);
                else
                  printf("Unable to move: %s to %s\n", arg1, arg2);
                break;
            case 'p': /* Print */
                if(numTokens != 2)
                    errorParse();
                res = tfsPrint(arg1);
                if (!res)
                  printf("Printed: %s\n", arg1);
                else
                  printf("Unable to print: %s\n", arg1);
                break;
            case '#':
                break;
            default: { /* Error */
                errorParse();
            }
        }

    }

    fclose(inputFile);
    return NULL;

}

int main(int argc, char* argv[]) {

    parseArgs(argc, argv);

    if (tfsMount(serverName) == 0)
      printf("Mounted! (socket = %s)\n", serverName);
    else {
      fprintf(stderr, "Unable to mount socket: %s\n", serverName);
      exit(EXIT_FAILURE);
    }

    processInput();

    tfsUnmount();

    exit(EXIT_SUCCESS);

}
