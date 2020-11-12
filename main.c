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

// Produtor = insertCommand
// Consumidor = removeCommand

int numberThreads = 0;
int numberCommands = 0;
bool allInserted = false;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];

struct timeval tv1, tv2;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condIns = PTHREAD_COND_INITIALIZER, condRem = PTHREAD_COND_INITIALIZER;
int counter = 0;
int condptr = 0, condrem = 0;

void insertCommand(char* data) {
    pthread_mutex_lock(&mutex);
    while (counter == MAX_COMMANDS){
        pthread_cond_wait(&condIns, &mutex);
    }
    strcpy(inputCommands[condptr], data);
    condptr = (condptr + 1) % MAX_COMMANDS;
    counter++;
    pthread_cond_signal(&condRem);
    pthread_mutex_unlock(&mutex);
}

void removeCommand(char* cmd) {
    while (!allInserted && counter == 0)
        pthread_cond_wait(&condRem, &mutex);
    if (allInserted && counter == 0)
        return;
    strcpy(cmd, inputCommands[condrem]);
    condrem = (condrem + 1) % MAX_COMMANDS;
    counter--;
    pthread_cond_signal(&condIns);
}


void errorParse() {
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void validateNumThreads(char* numThreads){
    int n = atoi(numThreads);
    /* atoi error */
    if (n <= 0) {
        fprintf(stderr, "Invalid input of numThreads!\n");
        exit(EXIT_FAILURE);
    }
    numberThreads = n;
}

void processInput(FILE* fp){
    char line[MAX_INPUT_SIZE];
    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), fp)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* sscanf returns 0 or EOF if input is invalid */
        if (numTokens < 1) {
            continue;
        }
        /* adds commands to inputCommands array */
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
    pthread_mutex_lock(&mutex);
    allInserted = true;
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&condIns);
}

/* 
 * Checks if the file exists, processes 
 * each line of commands, and closes it. 
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


void applyCommands() {
    int activeLocks[INODE_TABLE_SIZE], j = 0;
    while (true){
        pthread_mutex_lock(&mutex);
        if (allInserted && counter == 0){
            pthread_mutex_unlock(&mutex);
            break;
        }
        char command[MAX_INPUT_SIZE];
        removeCommand(command);
        if (command == NULL){
            continue;
        }
        pthread_mutex_unlock(&mutex);
        char token, type;
        char name[MAX_INPUT_SIZE], arg[MAX_INPUT_SIZE];

        int numTokens = sscanf(command, "%c %s %s", &token, name, arg);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command :%s: in Queue\n", command);
            exit(EXIT_FAILURE);
        }
        if (numTokens == 3 && strlen(arg) == 1)
            type = arg[0];
        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                printf("Searching %s...\n", name);
                searchResult = lookup(name, activeLocks, &j, false);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                unlockAll(activeLocks, j);
                /* reset array after each iteration */
                memset(activeLocks, 0, sizeof(activeLocks));
                j=0;
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            case 'm':
                printf("Move: %s to %s\n", name, arg);
                move(name, arg);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    pthread_cond_broadcast(&condRem);
}


/*
 * Initializes the thread pool.
 */
void startThreadPool(char* inputPath){
    pthread_t tid[numberThreads];
    for (int i = 0; i < numberThreads; i++) {
        //printf("%d\n",i);
        if (pthread_create(&tid[i], NULL, (void*) applyCommands, NULL) != 0){
            fprintf(stderr, "Thread not created!\n");
            exit(EXIT_FAILURE);
        }
        //printf("Thread %d created!\n", i);
    }
    /* start the clock right after pool thread creation */
    gettimeofday(&tv1, NULL);
    processInput_aux(inputPath);
    /* end the clock */
    gettimeofday(&tv2, NULL);
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Thread %d failed to join!\n", i);
            exit(EXIT_FAILURE);
        }
    }
}

/* 
 * Tries creating a new file (overwriting the original content
 * if already existing), print the FS tree, and close it.
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
    /* check argc */ 
    if (argc != 4) {
        fprintf(stderr, "Usage: ./tecnicofs inputfile outputfile numthreads\n");
        exit(EXIT_FAILURE);
    }

    /* init filesystem and dinamically initialize mutex if needed */
    init_fs();
    validateNumThreads(argv[3]);
    
    startThreadPool(argv[1]);

    print_tecnicofs_tree_aux(argv[2]);

    /* release allocated memory */
    destroy_fs();

    printf ("TecnicoFS completed in %.4f seconds.\n",
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));

    exit(EXIT_SUCCESS);
}