#include <sys/syscall.h>

#include "util.h"

void chomp(char *str)
{
	if (!str)
		return;
	while(*str) {
		if (*str == '\n') {
			*str = 0;
			break;
		}
		str++;
	}
}

#define SYS_CREATE_RESERVE 361
int create_reserve(char *name, unsigned int len)
{
	return syscall(SYS_CREATE_RESERVE, name, len);
}

int handle_create_reserve()
{
	char name[CINDER_MAX_NAMELEN];
	char *retstr;
	unsigned int len;
	int ret;

	printf("Please enter the name for the reserve: ");
	retstr = fgets(name, sizeof(name), stdin);
	if (!retstr) {
		printf("Error reading name.\n");
		return -EINVAL;
	}
	name[CINDER_MAX_NAMELEN - 1] = 0;
	chomp(name);
	len = strlen(name) + 1;
	ret = create_reserve(name, len);
	if (ret < 0) {
		printf("Error creating reserve: %d\n", ret);
	}
	else
		printf("Created reserve with id %d\n", ret);

	return ret;
}

#define SYS_CREATE_TAP 362
int create_tap(char *name, int len, int srcReserve, int destReserve)
{
	return syscall(SYS_CREATE_TAP, name, len, srcReserve, destReserve);
}

int handle_create_tap()
{
	char name[CINDER_MAX_NAMELEN], id[12];
	char *retstr;
	unsigned int len;
	int ret, srcReserve, destReserve;

	printf("Please enter the name for the tap: ");
	retstr = fgets(name, sizeof(name), stdin);
	if (!retstr) {
		printf("Error reading name.\n");
		return -EINVAL;
	}
	name[CINDER_MAX_NAMELEN - 1] = 0;
	chomp(name);
	len = strlen(name) + 1;

	memset(id, 0, sizeof(id));
	printf("Please enter the source reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	srcReserve = atoi(id);
	
	memset(id, 0, sizeof(id));
	printf("Please enter the destination reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	destReserve = atoi(id);

	ret = create_tap(name, len, srcReserve, destReserve);

	if (ret < 0) {
		printf("Error creating tap: %d\n", ret);
	}
	else
		printf("Created tap with id %d\n", ret);

	return ret;
}

#define SYS_RESERVE_TRANSFER 363
long reserve_transfer(int src_reserve_id, 
                      int dest_reserve_id, 
                      long amount, 
                      long fail_if_insufficient)
{
	return syscall(SYS_RESERVE_TRANSFER, src_reserve_id, dest_reserve_id, amount, fail_if_insufficient);
}

long handle_reserve_transfer()
{
	char id[12], amount_str[22], *retstr;
	int srcReserve, destReserve;
	long amount, fail_if_insufficient, ret;

	memset(id, 0, sizeof(id));
	printf("Please enter the source reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	srcReserve = atoi(id);
	
	memset(id, 0, sizeof(id));
	printf("Please enter the destination reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	destReserve = atoi(id);

	memset(amount_str, 0, sizeof(amount_str));
	printf("Please enter the amount.\n");
	retstr = fgets(amount_str, sizeof(amount_str), stdin);
	if (!retstr) {
		printf("Error reading amount.\n");
		return -EINVAL;
	}
	amount = atol(amount_str);
	
	memset(amount_str, 0, sizeof(amount_str));
	printf("Fail if insufficient? 0/1 for no fail/fail.\n");
	retstr = fgets(amount_str, sizeof(amount_str), stdin);
	if (!retstr) {
		printf("Error reading value.\n");
		return -EINVAL;
	}
	fail_if_insufficient = atol(amount_str);

	ret = reserve_transfer(srcReserve, destReserve, amount, fail_if_insufficient);
	printf("Reserve transfer returned result %ld\n", ret);
	return ret;
}

#define SYS_RESERVE_INFO 364
long reserve_info(int reserve_id, struct reserve_info *info)
{
	return syscall(SYS_RESERVE_INFO, reserve_id, info);
}

long handle_info_reserve()
{
	int reserve_id;
	long ret;
	struct reserve_info info;
	char id[12], *retstr;

	memset(&info, 0, sizeof(info));

	memset(id, 0, sizeof(id));
	printf("Please enter the reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	reserve_id = atoi(id);

	ret = reserve_info(reserve_id, &info);
	if (ret) {
		printf("Error getting reserve info: %ld\n", ret);
		return ret;
	}

	printf("Got reserve info for reserve ID %d Name %s Capacity %ld Total lifetime input %ld Total lifetime usage %ld\n",
        info.id, info.name, info.capacity, info.lifetime_input, info.lifetime_usage);
	return ret;
}

#define SYS_RESERVE_LEVEL 365
long reserve_level(int reserve_id, long *capacity)
{
	return syscall(SYS_RESERVE_LEVEL, reserve_id, capacity);
}

int handle_reserve_level()
{
	int reserve_id;
	long ret, capacity;
	char id[12], *retstr;

	memset(id, 0, sizeof(id));
	printf("Please enter the reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	reserve_id = atoi(id);

	ret = reserve_level(reserve_id, &capacity);
	if (ret) {
		printf("Error getting reserve info: %ld\n", ret);
		return ret;
	}

	printf("Got reserve level for reserve ID %d Capacity %ld\n", reserve_id, capacity);
	return ret;
}

#define SYS_SET_ACTIVE_RESERVE 366
long set_active_reserve(int reserve_id)
{
	// TODO: Need get active reserve
	return syscall(SYS_SET_ACTIVE_RESERVE, reserve_id);
}

#define SYS_SET_TAP_RATE 367
long tap_set_rate(int tap_id, long rate_type, long value)
{
	return syscall(SYS_SET_TAP_RATE, tap_id, rate_type, value);
}

long handle_tap_set_rate()
{
	int tap_id;
	long ret, rate_type, value;
	char id[12], value_str[22], *retstr;

	memset(id, 0, sizeof(id));
	printf("Please enter the tap id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	tap_id = atoi(id);

	memset(value_str, 0, sizeof(value_str));
	printf("Please enter the rate type (%d for constant, %d for proportional).\n", CINDER_TAP_RATE_CONSTANT, CINDER_TAP_RATE_PROPORTIONAL);
	retstr = fgets(value_str, sizeof(value_str), stdin);
	if (!retstr) {
		printf("Error reading rate type.\n");
		return -EINVAL;
	}
	rate_type = atol(value_str);

	memset(value_str, 0, sizeof(value_str));
	printf("Please enter the rate value.\n");
	retstr = fgets(value_str, sizeof(value_str), stdin);
	if (!retstr) {
		printf("Error reading rate value.\n");
		return -EINVAL;
	}
	value = atol(value_str);

	ret = tap_set_rate(tap_id, rate_type, value);
	printf("Set tap rate returned: %ld\n", ret);
	return ret;
}

#define SYS_SELF_BILL 368
long self_bill(long billing_type, long amount)
{
	return syscall(SYS_SELF_BILL, billing_type, amount);
}

#define SYS_NUM_RESERVES 369
long num_reserves(void)
{
	return syscall(SYS_NUM_RESERVES);
}

#define SYS_GET_RESERVE 370
int get_reserve(long index)
{
	return syscall(SYS_GET_RESERVE, index);
}

int handle_list_reserves()
{
	printf("Enumerating reserves:\n");
	long index, num = num_reserves();
	if (num < 0) {
		printf("Getting number of reserves returned %ld\n", num);
		return num;
	}

	printf("Found %ld reserves:\n", num);
	for (index = 0; index < num; index++) {
		int reserve = get_reserve(index);
		if (reserve < 0) {
			printf("Getting reserve at index %ld returned %d\n", index, reserve);
		}
		printf("Reserve %ld has ID %d\n", index, reserve);
	}
	printf("Done enumerating reserves.\n");

	return 0;
}

#define SYS_NUM_TAPS 371
long num_taps(void)
{
	return syscall(SYS_NUM_TAPS);
}

#define SYS_GET_TAP 372
int get_tap(long index)
{
	return syscall(SYS_GET_TAP, index);
}

int handle_list_taps()
{
	printf("Enumerating taps:\n");
	long index, num = num_taps();
	if (num < 0) {
		printf("Getting number of taps returned %ld\n", num);
		return num;
	}

	printf("Found %ld taps:\n", num);
	for (index = 0; index < num; index++) {
		int reserve = get_tap(index);
		if (reserve < 0) {
			printf("Getting tap at index %ld returned %d\n", index, reserve);
		}
		printf("Tap %ld has ID %d\n", index, reserve);
	}
	printf("Done enumerating taps.\n");

	return 0;
}

#define SYS_EXPOSE_RESERVE 373
long expose_reserve(pid_t pid, int reserve_id, unsigned int capabilities)
{
	return syscall(SYS_EXPOSE_RESERVE, pid, reserve_id, capabilities);
}

long handle_expose_reserve()
{
	int reserve_id;
	pid_t pid;
	long ret;
	char id[12], *retstr;
	unsigned int capabilities = 0;

	memset(id, 0, sizeof(id));
	printf("Please enter the reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	reserve_id = atoi(id);

	memset(id, 0, sizeof(id));
	printf("Please enter the target pid.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading pid.\n");
		return -EINVAL;
	}
	pid = atoi(id);

	memset(id, 0, sizeof(id));
	printf("Draw capability (y/n?) :");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading input.\n");
		return -EINVAL;
	}
	chomp(id);
	if (strcmp(id, "y") == 0) {
		printf("Adding draw capability.\n");
		capabilities |= CINDER_CAP_RESERVE_DRAW;
	}

	memset(id, 0, sizeof(id));
	printf("Modify capability (y/n?) :");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading input.\n");
		return -EINVAL;
	}
	chomp(id);
	if (strcmp(id, "y") == 0) {
		printf("Adding reserve modify capability.\n");
		capabilities |= CINDER_CAP_RESERVE_MODIFY;
	}

	ret = expose_reserve(pid, reserve_id, capabilities);
	printf("Expose reserve returned %ld\n", ret);
	return ret;
}

#define SYS_PUT_RESERVE 374
long put_reserve(int reserve_id)
{
	return syscall(SYS_PUT_RESERVE, reserve_id);
}

long handle_put_reserve()
{
	int reserve_id;
	long ret;
	char id[12], *retstr;

	memset(id, 0, sizeof(id));
	printf("Please enter the reserve id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	reserve_id = atoi(id);

	ret = put_reserve(reserve_id);
	printf("Put reserve returns %ld\n", ret);
	return ret;
}

#define SYS_INFO_TAP 375
long tap_info(int tap_id, struct tap_info *info)
{
	return syscall(SYS_INFO_TAP, tap_id, info);
}

int handle_info_tap()
{
	int tap_id;
	long ret;
	struct tap_info info;
	char id[12], *retstr;

	memset(&info, 0, sizeof(info));

	memset(id, 0, sizeof(id));
	printf("Please enter the tap id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	tap_id = atoi(id);

	ret = tap_info(tap_id, &info);
	if (ret) {
		printf("Error getting tap info: %ld\n", ret);
		return ret;
	}

	printf("Got tap info for tap ID %d Name %s Creator %d Rate %ld Type %lu Reserve TO: %ld Reserve FROM: %ld \n", info.id, info.name, info.creator, info.rate, info.type, info.reserve_to, info.reserve_from);
	return ret;
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

#define SYS_NUM_CHILD_LIST_RESERVES 380
long num_child_list_reserves(void)
{
	return syscall(SYS_NUM_CHILD_LIST_RESERVES);
}

#define SYS_GET_CHILD_LIST_RESERVE 381
int get_child_list_reserve(long index)
{
	return syscall(SYS_GET_CHILD_LIST_RESERVE, index);
}

#define SYS_DELETE_TAP 382
long delete_tap(int tap_id)
{
	return syscall(SYS_DELETE_TAP, tap_id);
}

int handle_delete_tap()
{
	int tap_id;
	long ret;
	struct tap_info info;
	char id[12], *retstr;

	memset(&info, 0, sizeof(info));

	memset(id, 0, sizeof(id));
	printf("Please enter the tap id.\n");
	retstr = fgets(id, sizeof(id), stdin);
	if (!retstr) {
		printf("Error reading id.\n");
		return -EINVAL;
	}
	tap_id = atoi(id);

	ret = delete_tap(tap_id);
	printf("Delete tap returned %ld\n", ret);
	return ret;
}

int handle_pid()
{
	pid_t pid = getpid();
	printf("Pid is %d\n", pid);
	return pid;
}

int handle_stats()
{
	// TODO: Needs syscall
	printf("stats\n");
	return -ENOSYS;
}

int handle_root_reserve()
{
	// TODO: Needs syscall
	printf("root reserve\n");
	return -ENOSYS;
}

int handle_battery()
{
	// TODO: Needs syscall
	printf("battery\n");
	return -ENOSYS;
}

int handle_list_all_taps()
{
	// TODO: Need system call
	printf("list all taps\n");
	return -ENOSYS;
}

int handle_list_all_reserves()
{
	// TODO: Need system call
	printf("list all reserves\n");
	return -ENOSYS;
}

