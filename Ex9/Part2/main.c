#define _DEFAULT_SOURCE

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>

size_t consumersNum = 0;
size_t consumersActive = 0;
pthread_t *consumers = NULL;

size_t producersNum = 0;
size_t producersActive = 0;
pthread_t *producers = NULL;

size_t N = 0;
size_t currentElements = 0;
char **buffer = NULL;

size_t currentProducerIndex = 0;
size_t currentConsumerIndex = 0;

static volatile bool finished = false;

char filePath[512] = {0};
FILE * inputFile = NULL;

int verbose = 0;

int nk = 0;

size_t L = 0;

char searchMode = '=';

sem_t * semaphores = NULL;

sem_t writeDataSemaphore;
sem_t readDataSemaphore;

sem_t accSemaphore;

#define LINE_MAX 1024

bool readConfig(const char *configPath) {
    FILE *inFile = fopen(configPath, "r");

    if (inFile == NULL) {
        return false;
    }

    if (fscanf(inFile, "Producers: %lu\n", &producersNum) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "Producers: %lu\n", producersNum);

    if (fscanf(inFile, "Consumers: %lu\n", &consumersNum) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "Consumers: %lu\n", consumersNum);

    if (fscanf(inFile, "BufferSize: %lu\n", &N) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "BufferSize: %lu\n", N);

    if (fscanf(inFile, "File: %511[^\n]\n", filePath) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "File: %s\n", filePath);

    if (fscanf(inFile, "LineLen: %lu\n", &L) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "LineLen: %lu\n", L);

    if (fscanf(inFile, "SearchMode: %c\n", &searchMode) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "SearchMode: %c\n", searchMode);

    if (fscanf(inFile, "Verbose: %d\n", &verbose) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "Verbose: %d\n", verbose);

    if (fscanf(inFile, "NK: %d", &nk) < 1) {
        fclose(inFile);
        return false;
    }
    fprintf(stderr, "NK: %d\n", nk);

    fclose(inFile);

    if (producersNum < 1 || consumersNum < 1 || N < 1 || L < 1 ||
        (searchMode != '<' && searchMode != '=' && searchMode != '>') ||
        (verbose != 0 && verbose != 1) || nk < 0) {
        return false;
    }

    return true;
}

void die(int err, const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(err);
}

void deinit(void);

void signal_handler(int signum) {
    (void)signum;
    
    fprintf(stderr, "Received signal %s, canceling threads\n", signum == SIGALRM ? "SIGALRM" : "SIGINT");

    for (size_t i = 0; i < producersNum; i++){
        pthread_cancel(producers[i]);
    }

    for (size_t i = 0; i < consumersNum; i++){
        pthread_cancel(consumers[i]);
    }

    deinit();
    exit(EXIT_SUCCESS);
}

bool lenCondition(size_t lineLen) {
    if(searchMode == '=') {
        return lineLen == L;
    } else if(searchMode == '<') {
        return lineLen < L;
    } else {
        return lineLen > L;
    }
}

void *producerFun(void *arg) {
    (void)arg;

    int index = 0;
    char lineBuf[LINE_MAX] = {0};

    while (fgets(lineBuf, LINE_MAX, inputFile) != NULL) {
        if(verbose){
            fprintf(stderr, "Producer %ld reading file line\n", pthread_self());
        }
        sem_wait(&writeDataSemaphore);

        sem_wait(&accSemaphore);

        index = currentProducerIndex;
        if(verbose){
            fprintf(stderr, "Producer %ld taking buffer index (%d)\n",  pthread_self(), index);
        }
        currentProducerIndex = (currentProducerIndex + 1) % N;

        sem_wait(&semaphores[index]);
        sem_post(&writeDataSemaphore);

        buffer[index] = malloc((strlen(lineBuf) + 1) * sizeof(char));
        strcpy(buffer[index], lineBuf);

        if(verbose) fprintf(stderr, "Producer %ld line copied to buffer at index (%d)\n",  pthread_self(), index);

        sem_post(&semaphores[index]);
    }

    if(verbose) {
        fprintf(stderr, "Producer %ld finished\n", pthread_self());
    }

    finished = true;

    return 0;
}

void *consumerFun(void *arg) {
    (void)arg;

    char *line = NULL;
    int index;
    while (1) {
        if (finished) {
            if(verbose) fprintf(stderr, "Consumer %ld finished \n",  pthread_self());
            return NULL;
        }
        sem_wait(&readDataSemaphore);

        while (buffer[currentConsumerIndex] == NULL) {
            sem_post(&readDataSemaphore);
            if (finished) {
                if(verbose){
                    fprintf(stderr, "Consumer %ld finished \n",  pthread_self());
                }
                return NULL;
            }
            sem_wait(&readDataSemaphore);
        }

        index = currentConsumerIndex;
        currentConsumerIndex = (currentConsumerIndex + 1) % N;
        if(verbose){
            fprintf(stderr, "Consumer %ld taking buffer index (%d)\n",  pthread_self(), index);
        }

        sem_wait(&semaphores[index]);

        line = buffer[index];
        buffer[index] = NULL;
        if(verbose) {
            fprintf(stderr, "Consumer %ld taking line from buffer at index (%d)\n",  pthread_self(), index);
        }
        
        sem_post(&accSemaphore);
        sem_post(&readDataSemaphore);
        sem_post(&semaphores[index]);

        size_t lineLen = strlen(line);

        if(lenCondition(lineLen)){
            if(verbose) {
                fprintf(stderr, "Consumer %ld found line with length %ld %c %ld\n",
                                pthread_self(), lineLen, searchMode, L);
            } 
            fprintf(stderr, "Consumer %ld index(%d):\n%s",  pthread_self(), index, line);
        }
        free(line);
        usleep(10);
    }

    return 0;
}

void init(void) {
    signal(SIGINT, signal_handler);
    if(nk > 0) {
        signal(SIGALRM, signal_handler);
    }

    inputFile = fopen(filePath, "r");

    if(inputFile == NULL) {
        die(3, "Unable to open file.");
    }

    semaphores = (sem_t*) calloc(N, sizeof(sem_t));
    for(size_t i = 0; i < N; ++i) {
        sem_init(&(semaphores[i]), 0, 1);
    }

    sem_init(&writeDataSemaphore, 0, 1);
    sem_init(&readDataSemaphore, 0, 1);
    sem_init(&accSemaphore, 0, (unsigned int) N);

    buffer = (char **)calloc(N, sizeof(char *));

    consumers = (pthread_t *)calloc(consumersNum, sizeof(pthread_t));
    producers = (pthread_t *)calloc(producersNum, sizeof(pthread_t));
}

void joinThreads(void) {
    for (size_t i = 0; i < producersNum; ++i) {
        pthread_join(producers[i], NULL);
    }

    finished = true;

    for (size_t i = 0; i < consumersNum; ++i) {
        pthread_join(consumers[i], NULL);
    }
}

void deinit(void) {

    if(inputFile) {
        fclose(inputFile);
    }

    for(size_t i = 0; i < N; ++i) {
        sem_destroy(&semaphores[i]);
    }

    sem_destroy(&writeDataSemaphore);
    sem_destroy(&readDataSemaphore);

    free(semaphores);

    for(size_t i = 0; i < N; ++i) {
        if(buffer[i]) {
            free(buffer[i]);
        }
    }

    free(buffer);

    free(consumers);
    free(producers);
}

void runThreads(void) {
    for (size_t i = 0; i < producersNum; ++i) {
        if (pthread_create(&(producers[i]), NULL, producerFun, 0) != 0) {
            die(3, "Failed to create producer thread.");
        }
    }

    for (size_t i = 0; i < consumersNum; ++i) {
        if (pthread_create(&(consumers[i]), NULL, consumerFun, 0) != 0) {
            die(3, "Failed to create consumer thread.");
        }
    }

    if(nk > 0) {
        alarm(nk);
    }
}

void run(void) {

    init();

    runThreads();

    joinThreads();

    deinit();
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        die(1, "Not enough arguments.");
    }

    if (!readConfig(argv[1])) {
        die(2, "Failed to read config.");
    }

    run();

    return 0;
}