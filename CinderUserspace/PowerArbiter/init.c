#include "PowerArbiter.h"

int 
powerArbiterInitialize(PAContext **progctx_loc)
{
	int err;
	PAContext *progctx = NULL;

	if (!progctx_loc)
		return -EINVAL;

	// Allocate context
	progctx = malloc(sizeof(*progctx));
	if (!progctx)
		return -ENOMEM;

	// Create mutex
	err = pthread_mutex_init(&progctx->lock, NULL);
	if (err) {
		printf("Error creating database mutex: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating database mutex: %d\n", err);
		goto free_ctx;
	}

	// Create database
	err = reserveDataDBCreateEmpty(&progctx->dbase);
	if (err) {
		printf("Error creating empty database: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating empty database: %d\n", err);
		goto free_mutex;
	}

	// Create reaper thread status semaphore
	err = sem_init(&progctx->reaper_thread_exited, 0, 0); // Unshared, init 0
	if (err) {
		err = errno;
		printf("Error creating reaper thread status semaphore: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating reaper thread status semaphore: %d\n", err);
		goto free_db;
	}

	// Initialize listening socket connection in fs
	err = create_listening_ud_sock(POWER_ARBITER_SOCKPATH, 
		DEFAULT_LISTEN_BACKLOG, &progctx->listen_sock);
	if (err) {
		printf("Error creating listening UDS at %s: %d\n", 
			POWER_ARBITER_SOCKPATH, err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating listening UDS at %s: %d\n", 
			POWER_ARBITER_SOCKPATH, err);
		goto free_reaper_status;
	}

	// Success
	progctx->active = 1;
	progctx->num_users = 0;
	progctx->reaper_should_exit = 0;
	progctx->receiver_should_exit = 0;
	*progctx_loc = progctx;
	return 0;

free_reaper_status:
	sem_destroy(&progctx->reaper_thread_exited);
free_db:
	reserveDataDBDestroy(progctx->dbase);
free_mutex:
	pthread_mutex_destroy(&progctx->lock);
free_ctx:
	free(progctx);
	return err;
}

static int
__wait_for_client_exit(PAContext *progctx)
{
	int err, num_clients;

	do {
		err = pthread_mutex_lock(&progctx->lock);
		if (err)
			return err;

		num_clients = progctx->num_users;
		pthread_mutex_unlock(&progctx->lock);

		if (num_clients == 0)
			break;
	} while(1);

	return 0;
}

static int
__signal_reaper_exit(PAContext *progctx)
{
	int err;

	err = pthread_mutex_lock(&progctx->lock);
	if (err)
		return err;

	progctx->reaper_should_exit = 1;
	pthread_mutex_unlock(&progctx->lock);
	return 0;
}

static int
__signal_arbiter_inactive(PAContext *progctx)
{
	int err;

	err = pthread_mutex_lock(&progctx->lock);
	if (err)
		return err;

	progctx->active = 0;
	pthread_mutex_unlock(&progctx->lock);
	return 0;
}

static int
__wait_reaper_thread_quit(PAContext *progctx)
{
	int err, ret;

	// Do wait
	while ((ret = sem_wait(&progctx->reaper_thread_exited)) == -1 && errno == EINTR)
		continue;

	// Error?
	if (ret == -1) {
		err = errno;
		return err;
	}

	// Success
	return 0;
}

int powerArbiterDestroy(PAContext *progctx)
{
	int err, ret;
	if (!progctx)
		return -EINVAL;

	// Called destroy due to SIGTERM. We set ourselves to inactive first
	err = __signal_arbiter_inactive(progctx);
	if (err)
		return err;

	// Dispose the listening socket
	err = dispose_listening_ud_sock(progctx->listen_sock, POWER_ARBITER_SOCKPATH);
	if (err)
		return err;

	// Wait for all clients to exit
	err = __wait_for_client_exit(progctx);
	if (err)
		return err;

	// After all clients done, empty out active list in database
	err = reserveDataDBEmpty(progctx->dbase);
	if (err)
		return err;

	// Signal database reaper thread that it's time to quit.
	err = __signal_reaper_exit(progctx);
	if (err)
		return err;

	// Wait for database reaper thread to quit.
	err = __wait_reaper_thread_quit(progctx);
	if (err)
		return err;

	// At completion, database is ready to be finally freed.
	err = reserveDataDBDestroy(progctx->dbase);
	if (err)
		return err;

	// Dispose of the active lock and all semaphores
	pthread_mutex_destroy(&progctx->lock);
	sem_destroy(&progctx->reaper_thread_exited);

	// Free context and return
	free(progctx);
	return 0;	
}

