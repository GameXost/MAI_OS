#include <pthread.h>
#include <stdio.h>
#include "add.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

// т.к. общий add.h, а там прописан extern
long global_matches = 0;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
    long roundsAmount = 0;
    int threadsAmount = 0;

    int parsed = parseInput(argc, argv, &roundsAmount, &threadsAmount);
    if (parsed == 0) {
        return 1;
    }
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * threadsAmount);
    if (threads == NULL) {
        printf("memory allocation error\n");
        return 1;
    }
    threadData *data = (threadData*)malloc(sizeof(threadData) * threadsAmount);
    if (data == NULL) {
        printf("memory allocation error\n");
        return 1;
    }

    long roundsPerThread = roundsAmount / threadsAmount;
    long roundsRemain = roundsAmount % threadsAmount;

    for (int i = 0; i < threadsAmount ; i++) {
        data[i].rounds = roundsPerThread;
        if (i == 0) {
            data[i].rounds += roundsRemain;
        }
        data[i].seed = (unsigned int)(time(NULL) ^ (i * 12345));
        data[i].matches = 0;

        if (pthread_create(&threads[i], NULL, process_no_mutex, &data[i]) != 0) {
            printf("Error creating thread\n");
            return 1;
        }
    }

    for (int i = 0; i < threadsAmount; i++) {
        pthread_join(threads[i], NULL);
    }


    long total_matches = 0;
    for (int i = 0; i < threadsAmount; i++) {
        total_matches += data[i].matches;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\nTotal rounds: %ld\n", roundsAmount);
    printf("Matches: %ld\n", total_matches);
    printf("Probability: %.6f\n", (double)total_matches / roundsAmount);
    printf("Time: %.3f sec\n", elapsed);

    free(threads);
    free(data);

    return 0;
}