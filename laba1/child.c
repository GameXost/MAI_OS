#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


#define MAX_NUMS 100



int main(int argc, char **argv){
	char buf[4096];
	ssize_t bytes;
	pid_t pid = getpid();

	char str[4096];
	uint32_t strInd = 0;
	while((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0){
		if(bytes < 0){
			const char msg[] = "error occured while reading from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

		for(ssize_t i = 0; i < bytes; i++){
			if (buf[i] == '\n') { //  /r for windows lol
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
					close(STDOUT_FILENO);
					exit(EXIT_FAILURE);
				}
				outLen += snprintf(output + outLen, sizeof(output) - outLen, " = %.6f\n", res);

				int32_t writing = write(STDOUT_FILENO, output, outLen);
				if(writing != outLen){
					const char msg[] = "error occured while writing the result into the file\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				}

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
	}
	if (bytes == 0){
		char msg[128];
		int len = snprintf(msg, sizeof(msg), "PID %d successfully terminated\n", pid);
		write(STDERR_FILENO, msg, len);
	}
	return 0;
}