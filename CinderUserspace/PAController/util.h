#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 30
#define CINDER_MAX_NAMELEN 20

#define CINDER_CAP_RESERVE_DRAW   0x1
#define CINDER_CAP_RESERVE_MODIFY 0x2

#define CINDER_TAP_RATE_CONSTANT 1
#define CINDER_TAP_RATE_PROPORTIONAL 2

#define POWER_ARBITER_SOCKPATH "/data/power_arbiter"

struct reserve_info {
	int id;
	long capacity, lifetime_input, lifetime_usage;
	char name[CINDER_MAX_NAMELEN];

	int num_users;
	
	/* TODO: Implement # taps */
	int num_process_taps;
};

struct tap_info {
	int id;
	long rate;
	unsigned long type;

	long reserve_to;
	long reserve_from;

	char name[CINDER_MAX_NAMELEN];
	pid_t creator;
};

void chomp(char *str);

long handle_put_reserve();
int handle_list_reserves();
int handle_help();
int handle_add_reserve_mapping(int socket_fd);
int handle_del_reserve_mapping(int socket_fd);
int handle_get_reserve_mapping_data(int socket_fd);
int handle_set_reserve_mapping_data(int socket_fd);
int handle_connect_to_uid_reserve(int socket_fd);
long handle_info_reserve();
int disconnectFromServer(int socket_fd);

#endif /* ifndef __UTIL_H__ */
