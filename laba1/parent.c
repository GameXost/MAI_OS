#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

static char SERVER_PROGRAM_NAME[] = "div-server";

int main( int argc, char **argv){
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
		while (i >= 0 && progpath[i] != '/') --i;
		if (i >= 0) progpath[i] = '\0';
		else progpath[0] = '\0';
	}

	int parentToChild[2];
	if (pipe(parentToChild) == -1){
		const char msg[] = "error: failed to create pipe parent to child\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	int childToParent[2];
	if (pipe(childToParent) == -1){
		const char msg[] = "error: failed to create pipe child to parent\n";
		write(STDERR_FILENO, msg, sizeof(msg));
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
				pid_t pid = getpid();
				char msg[64];
				const int32_t len = snprintf(msg, sizeof(msg), "%d: child process created\n", pid);
				write(STDOUT_FILENO, msg, len);
			}
			close(parentToChild[1]);
			close(childToParent[0]);

			dup2(parentToChild[0], STDIN_FILENO);
			close(parentToChild[0]);

			dup2(childToParent[1], STDOUT_FILENO);
			close(childToParent[1]);
			{
				char path[2048];
				snprintf(path, sizeof(path), "%s/%s", progpath, SERVER_PROGRAM_NAME);
				char *const args[] = {SERVER_PROGRAM_NAME, NULL};
				int32_t status = execv(path, args);
				if (status == -1){
					const char msg[] = "error: failed to exec into new image \n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				}
			}
		} break;
		default: {
			{
				pid_t pid = getpid();
				char msg[64];
				const int32_t len = snprintf(msg, sizeof(msg), "%d: parent process, child PID %d\n", pid, child);
				write(STDOUT_FILENO, msg, len);
			}
			close(parentToChild[0]);
			close(childToParent[1]);

			int32_t file = open(argv[1], O_RDONLY);
			if (file == -1){
				const char msg[] = "error: failed to open input file\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				close(parentToChild[1]);
				close(childToParent[0]);
				exit(EXIT_FAILURE);
			}
			char buf[4096];
			ssize_t bytes;
			bool last_was_newline = true;

			while ((bytes = read(file, buf, sizeof(buf))) > 0){
				if (bytes > 0) {
					ssize_t w = write(parentToChild[1], buf, (size_t)bytes);
					if (w == -1) {
						const char msg[] = "error: failed to write to child pipe\n";
						write(STDERR_FILENO, msg, sizeof(msg));
						break;
					}
					last_was_newline = (buf[bytes-1] == '\n');
				}
				ssize_t responseBytes = read(childToParent[0], buf, sizeof(buf));
				if (responseBytes > 0){
					write(STDOUT_FILENO, buf, responseBytes);
				} else if (responseBytes == 0){
					const char msg[] = "parent: child process terminated (div by 0)\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					break;
				}
			}

			if (bytes == -1) {
				const char msg[] = "error: failed to read input file\n";
				write(STDERR_FILENO, msg, sizeof(msg));
			}

			if (!last_was_newline) {
				const char nl = '\n';
				write(parentToChild[1], &nl, 1);
			}

			close(file);
			close(parentToChild[1]);
			close(childToParent[0]);

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
		}break;
	}
	return 0;
}
