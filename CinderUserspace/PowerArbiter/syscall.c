#include "PowerArbiter.h"
#include <sys/syscall.h>

int 
cinder_create_reserve(char *name, unsigned int len)
{
	return syscall(SYS_CREATE_RESERVE, name, len);
}

int 
cinder_create_tap(char *name, int len, int srcReserve, int destReserve)
{
	return syscall(SYS_CREATE_TAP, name, len, srcReserve, destReserve);
}

long 
cinder_tap_set_rate(int tap_id, long rate_type, long value)
{
	return syscall(SYS_SET_TAP_RATE, tap_id, rate_type, value);
}

long 
cinder_put_reserve(int reserve_id)
{
	return syscall(SYS_PUT_RESERVE, reserve_id);
}

long 
cinder_delete_tap(int tap_id)
{
	return syscall(SYS_DELETE_TAP, tap_id);
}

long 
cinder_expose_reserve(pid_t pid, int reserve_id, unsigned int capabilities)
{
	return syscall(SYS_EXPOSE_RESERVE, pid, reserve_id, capabilities);
}

int 
cinder_get_num_reserve_users(long rsv_id, long *num_users)
{
	int ret;
	struct reserve_info rinfo;

	if (!num_users)
		return -EINVAL;

	ret = syscall(SYS_RESERVE_INFO, rsv_id, &rinfo);
	if (ret)
		return ret;

	*num_users = rinfo.num_users;
	return 0;
}

int cinder_get_root_reserve_id(int *root_reserve_id)
{
	int ret;

	if (!root_reserve_id)
		return -EINVAL;

	*root_reserve_id = syscall(SYS_ROOT_RESERVE_ID);
	return 0;
}

