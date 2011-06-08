#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#define INPUT_SIZE 30
#define CINDER_MAX_NAMELEN 20

#define CINDER_CAP_RESERVE_DRAW   0x1
#define CINDER_CAP_RESERVE_MODIFY 0x2

#define CINDER_TAP_RATE_CONSTANT 1
#define CINDER_TAP_RATE_PROPORTIONAL 2

#define SYS_CREATE_RESERVE 361

#define ROOT_RESERVE_ID 0

#define SYS_RESERVE_LEVEL 365
long reserve_level(int reserve_id, long *capacity)
{
	return syscall(SYS_RESERVE_LEVEL, reserve_id, capacity);
}

#define SYS_SET_ACTIVE_RESERVE 366
long set_active_reserve(int reserve_id)
{
	// TODO: Need get active reserve
	return syscall(SYS_SET_ACTIVE_RESERVE, reserve_id);
}

int create_reserve(char *name, unsigned int len)
{
	return syscall(SYS_CREATE_RESERVE, name, len);
}

#define SYS_SET_TAP_RATE 367
long tap_set_rate(int tap_id, long rate_type, long value)
{
	return syscall(SYS_SET_TAP_RATE, tap_id, rate_type, value);
}

#define SYS_CREATE_TAP 362
int create_tap(char *name, int len, int srcReserve, int destReserve)
{
	return syscall(SYS_CREATE_TAP, name, len, srcReserve, destReserve);
}

#define SYS_ADD_RESERVE_CHILD_LIST 378
long add_reserve_to_child_list(int reserve_id, unsigned int capabilities)
{
	return syscall(SYS_ADD_RESERVE_CHILD_LIST, reserve_id, capabilities);
}

#define SYS_DEL_RESERVE_CHILD_LIST 379
long del_reserve_from_child_list(int reserve_id)
{
	return syscall(SYS_DEL_RESERVE_CHILD_LIST, reserve_id);
}

#define SYS_RESERVE_TRANSFER 363
long reserve_transfer(int src_reserve_id, 
                      int dest_reserve_id, 
                      long amount, 
                      long fail_if_insufficient)
{
	return syscall(SYS_RESERVE_TRANSFER, src_reserve_id, dest_reserve_id, amount, fail_if_insufficient);
}

void print_usage(int argc, char **argv)
{
	printf("Usage: %s <constant_rate> <counter_max_val>\n", argv[0]);
}

int get_root_reserve_id()
{
	// TODO: Make it so root reserve is uniquely defined, and make it so we
	// can acquire a handle to it in case it exists. For now, hardcode the
	// id to be 0 since we assume it is id 0.
	return ROOT_RESERVE_ID;
}

int test_method(unsigned long maxval, long tap_rate, int reserve_id)
{
	long ret, capacity;
	printf("Child task start: PID is %d\n", getpid());

	struct timeval start, end;
	long mtime, seconds, useconds;
	unsigned long counter = 0;

	gettimeofday(&start, NULL);

	while (counter < maxval) {
		if (counter % 100 == 0) {
		ret = reserve_level(reserve_id, &capacity);
		printf("Reserve level is now %ld Counter is %ld\n", capacity, counter);}
		counter++;
	}

	gettimeofday(&end, NULL);

	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

	printf("Elapsed time: %ld milliseconds to count to value %ld with tap rate %ld\n", mtime, maxval, tap_rate);
	return 0;
}

int main(int argc, char **argv)
{
	int tap_id, reserve_id, len, root_reserve_id, child_status;
	const char *rsvname = "testreserve";
	const char *tapname = "testtap";
	long tap_rate, ret;
	pid_t forkpid, waitresult;
	unsigned long maxval;

	// Check number of command line parameters
	if (argc != 3) {
		print_usage(argc, argv);
		exit(1);
	}

	// Get the constant rate for the tap we'll create
	tap_rate = atol(argv[1]);
	if (tap_rate < 1) {
		printf("Error: Must have positive tap rate, was provided %ld instead.\n", tap_rate);
		exit(1);
	}

	maxval = atol(argv[2]);
	if (maxval == 0) {
		printf("Error: Must provide nonzero maxval.\n", maxval);
		exit(1);
	}
	
	// Set up parameters for our reserve	
	len = strlen(rsvname) + 1;

	// Create reserve for child to use
	reserve_id = create_reserve((char *)rsvname, len);
	if (reserve_id < 0) {
		printf("Error creating reserve: %d\n", reserve_id);
		exit(1);
	}
	else
		printf("Created reserve with id %d\n", reserve_id);

	// Get root reserve id
	root_reserve_id = get_root_reserve_id();

	// Create a tap from root reserve to the reserve we just created
	len = strlen(tapname) + 1;
	tap_id = create_tap((char *)tapname, len, root_reserve_id, reserve_id);
	if (tap_id < 0) {
		printf("Error creating tap: %d\n", tap_id);
		exit(1);
	}
	else
		printf("Created tap with id %d\n", tap_id);

	// Remove root reserve from our child list
	ret = del_reserve_from_child_list(root_reserve_id);
	if (ret < 0) {
		printf("Error removing root reserve from child list: %d\n", ret);
		exit(1);
	}
	else {
		printf("Removed root reserve from child list.\n");
	}

	// Add the reserve we created to the child list
	ret = add_reserve_to_child_list(reserve_id, CINDER_CAP_RESERVE_DRAW);
	if (ret < 0) {
		printf("Error adding new reserve to child list: %d\n", ret);
		exit(1);
	}
	else {
		printf("Added new reserve to child list.\n");
	}

	// Now we are ready to fork, and the child process should automatically
	// have the correct active reserve set.
	forkpid = fork();
	if (forkpid == -1) { // error
		perror("Fork(): ");
		exit(1);
	}
	else if (forkpid == 0) { // child
		test_method(maxval, tap_rate, reserve_id);
	}
	else { // parent
		printf("Successfully forked child with pid %d\n", forkpid);

		// Start flowing tap.
		ret = tap_set_rate(tap_id, CINDER_TAP_RATE_CONSTANT, tap_rate);
		if (ret < 0) {
			printf("Error setting tap rate: %d\n", ret);
			// TODO: What else can we do for recovery here? Maybe grant child some resources and do a kill -9?			
			exit(1);
		}
		else {
			printf("Tap rate set to %ld\n", tap_rate);
		}
		
		/* Wait on child */
		waitresult = waitpid(forkpid, &child_status, 0);
		if (waitresult == -1) {
			perror("Wait(): ");
			exit(1);
		}
		else if (waitresult != forkpid) {
			printf("Unexpected child process waited upon: Expected %d Found %d\n", forkpid, waitresult);
			exit(1);
		}
		else {
			printf("Child %d exited with code %d\n", waitresult, WEXITSTATUS(child_status));
		}
	}

	printf("Parent process exiting normally.\n");
	return 0;
}

