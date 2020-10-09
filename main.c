#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse() {
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void print_tecnicofs_tree_aux(char* outputPath) {
    FILE* fp = fopen(outputPath, "w");
    if (!fp) {
        perror("Output path invalid, please try again!\n");
        exit(EXIT_FAILURE);
    }
    print_tecnicofs_tree(fp);
    fclose(fp);
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
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
}

void processInput_aux(char* inputPath) {
    FILE* fp = fopen(inputPath, "r");
    if (!fp) {
        perror("Input path invalid, please try again!\n");
        exit(EXIT_FAILURE);
    }
    processInput(fp);
    fclose(fp);
}

void applyCommands(){
    while (numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

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
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*
 * Initializes the thread pool.
 */

void startThreadPool(char* numthreads, char* syncStrat){
    int n = atoi(numthreads), i;
    if (n <= 0){
        perror("Invalid value, please try again!\n");
        exit(EXIT_FAILURE);
    }
    if (!strcmp("nosync", syncStrat) && n != 1){
        perror("Invalid value, please try again!\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        pthread_t* tid = (pthread_t*) malloc (sizeof(pthread_t) * n);
        for (i = 0; i < n; i++){
            if(!pthread_create(tid + i, NULL, (void*) applyCommands, NULL))
                printf("Thread number %ld created successfully.\n", tid[i]);
            else
            {
                perror("Thread not created!");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput_aux(argv[1]);
    startThreadPool(argv[3], argv[4]);
    applyCommands();
    print_tecnicofs_tree_aux(argv[2]);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
