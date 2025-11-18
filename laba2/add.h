#pragma once

#include <pthread.h>

typedef struct {
    long rounds;
    unsigned int seed;
    long matches;
} __attribute__((aligned(64))) threadData;

void shuffle(int *deck, unsigned int *seed);
int parseInput(int argc, char **argv, long *rounds, int *threads);
void *process(void *something);
extern long global_matches;
extern pthread_mutex_t result_mutex;
void *process_no_mutex(void *something);