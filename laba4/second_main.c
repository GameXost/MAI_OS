#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>

#define BUFSIZE 1024

typedef float (*e_func_t)(int);
typedef int* (*sort_func_t)(int*, size_t);

typedef struct {
    void* handle;
    e_func_t e_func;
    sort_func_t sort_func;
    int lib_number;
} Library;

Library current_lib = {NULL, NULL, NULL, 1};

int readLine(char *buffer, const ssize_t size) {
    ssize_t bytes_read = read(STDIN_FILENO, buffer, size);
    if (bytes_read < 0) {
        return 1;
    }
    if (bytes_read == 0 || buffer[0] == '\n') {
        return 2;
    }
    if (bytes_read == size && buffer[size - 1] != '\n') {
        return 3;
    }
    if (buffer[bytes_read - 1] == '\n') {
        buffer[bytes_read - 1] = '\0';
    } else {
        buffer[bytes_read] = '\0';
    }
    return 0;
}

int load_library(int lib_num) {
    if (current_lib.handle) {
        dlclose(current_lib.handle);
    }

    const char* lib_path;
    if (lib_num == 1) {
        lib_path = "./l1/libfirst.so";
    } else {
        lib_path = "./l2/libsecond.so";
    }

    current_lib.handle = dlopen(lib_path, RTLD_LAZY);
    if (!current_lib.handle) {
        char msg[BUFSIZE];
        int msg_size = snprintf(msg, BUFSIZE, "Ошибка загрузки библиотеки: %s\n", dlerror());
        write(STDERR_FILENO, msg, msg_size);
        return 0;
    }

    current_lib.e_func = (e_func_t)dlsym(current_lib.handle, "e");
    if (!current_lib.e_func) {
        char msg[BUFSIZE];
        int msg_size = snprintf(msg, BUFSIZE, "Ошибка загрузки функции e: %s\n", dlerror());
        write(STDERR_FILENO, msg, msg_size);
        dlclose(current_lib.handle);
        return 0;
    }

    current_lib.sort_func = (sort_func_t)dlsym(current_lib.handle, "sort");
    if (!current_lib.sort_func) {
        char msg[BUFSIZE];
        int msg_size = snprintf(msg, BUFSIZE, "Ошибка загрузки функции sort: %s\n", dlerror());
        write(STDERR_FILENO, msg, msg_size);
        dlclose(current_lib.handle);
        return 0;
    }

    current_lib.lib_number = lib_num;

    char output[BUFSIZE];
    int offset = 0;
    offset += snprintf(output + offset, BUFSIZE - offset, "Загружена библиотека %d\n", lib_num);
    if (lib_num == 1) {
        offset += snprintf(output + offset, BUFSIZE - offset, "Функция e: (1 + 1/x)^x\n");
        offset += snprintf(output + offset, BUFSIZE - offset, "Сортировка: Пузырьковая\n");
    } else {
        offset += snprintf(output + offset, BUFSIZE - offset, "Функция e: Сумма ряда 1/n!\n");
        offset += snprintf(output + offset, BUFSIZE - offset, "Сортировка: Быстрая сортировка Хоара\n");
    }
    write(STDOUT_FILENO, output, offset);

    return 1;
}

int main() {
    char user_input[BUFSIZE];
    int running = 1;

    if (!load_library(1)) {
        const char msg[] = "Не удалось загрузить начальную библиотеку\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    do {
        int err = readLine(user_input, BUFSIZE);
        switch (err) {
            case 1: {
                const char msg[] = "error: failed to read stdin\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
            case 2: {
                running = 0;
            } break;
            case 3: {
                const char msg[] = "error: buffer overflow\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
        }

        if (!running) {
            break;
        }

        if (strcmp(user_input, "exit") == 0) {
            break;
        }

        char *arg = strtok(user_input, " \t\n");
        if (!arg) {
            const char msg[] = "Ошибка: неверная команда\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            continue;
        }

        int command = atoi(arg);

        switch (command) {
            case 0: {
                int new_lib = (current_lib.lib_number == 1) ? 2 : 1;
                if (!load_library(new_lib)) {
                    char msg[BUFSIZE];
                    int msg_size = snprintf(msg, BUFSIZE, "Не удалось загрузить библиотеку %d\n", new_lib);
                    write(STDERR_FILENO, msg, msg_size);
                }
            } break;

            case 1: {
                char *x_str = strtok(NULL, " \t\n");
                if (!x_str) {
                    const char msg[] = "Ошибка: необходим параметр x\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    break;
                }

                int x = atoi(x_str);
                float result = current_lib.e_func(x);

                char result_msg[BUFSIZE];
                int msg_size = snprintf(result_msg, BUFSIZE, "e(%d) = %.6f (библиотека %d)\n", x, result, current_lib.lib_number);
                write(STDOUT_FILENO, result_msg, msg_size);
            } break;

            case 2: {
                char *n_str = strtok(NULL, " \t\n");
                if (!n_str) {
                    const char msg[] = "Ошибка: необходимо указать размер массива n > 0\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    break;
                }

                int n = atoi(n_str);
                if (n <= 0) {
                    const char msg[] = "Ошибка: необходимо указать размер массива n > 0\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    break;
                }

                int* array = (int*)malloc(n * sizeof(int));
                if (!array) {
                    const char msg[] = "Ошибка выделения памяти\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    break;
                }

                int count = 0;
                int success = 1;

                for (int i = 0; i < n; i++) {
                    char* token = strtok(NULL, " \t\n");
                    if (!token) {
                        const char msg[] = "Ошибка: недостаточно элементов массива\n";
                        write(STDERR_FILENO, msg, sizeof(msg) - 1);
                        free(array);
                        success = 0;
                        break;
                    }
                    array[i] = atoi(token);
                    count++;
                }

                if (success) {
                    if (count != n) {
                        char msg[BUFSIZE];
                        int msg_size = snprintf(msg, BUFSIZE, "Ошибка: получено %d элементов вместо %d\n", count, n);
                        write(STDERR_FILENO, msg, msg_size);
                        free(array);
                        break;
                    }

                    char output[BUFSIZE];
                    int offset = 0;

                    offset += snprintf(output + offset, BUFSIZE - offset, "Исходный массив: ");
                    for (int i = 0; i < n; i++) {
                        offset += snprintf(output + offset, BUFSIZE - offset, "%d ", array[i]);
                    }
                    offset += snprintf(output + offset, BUFSIZE - offset, "\n");
                    write(STDOUT_FILENO, output, offset);

                    current_lib.sort_func(array, n);

                    offset = 0;
                    offset += snprintf(output + offset, BUFSIZE - offset, "Отсортированный массив: ");
                    for (int i = 0; i < n; i++) {
                        offset += snprintf(output + offset, BUFSIZE - offset, "%d ", array[i]);
                    }
                    offset += snprintf(output + offset, BUFSIZE - offset, " (библиотека %d)\n", current_lib.lib_number);
                    write(STDOUT_FILENO, output, offset);

                    free(array);
                }
            } break;

            default: {
                char msg[BUFSIZE];
                int msg_size = snprintf(msg, BUFSIZE, "Ошибка: неизвестная команда %d\n", command);
                write(STDERR_FILENO, msg, msg_size);
            } break;
        }
    } while (running);

    if (current_lib.handle) {
        dlclose(current_lib.handle);
    }

    return 0;
}