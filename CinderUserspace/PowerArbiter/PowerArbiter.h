#ifndef __POWER_ARBITER__
#define __POWER_ARBITER__

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <semaphore.h>

#include "common.h"
#include "sock.h"
#include "client.h"
#include "protocol.h"
#include "controller.h"
#include "dbase.h"
#include "syscall.h"
#include "init.h"

#define RESERVE_REAPER_DELAY_SECONDS 1

typedef struct __pa_context {

	pthread_mutex_t lock;
	int active;
	int num_users;
	int reaper_should_exit;
	int receiver_should_exit;

	sem_t reaper_thread_exited; // Has database reaper exited?

	ReserveDataDatabase *dbase;
	int listen_sock;
} PAContext;

#endif /* ifndef __POWER_ARBITER__ */

