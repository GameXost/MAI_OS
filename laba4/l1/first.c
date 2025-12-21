#include "first.h"
#include <math.h>
#include <stdlib.h>

float e(int x) {
    if (x == 0) {
        return 1.0f;
    }
    return powf(1.0f + 1.0f / x, x);
}

int* sort(int* array, size_t n) {
    if (array == NULL || n == 0) {
        return array;
    }
    for (size_t i = 0; i < n - 1; i++) {
        for (size_t j = 0; j < n - i - 1; j++) {
            if (array[j] > array[j + 1]) {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }

    return array;
}