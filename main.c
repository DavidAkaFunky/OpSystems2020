#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs/operations.h"


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

struct timeval  tv1, tv2;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    pthread_mutex_lock(&mutex);
    if(numberCommands > 0){
        numberCommands--;
        pthread_mutex_unlock(&mutex);
        return inputCommands[headQueue++];  
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void errorParse() {
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void print_tecnicofs_tree_aux(char* outputPath) {
    FILE* fp = fopen(outputPath, "w");
    if (!fp) {
        fprintf(stderr, "Output path invalid, please try again!\n");
        exit(EXIT_FAILURE);
    }
    print_tecnicofs_tree(fp);
    fclose(fp);
}

void validateArguments(char* numThreads, char* syncStrat){
        
    int n = atoi(numThreads), i, validStrat = 0;
    const char* validStrats[] = {"nosync", "mutex", "rwlock"};

    if (n <= 0 || (n > 1 && !strcmp(syncStrat, "nosync"))) {
        fprintf(stderr, "Error starting thread pool, please try again!\n");
        exit(EXIT_FAILURE);
    }

    numberThreads = n;

    /* process input strategy */
    for (i = 0; i < 3; i++)
        if (!strcmp(syncStrat, validStrats[i])){
            if (i == RWLOCK)
                pthread_rwlock_init(&rwl, NULL);
            syncStrategy = i;
            validStrat++;
            break;
        }

    if (!validStrat){
        fprintf(stderr, "Invalid synchronisation strategy, please try again!\n");
        exit(EXIT_FAILURE);
    }
    
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
        fprintf(stderr, "Input path invalid, please try again!\n");
        exit(EXIT_FAILURE);
    }
    
    processInput(fp);
    fclose(fp);
}

void applyCommands() {
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

void startThreadPool(){

    int i;
    pthread_t tid[numberThreads];

    for (i = 0; i < numberThreads; i++){
        if(!pthread_create(&tid[i], NULL, (void*) applyCommands, NULL))
            printf("Thread number %d created successfully.\n", i);
        else
        {
            fprintf(stderr, "Thread not created!");
            exit(EXIT_FAILURE);
        }
    }
    /* wait for them to finish */
    for (i = 0; i < numberThreads; i++){
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Thread failed to join!");
            exit(EXIT_FAILURE);
        }
    }

}

int main(int argc, char* argv[]) {
    
    /* check argc */ 
    if (argc != 5) {
        fprintf(stderr, "Usage: ./tecnicofs inputfile outputfile numthreads synchstrategy\n");
        exit(EXIT_FAILURE);
    }

    validateArguments(argv[3], argv[4]);

    /* init filesystem and dinamically initialize mutex */
    init_fs();
    pthread_mutex_init(&mutex, NULL);

    /* process input and print tree */
    processInput_aux(argv[1]);
    startThreadPool();
    /* start the clock right after pool thread creation */
    gettimeofday(&tv1, NULL);
    applyCommands();
    print_tecnicofs_tree_aux(argv[2]);

    /* release allocated memory and destroy mutex */
    destroy_fs();
    destroySyncStructures();

    /* end the clock */
    gettimeofday(&tv2, NULL);
    printf ("TecnicoFS completed in %.4f seconds.\n",
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));

    exit(EXIT_SUCCESS);
}
