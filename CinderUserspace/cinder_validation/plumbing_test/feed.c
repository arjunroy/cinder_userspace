#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/unistd.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s <pid>\n", argv[0]);
		exit(1);
	}

	int pid = atoi(argv[1]);
	long ret;
	
	// __NR_starve = 376, feed = 377
	ret = syscall(377, pid);
	printf("Feed returned %d\n", ret);	
	return 0;
}

