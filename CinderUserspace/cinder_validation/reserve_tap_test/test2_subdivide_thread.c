#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include <pthread.h>

#define TRUE 1
#define FALSE 0

#define INPUT_SIZE 30
#define CINDER_MAX_NAMELEN 20

#define CINDER_CAP_RESERVE_DRAW   0x1
#define CINDER_CAP_RESERVE_MODIFY 0x2

#define CINDER_TAP_RATE_CONSTANT 1
#define CINDER_TAP_RATE_PROPORTIONAL 2

#define SYS_CREATE_RESERVE 361

#define ROOT_RESERVE_ID 0

// Globals
int keep_running = TRUE;

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

int get_root_reserve_id()
{
	// TODO: Make it so root reserve is uniquely defined, and make it so we
	// can acquire a handle to it in case it exists. For now, hardcode the
	// id to be 0 since we assume it is id 0.
	return ROOT_RESERVE_ID;
}

struct test_args
{
	char id;
	int reserve_id;
};

void *
test_method(void *arg)
{
	struct test_args *testargs = (struct test_args *) arg;
	unsigned long counter = 0;
	long ret, capacity;
	pid_t mypid = getpid();

	ret = set_active_reserve(testargs->reserve_id);
	if (ret != 0) {
		printf("Thread %c could not set active reserve: %d\n", testargs->id, ret);
		exit(1);
	}

	//printf("Child task start: Thread %c\n", testargs->id);
	while (keep_running == TRUE) {
		counter++;
		if (counter % 100000 == 0) {
			//ret = reserve_level(testargs->reserve_id, &capacity);
			printf("Thread %c counter is %ld reserve level is %ld\n", testargs->id, counter, ret);
		}
	}

	printf("Thread %c exiting at %ld.\n", testargs->id, counter);
	return (void *)counter;
}

void print_usage(char *arg0)
{
	printf("Usage: %s <overall_tap_rate>  <weightA> <weightB> <weightC> <runtime>\n", arg0);
}

int main(int argc, char **argv)
{
	long overall_tap_rate = 0;
	int weightA = 1, weightB = 1, weightC = 1, total_weight;
	int proportionA, proportionB, proportionC;
	int runtime_seconds = 0, namelen;
	int main_reserve_id, main_tap_id, root_reserve_id, ret;
	unsigned long A_status, B_status, C_status, total = 0;

	int A_reserve_id, A_tap_id, B_reserve_id, B_tap_id, C_reserve_id, C_tap_id;

	const char *main_reserve_name = "main_reserve";
	const char *main_tap_name = "main_tap";
	
	const char *A_reserve_name = "A_reserve";
	const char *A_tap_name = "A_tap";

	const char *B_reserve_name = "B_reserve";
	const char *B_tap_name = "B_tap";

	const char *C_reserve_name = "C_reserve";
	const char *C_tap_name = "C_tap";

	pthread_t threadA, threadB, threadC;

	if (argc != 6) {
		print_usage(argv[0]);
		exit(1);
	}

	overall_tap_rate = atol(argv[1]);
	if (overall_tap_rate < 1) {
		printf("Invalid overall tap rate.\n");
		exit(1);
	}

	weightA = atoi(argv[2]);
	weightB = atoi(argv[3]);
	weightC = atoi(argv[4]);

	if (weightA < 0 || weightB < 0 || weightC < 0) {
		printf("Invalid weighting provided.\n");
		exit(1);
	}

	runtime_seconds = atoi(argv[5]);
	if (runtime_seconds < 1) {
		printf("Must run for >0 seconds.\n");
		exit(1);
	}

	// Create main reserve for child threads to use
	namelen = strlen(main_reserve_name) + 1;
	main_reserve_id = create_reserve((char *)main_reserve_name, namelen);
	if (main_reserve_id < 0) {
		printf("Error creating main reserve: %d\n", main_reserve_id);
		exit(1);
	}
	else
		printf("Created main reserve with id %d\n", main_reserve_id);

	// Get root reserve id
	root_reserve_id = get_root_reserve_id();

	// Create main tap for main reserve
	namelen = strlen(main_tap_name) + 1;
	main_tap_id = create_tap((char *)main_tap_name, namelen, root_reserve_id, main_reserve_id);
	if (main_tap_id < 0) {
		printf("Error creating main tap: %d\n", main_tap_id);
		exit(1);
	}
	else
		printf("Created main tap with id %d\n", main_tap_id);

	// Calculate proportions based on weighting
	total_weight = weightA + weightB + weightC;
	proportionA = (int)((((double)weightA) / total_weight)*100);
	proportionB = (int)((((double)weightB) / total_weight)*100);
	proportionC = (int)((((double)weightC) / total_weight)*100);

	// Create reserve and tap for A
	printf("\n");
	namelen = strlen(A_reserve_name) + 1;
	A_reserve_id = create_reserve((char *)A_reserve_name, namelen);
	if (A_reserve_id < 0) {
		printf("Error creating A reserve: %d\n", A_reserve_id);
		exit(1);
	}
	else
		printf("Created A reserve with id %d\n", A_reserve_id);

	namelen = strlen(main_tap_name) + 1;
	A_tap_id = create_tap((char *)A_tap_name, namelen, main_reserve_id, A_reserve_id);
	if (A_tap_id < 0) {
		printf("Error creating A tap: %d\n", A_tap_id);
		exit(1);
	}
	else
		printf("Created A tap with id %d\n", A_tap_id);

	ret = tap_set_rate(A_tap_id, CINDER_TAP_RATE_PROPORTIONAL, proportionA);
	if (ret < 0) {
		printf("Error setting A tap rate: %d\n", ret);
		exit(1);
	}
	else {
		printf("A Tap rate set to %ld\n", proportionA);
	}

	// Create reserve and tap for B
	printf("\n");
	namelen = strlen(B_reserve_name) + 1;
	B_reserve_id = create_reserve((char *)B_reserve_name, namelen);
	if (B_reserve_id < 0) {
		printf("Error creating B reserve: %d\n", B_reserve_id);
		exit(1);
	}
	else
		printf("Created B reserve with id %d\n", B_reserve_id);

	namelen = strlen(main_tap_name) + 1;
	B_tap_id = create_tap((char *)B_tap_name, namelen, main_reserve_id, B_reserve_id);
	if (B_tap_id < 0) {
		printf("Error creating B tap: %d\n", B_tap_id);
		exit(1);
	}
	else
		printf("Created B tap with id %d\n", B_tap_id);

	ret = tap_set_rate(B_tap_id, CINDER_TAP_RATE_PROPORTIONAL, proportionB);
	if (ret < 0) {
		printf("Error setting B tap rate: %d\n", ret);
		exit(1);
	}
	else {
		printf("B Tap rate set to %ld\n", proportionB);
	}

	// Create reserve and tap for C
	printf("\n");
	namelen = strlen(C_reserve_name) + 1;
	C_reserve_id = create_reserve((char *)C_reserve_name, namelen);
	if (C_reserve_id < 0) {
		printf("Error creating C reserve: %d\n", C_reserve_id);
		exit(1);
	}
	else
		printf("Created C reserve with id %d\n", C_reserve_id);

	namelen = strlen(main_tap_name) + 1;
	C_tap_id = create_tap((char *)C_tap_name, namelen, main_reserve_id, C_reserve_id);
	if (C_tap_id < 0) {
		printf("Error creating C tap: %d\n", C_tap_id);
		exit(1);
	}
	else
		printf("Created C tap with id %d\n", C_tap_id);

	ret = tap_set_rate(C_tap_id, CINDER_TAP_RATE_PROPORTIONAL, proportionC);
	if (ret < 0) {
		printf("Error setting C tap rate: %d\n", ret);
		exit(1);
	}
	else {
		printf("C Tap rate set to %ld\n", proportionC);
	}

	// Print out the proportions we are using
	printf("\nProportions: \n"
	       "A_Thread : %ld\n"
	       "B_Thread : %ld\n"
	       "C_Thread : %ld\n\n", proportionA, proportionB, proportionC);

	// Flow main tap
	ret = tap_set_rate(main_tap_id, CINDER_TAP_RATE_CONSTANT, overall_tap_rate);
	if (ret < 0) {
		printf("Error setting main tap rate: %d\n", ret);			
		exit(1);
	}
	else {
		printf("Main Tap rate set to %ld\n", overall_tap_rate);
	}

	// Fork threads A
	struct test_args A_args = {'A', A_reserve_id};
	ret = pthread_create(&threadA, NULL, test_method, &A_args);
	if (ret != 0) {
		printf("Error creating Pthread for A, error is %d\n", ret);
		exit(1);
	}

	// Fork thread B
	struct test_args B_args = {'B', B_reserve_id};
	ret = pthread_create(&threadB, NULL, test_method, &B_args);
	if (ret != 0) {
		printf("Error creating Pthread for B, error is %d\n", ret);
		exit(1);
	}

	// Fork thread C
	struct test_args C_args = {'C', C_reserve_id};
	ret = pthread_create(&threadC, NULL, test_method, &C_args);
	if (ret != 0) {
		printf("Error creating Pthread for C, error is %d\n", ret);
		exit(1);
	}

	// Sleep for the desired amount of seconds
	sleep(runtime_seconds);
	keep_running = 0;

	// Join thread A
	ret = pthread_join(threadA, (void **)&A_status);
	if (ret != 0) {
		printf("Join on thread A failed %d\n", ret);
	}
	else {
		total += A_status;
	}

	// Join thread B
	ret = pthread_join(threadB, (void **)&B_status);
	if (ret != 0) {
		printf("Join on thread B failed %d\n", ret);
	}
	else {
		total += B_status;
	}

	// Join thread C
	ret = pthread_join(threadC, (void **)&C_status);
	if (ret != 0) {
		printf("Join on thread C failed %d\n", ret);
	}
	else {
		total += C_status;
	}

	printf("Totals: \n");
	printf("A: %ld and proportion is %f/100 desired was %d/100\n", A_status, ((double)A_status/total) * 100, proportionA);
	printf("B: %ld and proportion is %f/100 desired was %d/100\n", B_status, ((double)B_status/total) * 100, proportionB);
	printf("C: %ld and proportion is %f/100 desired was %d/100\n", C_status, ((double)C_status/total) * 100, proportionC);

	// Done
	printf("Main thread exiting.\n");
	return 0;
}

