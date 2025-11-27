#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define MAX_NUMS 100
#define SHM_SIZE 8192

typedef struct {
	char data[SHM_SIZE];
	size_t size;
	bool finished;
} SharedData;

int main(int argc, char **argv){
	if (argc != 5){
		const char msg[] = "error: incorrect arguments\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	char *shm_in_name = argv[1];
	char *shm_out_name = argv[2];
	char *sem_in_name = argv[3];
	char *sem_out_name = argv[4];

	// shared memory
	int shm_in = shm_open(shm_in_name, O_RDWR, 0666);
	int shm_out = shm_open(shm_out_name, O_RDWR, 0666);

	if (shm_in == -1 || shm_out == -1){
		const char msg[] = "error: failed to open shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	SharedData *data_in = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_in, 0);
	SharedData *data_out = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_out, 0);

	if (data_in == MAP_FAILED || data_out == MAP_FAILED){
		const char msg[] = "error: failed to map shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	// семафоры
	sem_t *sem_in = sem_open(sem_in_name, 0);
	sem_t *sem_out = sem_open(sem_out_name, 0);

	if (sem_in == SEM_FAILED || sem_out == SEM_FAILED){
		const char msg[] = "error: failed to open semaphores\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	pid_t pid = getpid();
	char str[4096];
	uint32_t strInd = 0;

	while(true){
		sem_wait(sem_in);
		data_out->size = 0;

		if (data_in->finished){
			char msg[128];
			int len = snprintf(msg, sizeof(msg), "PID %d successfully terminated\n", pid);
			write(STDERR_FILENO, msg, len);
			break;
		}

		ssize_t bytes = data_in->size;
		char *buf = data_in->data;

		for(ssize_t i = 0; i < bytes; i++){
			if (buf[i] == '\n') {
				if (strInd == 0){
					continue;
				}
				str[strInd] = '\0';

				float nums[MAX_NUMS];
				uint32_t cnt = 0;

				char *ptr = str;
				char *endptr = NULL;

				while(*ptr != '\0' && cnt < MAX_NUMS) {
					while(*ptr == ' ' || *ptr == '\t'){
						ptr++;
					}
					if (*ptr == '\0'){
						break;
					}
					nums[cnt] = strtof(ptr, &endptr);
					if (endptr == ptr){
						break;
					}
					cnt++;
					ptr = endptr;
				}

				if (cnt < 2){
					const char msg[] = "error occured, need at least 2 nums\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					strInd = 0;
					continue;
				}

				float res = nums[0];
				bool divByZero = false;
				char output[4096];
				int outLen = snprintf(output, sizeof(output), "processing %.2f", nums[0]);

				for (uint32_t j = 1; j < cnt; j++){
					if(nums[j] == 0.0f){
						char msg[256];
						int msgLen = snprintf(msg, sizeof(msg), "div by zero occeured, child PID: %d is terminatied\n", pid);
						write(STDERR_FILENO, msg, msgLen);
						divByZero = true;
						break;
					}
					outLen += snprintf(output + outLen, sizeof(output) - outLen, " / %.2f", nums[j]);
					res /= nums[j];
				}

				if (divByZero) {
					data_out->finished = true;
					data_out->size = 0;
					sem_post(sem_out);

					munmap(data_in, sizeof(SharedData));
					munmap(data_out, sizeof(SharedData));
					sem_close(sem_in);
					sem_close(sem_out);
					exit(EXIT_FAILURE);
				}

				outLen += snprintf(output + outLen, sizeof(output) - outLen, " = %.6f\n", res);

				if (data_out->size + outLen <= SHM_SIZE) {
					memcpy(data_out->data + data_out->size, output, outLen);
					data_out->size += outLen;
				}

				data_out->finished = false;

				char log[256];
				int logLen = snprintf(log, sizeof(log), "PID: %d with result : %.6f\n", pid, res);
				write(STDERR_FILENO, log, logLen);
				strInd = 0;

			} else {
				if(strInd < sizeof(str) - 1){
					str[strInd++] = buf[i];
				}
			}
		}
		sem_post(sem_out);
	}

	munmap(data_in, sizeof(SharedData));
	munmap(data_out, sizeof(SharedData));
	sem_close(sem_in);
	sem_close(sem_out);

	return 0;
}