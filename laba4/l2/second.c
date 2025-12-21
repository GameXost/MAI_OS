#include "second.h"
#include <stdlib.h>

static unsigned long long factorial(int n) {
    if (n <= 1){
        return 1;
    }
    unsigned long long result = 1;
    for (int i = 2; i <= n; i++){
        result *= i;
    }
    return result;
}

float e(int x) {
    if (x < 0){
        x = 0;
    }

    float sum = 0.0f;
    for (int n = 0; n <= x; n++){
        sum += 1.0f / factorial(n);
    }
    return sum;
}

static int partition_hoare(int* array, int low, int high) {
    int pivot = array[(low + high) / 2];
    int i = low - 1;
    int j = high + 1;

    while (1) {
        do {
            i++;
        } while (array[i] < pivot);
        do {
            j--;
        } while (array[j] > pivot);
        if (i >= j) {
            return j;
        }
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

//сортировка Хоара
static void quicksort(int* array, int low, int high) {
    if (low < high) {
        int pi = partition_hoare(array, low, high);
        quicksort(array, low, pi);
        quicksort(array, pi + 1, high);
    }
}


int* sort(int* array, size_t n) {
    if (array == NULL || n <= 1) {
        return array;
    }

    quicksort(array, 0, n - 1);
    return array;
}