#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "add.h"

void shuffle(int *deck, unsigned int *seed) {
    for (int i = 51; i > 0; i--) {
        int j = rand_r(seed) % (i + 1);
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}


int parseInput(int argc, char **argv, long *rounds, int *threads) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            i++;
            *rounds = atol(argv[i]);
            if (*rounds <= 0) {
                printf("Enter positive amount of rounds\n");
                return 0;
            }
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            i++;
            *threads = atoi(argv[i]);
            if (*threads <= 0) {
                printf("Enter positive amount of theads\n");
                return 0;
            }
        } else {
            printf("Wtf you entered?\n");
            return 0;
        }
    }
    return 1;
}

//кол-во раундов и кол-во потоков
void *process(void *something) {
    threadData *data = (threadData*)something;
    int deck[52];
    long local_matches = 0;
    for (int i = 0; i < 52; i++) {
        deck[i] = i;
    }

    for (int i = 0; i < data->rounds; i++) {
        shuffle(deck, &data->seed);
        if (deck[0] % 13 == deck[1] % 13) {
            local_matches++;
        }
    }
    pthread_mutex_lock(&result_mutex);
    global_matches += local_matches;
    pthread_mutex_unlock(&result_mutex);
    return NULL;
}

//рещение без мьютексов, используются локальные данные
void *process_no_mutex(void *something) {
    threadData *data = (threadData*)something;
    int deck[52];
    long local_matches = 0;

    for (int i = 0; i < 52; i++) {
        deck[i] = i;
    }

    for (int i = 0; i < data->rounds; i++) {
        shuffle(deck, &data->seed);
        if (deck[0] % 13 == deck[1] % 13) {
            local_matches++;
        }
    }

    data->matches = local_matches;

    return NULL;
}