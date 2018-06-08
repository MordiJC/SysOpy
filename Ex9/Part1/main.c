#define _DEFAULT_SOURCE

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

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

pthread_mutex_t * mutexes = NULL;

pthread_mutex_t writeDataMutex;
pthread_mutex_t readDataMutex;

pthread_cond_t writeConditional;
pthread_cond_t readConditional;

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
    int index = 0;
    char lineBuf[LINE_MAX] = {0};

    while (fgets(lineBuf, LINE_MAX, inputFile) != NULL) {
        if(verbose){
            fprintf(stderr, "Producer %ld reading file line\n", pthread_self());
        }
        pthread_mutex_lock(&writeDataMutex);

        while (buffer[currentProducerIndex] != NULL) {
            pthread_cond_wait(&writeConditional, &writeDataMutex);
        }

        index = currentProducerIndex;
        if(verbose){
            fprintf(stderr, "Producer %ld taking buffer index (%d)\n",  pthread_self(), index);
        }
        currentProducerIndex = (currentProducerIndex + 1) % N;

        pthread_mutex_lock(&mutexes[index]);
        buffer[index] = malloc((strlen(lineBuf) + 1) * sizeof(char));
        strcpy(buffer[index], lineBuf);

        if(verbose) fprintf(stderr, "Producer %ld line copied to buffer at index (%d)\n",  pthread_self(), index);

        pthread_cond_broadcast(&readConditional);
        pthread_mutex_unlock(&mutexes[index]);
        pthread_mutex_unlock(&writeDataMutex);
    }

    if(verbose) {
        fprintf(stderr, "Producer %lu %ld finished\n", (size_t)arg, pthread_self());
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
            if(verbose) {
                fprintf(stderr, "Consumer %ld finished \n",  pthread_self());
            }
            return NULL;
        }
        pthread_mutex_lock(&readDataMutex);

        while (buffer[currentConsumerIndex] == NULL) {
            if (finished) {
                pthread_mutex_unlock(&readDataMutex);
                if(verbose){
                    fprintf(stderr, "Consumer %ld finished \n",  pthread_self());
                }
                return NULL;
            }
            pthread_cond_wait(&readConditional, &readDataMutex);
        }

        index = currentConsumerIndex;
        currentConsumerIndex = (currentConsumerIndex + 1) % N;
        if(verbose){
            fprintf(stderr, "Consumer %ld taking buffer index (%d)\n",  pthread_self(), index);
        }

        pthread_mutex_unlock(&readDataMutex);
        pthread_mutex_lock(&mutexes[index]);

        if(buffer[index] == NULL) {
            pthread_mutex_unlock(&mutexes[index]);
            continue;
        }

        line = buffer[index];
        buffer[index] = NULL;
        if(verbose) {
            fprintf(stderr, "Consumer %ld taking line from buffer at index (%d)\n",  pthread_self(), index);
        }
    
        size_t lineLen = strlen(line);

        if(lenCondition(lineLen)){
            if(verbose) {
                fprintf(stderr, "Consumer %ld found line with length %ld %c %ld\n",
                                pthread_self(), lineLen, searchMode, L);
            } 
            fprintf(stderr, "Consumer %ld index(%d):\n%s",  pthread_self(), index, line);
        }
        free(line);
        pthread_cond_broadcast(&writeConditional);
        pthread_mutex_unlock(&mutexes[index]);
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

    pthread_mutex_init(&writeDataMutex, NULL);
    pthread_mutex_init(&readDataMutex, NULL);

    mutexes = (pthread_mutex_t*) calloc(N, sizeof(pthread_mutex_t));
    for(size_t i = 0; i < N; ++i) {
        pthread_mutex_init(&(mutexes[i]), NULL);
    }

    pthread_cond_init(&writeConditional, NULL);
    pthread_cond_init(&readConditional, NULL);

    buffer = (char **)calloc(N, sizeof(char *));

    consumers = (pthread_t *)calloc(consumersNum, sizeof(pthread_t));
    producers = (pthread_t *)calloc(producersNum, sizeof(pthread_t));
}

void joinThreads(void) {
    for (size_t i = 0; i < producersNum; ++i) {
        fprintf(stderr, "JOINING PRODUCER %lu\n", i);
        pthread_join(producers[i], NULL);
        fprintf(stderr, "JOINED PRODUCER %lu\n", i);
    }

    finished = true;

    for (size_t i = 0; i < consumersNum; ++i) {
        fprintf(stderr, "JOINING CONSUMER %lu\n", i);
        pthread_join(consumers[i], NULL);
        fprintf(stderr, "JOINED CONSUMER %lu\n", i);
    }
}

void deinit(void) {

    if(inputFile) {
        fclose(inputFile);
    }

    for(size_t i = 0; i < N; ++i) {
        pthread_mutex_destroy(&(mutexes[i]));
    }

    pthread_cond_destroy(&writeConditional);
    pthread_cond_destroy(&readConditional);

    free(mutexes);

    pthread_mutex_destroy(&writeDataMutex);
    pthread_mutex_destroy(&readDataMutex);

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
        if (pthread_create(&(producers[i]), NULL, producerFun, (void*)i) != 0) {
            die(3, "Failed to create producer thread.");
        }
    }

    for (size_t i = 0; i < consumersNum; ++i) {
        if (pthread_create(&(consumers[i]), NULL, consumerFun, (void*)i) != 0) {
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