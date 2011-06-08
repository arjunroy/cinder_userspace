#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(void)
{
	pid_t pid = getpid();
	int i = 0;

	while (1) {
		printf("PID is %d counter %d\n", pid, i++);
		//sleep(1);
	}

	return 0;
}
