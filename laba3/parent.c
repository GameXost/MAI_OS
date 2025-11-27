#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>

static char SERVER_PROGRAM_NAME[] = "div-server";

#define SHM_SIZE 8192

typedef struct {
	char data[SHM_SIZE];
	size_t size;
	bool finished;
} SharedData;

int main(int argc, char **argv){
	if (argc == 1){
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg), "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

	char progpath[1024];
	{
		ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
		if (len == -1){
			const char msg[] = "error: failed to read full path of programm\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		progpath[len] = '\0';

		ssize_t i = len - 1;
		while (i >= 0 && progpath[i] != '/'){
			--i;
		}
		if (i >= 0){
			progpath[i] = '\0';
		} else{
			progpath[0] = '\0';
		}
	}

	char shm_in_name[256], shm_out_name[256];
	char sem_in_name[256], sem_out_name[256];
	pid_t pid = getpid();
	snprintf(shm_in_name, sizeof(shm_in_name), "/shm_in_%d", pid);
	snprintf(shm_out_name, sizeof(shm_out_name), "/shm_out_%d", pid);
	snprintf(sem_in_name, sizeof(sem_in_name), "/sem_in_%d", pid);
	snprintf(sem_out_name, sizeof(sem_out_name), "/sem_out_%d", pid);

	int shm_in = shm_open(shm_in_name, O_CREAT | O_RDWR, 0666);
	if (shm_in == -1){
		const char msg[] = "error: failed to create input shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	int shm_out = shm_open(shm_out_name, O_CREAT | O_RDWR, 0666);
	if (shm_out == -1){
		const char msg[] = "error: failed to create output shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		shm_unlink(shm_in_name);
		exit(EXIT_FAILURE);
	}

	if (ftruncate(shm_in, sizeof(SharedData)) == -1 || ftruncate(shm_out, sizeof(SharedData)) == -1){
		const char msg[] = "error: failed to set shared memory size\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		shm_unlink(shm_in_name);
		shm_unlink(shm_out_name);
		exit(EXIT_FAILURE);
	}

	SharedData *data_in = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_in, 0);
	SharedData *data_out = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_out, 0);

	if (data_in == MAP_FAILED || data_out == MAP_FAILED){
		const char msg[] = "error: failed to map shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		shm_unlink(shm_in_name);
		shm_unlink(shm_out_name);
		exit(EXIT_FAILURE);
	}

	data_in->size = 0;
	data_in->finished = false;
	data_out->size = 0;
	data_out->finished = false;

	sem_t *sem_in = sem_open(sem_in_name, O_CREAT, 0666, 0);
	sem_t *sem_out = sem_open(sem_out_name, O_CREAT, 0666, 0);

	if (sem_in == SEM_FAILED || sem_out == SEM_FAILED){
		const char msg[] = "error: failed to create semaphores\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		munmap(data_in, sizeof(SharedData));
		munmap(data_out, sizeof(SharedData));
		shm_unlink(shm_in_name);
		shm_unlink(shm_out_name);
		exit(EXIT_FAILURE);
	}

	const pid_t child = fork();
	switch (child){
		case (-1): {
			const char msg[] = "error failed to create new process\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		case(0):{
			{
				pid_t child_pid = getpid();
				char msg[64];
				const int32_t len = snprintf(msg, sizeof(msg), "%d: child process created\n", child_pid);
				write(STDOUT_FILENO, msg, len);
			}

			char path[2048];
			snprintf(path, sizeof(path), "%s/%s", progpath, SERVER_PROGRAM_NAME);
			char *const args[] = {SERVER_PROGRAM_NAME, shm_in_name, shm_out_name, sem_in_name, sem_out_name, NULL};
			int32_t status = execv(path, args);
			if (status == -1){
				const char msg[] = "error: failed to exec into new image\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		} break;
		default: {
			{
				char msg[64];
				const int32_t len = snprintf(msg, sizeof(msg), "%d: parent process, child PID %d\n", pid, child);
				write(STDOUT_FILENO, msg, len);
			}

			int32_t file = open(argv[1], O_RDONLY);
			if (file == -1){
				const char msg[] = "error: failed to open input file\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}

			char buf[4096];
			ssize_t bytes;
			bool child_alive = true;

			while (child_alive && (bytes = read(file, buf, sizeof(buf))) > 0){
				if ((size_t)bytes < SHM_SIZE){
					memcpy(data_in->data, buf, bytes);
					data_in->size = bytes;
					data_in->finished = false;

					sem_post(sem_in);
					sem_wait(sem_out);

					if (data_out->size > 0){
						write(STDOUT_FILENO, data_out->data, data_out->size);
					}

					if (data_out->finished){
						const char msg[] = "parent: child process terminated (div by 0)\n";
						write(STDERR_FILENO, msg, sizeof(msg));
						child_alive = false;
						break;
					}
				}
			}

			if (bytes == -1) {
				const char msg[] = "error: failed to read input file\n";
				write(STDERR_FILENO, msg, sizeof(msg));
			}

			data_in->finished = true;
			data_in->size = 0;
			sem_post(sem_in);

			close(file);

			int status;
			wait(&status);
			if (WIFEXITED(status)){
				int exitCode = WEXITSTATUS(status);
				if (exitCode != 0){
					const char msg[] = "parent: terminating due to child error\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				}
			}
			const char msg[] = "parent: programm completed successfully\n";
			write(STDOUT_FILENO, msg, sizeof(msg));

			munmap(data_in, sizeof(SharedData));
			munmap(data_out, sizeof(SharedData));
			shm_unlink(shm_in_name);
			shm_unlink(shm_out_name);
			sem_close(sem_in);
			sem_close(sem_out);
			sem_unlink(sem_in_name);
			sem_unlink(sem_out_name);
		} break;
	}
	return 0;
}