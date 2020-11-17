#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs/operations.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
int numberCommands = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];

struct timeval begin, end;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condProd = PTHREAD_COND_INITIALIZER, condCons = PTHREAD_COND_INITIALIZER;
int insert = 0, headQueue = 0;

/**
 * Inserts commands from input file to buffer inputCommands
 * Input:
 *  - data: line from input file to be added to buffer.
 */
void insertCommand(char* data) {
    pthread_mutex_lock(&mutex);
    while (numberCommands == MAX_COMMANDS) {
        pthread_cond_wait(&condProd, &mutex);
    }
    strcpy(inputCommands[insert++], data);
    insert %= MAX_COMMANDS;
    numberCommands++;
    pthread_cond_signal(&condCons);
    pthread_mutex_unlock(&mutex);
}

/**
 * Reads commands from buffer.
 * Return:
 *  - command: string in headQueue index of the buffer
 */

char * removeCommand() {
    char * command = inputCommands[headQueue];
    if (numberCommands > 0) {
        numberCommands--;
        headQueue = (headQueue + 1) % MAX_COMMANDS;
        return command;
    }
    return NULL;
}

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
    if (argc != 4) {
        fprintf(stderr, "Usage: ./tecnicofs inputfile outputfile numthreads\n");
        exit(EXIT_FAILURE);
    }
    init_fs();
}

/**
 * Prints TecnicoFS execution time and destroys TecnicoFS and i-node table
 */
void destroy_fs_aux() {
    if (pthread_mutex_destroy(&mutex)) {
        fprintf(stderr, "Error destroying mutex!\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_destroy(&condProd)) {
        fprintf(stderr, "Error destroying producer conditional variable!\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_destroy(&condCons)) {
        fprintf(stderr, "Error destroying consumer conditional variable!\n");
        exit(EXIT_FAILURE);
    }
    destroy_fs();
    printf ("TecnicoFS completed in %.4f seconds.\n",
         (double) (end.tv_usec - begin.tv_usec) / 1000000 +
         (double) (end.tv_sec - begin.tv_sec));
}

/**
 * Assigns number of threads inputted to global variable
 * numberThreads.
 */
void validateNumThreads(char* numThreads) {
    int n = atoi(numThreads);
    /* atoi error */
    if (n <= 0) {
        fprintf(stderr, "Invalid input of numThreads!\n");
        exit(EXIT_FAILURE);
    }
    numberThreads = n;
}

/**
 * Alternative to global flag.
 * Injects commands to let consumer threads know that the buffer has been all read.
 */
void EOFProcess() {
    for (int i = 0; i < numberThreads; ++i) {
        insertCommand("x x x");
    }
    pthread_cond_broadcast(&condCons);
}

/**
 * Processes each line from file "fp" and adds it to inputCommands buffer.
 * Input:
 *  - fp: input file where each line is a command to a certain operation
 */
void processInput(FILE* fp) {
    char line[MAX_INPUT_SIZE];
    while (fgets(line, sizeof(line)/sizeof(char), fp)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* Function sscanf returns 0 or EOF if input is invalid */
        if (numTokens < 1) {
            continue;
        }
        /* Process each line and give it to the PRODUCER */
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                insertCommand(line);
                break;            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                insertCommand(line);
                break;            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                insertCommand(line);
                break;
            case 'm':
                if(numTokens != 3)
                    errorParse();
                insertCommand(line);
                break;        
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
    EOFProcess();
}

/* 
 * Processes input to buffer.
 * Input:
 *  - inputPath: string containing the input file path
 */
void processInput_aux(char* inputPath) {
    FILE* fp = fopen(inputPath, "r");
    if (!fp) {
        fprintf(stderr, "Input path invalid, please try again!\n");
        exit(EXIT_FAILURE);
    }
    processInput(fp);
    fclose(fp);
}

/**
 * Calls operations on different commands read from "inputCommands" buffer
 */
void applyCommands() {
    while (true) {
        pthread_mutex_lock(&mutex);
        while (numberCommands == 0) pthread_cond_wait(&condCons, &mutex);
        const char * commandTemp = removeCommand();
        if (commandTemp == NULL) {
            /* Release producer thread */
            pthread_cond_signal(&condProd);
            pthread_mutex_unlock(&mutex);
            continue;
        }
        /* to free inputCommands asap */
        char command[MAX_INPUT_SIZE];
        strcpy(command, commandTemp);

        pthread_cond_signal(&condProd);
        pthread_mutex_unlock(&mutex);
        
        char token, type;
        char name[MAX_INPUT_SIZE], arg[MAX_INPUT_SIZE];

        int numTokens = sscanf(command, "%c %s %s", &token, name, arg);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command :%s: in Queue\n", command);
            exit(EXIT_FAILURE);
        }

        /* Create third argument is either "d" or "f" */
        type = arg[0];
        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create_aux(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create_aux(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                printf("Searching %s...\n", name);
                searchResult = lookup_aux(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete_aux(name);
                break;
            case 'm':
                printf("Move: %s to %s\n", name, arg);
                move_aux(name, arg);
                break;
            case 'x':
                return;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}


/*
 * Initializes the thread pool.
 * Input:
 *  - inputPath: string containing the input file path
 */
void startThreadPool(char* inputPath) {
    pthread_t tid[numberThreads];
    /* Create consuming threads */
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, (void *) applyCommands, NULL) != 0) {
            fprintf(stderr, "Thread not created!\n");
            exit(EXIT_FAILURE);
        }
    }
    /* Start the clock when processing input starts */
    gettimeofday(&begin, NULL);
    processInput_aux(inputPath);
    /* End the clock after processing input */
    gettimeofday(&end, NULL);
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Thread %d failed to join!\n", i);
            exit(EXIT_FAILURE);
        }
    }
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

int main(int argc, char* argv[]) {

    /* Initializes TecnicoFS and i-node table */
    init_fs_aux(argc);

    validateNumThreads(argv[3]);
    startThreadPool(argv[1]);
    print_tecnicofs_tree_aux(argv[2]);

    /* Destroys TecnicoFS and i-node table */
    destroy_fs_aux();

    exit(EXIT_SUCCESS);
}