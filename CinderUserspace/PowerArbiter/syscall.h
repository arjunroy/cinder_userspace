#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// Cinder system calls

#define CINDER_MAX_NAMELEN 20

#define CINDER_CAP_RESERVE_DRAW   0x1
#define CINDER_CAP_RESERVE_MODIFY 0x2
#define CINDER_CAP_ALL (CINDER_CAP_RESERVE_DRAW | CINDER_CAP_RESERVE_MODIFY)

#define CINDER_TAP_RATE_CONSTANT 1
#define CINDER_TAP_RATE_PROPORTIONAL 2

struct reserve_info {
	int id;
	long capacity, lifetime_input, lifetime_usage;
	char name[CINDER_MAX_NAMELEN];

	int num_users;
	
	/* TODO: Implement # taps */
	int num_process_taps;
};

int cinder_create_reserve(char *name, unsigned int len);
int cinder_create_tap(char __user *name, int len, int srcReserve, int destReserve);
long cinder_tap_set_rate(int tap_id, long rate_type, long value);
long cinder_put_reserve(int reserve_id);
long cinder_delete_tap(int tap_id);
long cinder_expose_reserve(pid_t pid, int reserve_id, unsigned int capabilities);

int cinder_get_num_reserve_users(long rsv_id, long *num_users);
int cinder_get_root_reserve_id(int *root_reserve_id);

#define SYS_CREATE_RESERVE 361
#define SYS_CREATE_TAP 362
#define SYS_SET_TAP_RATE 367
#define SYS_PUT_RESERVE 374
#define SYS_DELETE_TAP 382
#define SYS_EXPOSE_RESERVE 373
#define SYS_ROOT_RESERVE_ID 383
#define SYS_RESERVE_INFO 364

#endif

