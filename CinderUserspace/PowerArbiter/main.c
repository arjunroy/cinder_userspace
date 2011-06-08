#include <stdio.h>
#include <stdlib.h>

#include "PowerArbiter.h"

// Globals
PAContext *progctx = NULL;
sem_t keep_running_semaphore; // Should program keep running?

static int
__wait_for_sigterm()
{
	int err, ret;

	// Do wait
	while ((ret = sem_wait(&keep_running_semaphore)) == -1 && errno == EINTR)
		continue;

	// Error?
	if (ret == -1) {
		err = errno;
		return err;
	}

	// Success, dispose semaphore and return
	sem_destroy(&keep_running_semaphore);
	return 0;
}

static void
__sigterm_handler(int sig)
{
	sem_post(&keep_running_semaphore);
}

static int
__setup_sigterm_handler()
{
	int err;
	struct sigaction sa;

	sa.sa_handler = __sigterm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		err = errno;
		return err;
	}

	return 0;
}

static void *
reaper_thread_loop(void *arg)
{
	int err, is_empty, should_quit = 0;
	PAContext *progctx = arg;

	for (;;) {
		// Should we quit?
		err = pthread_mutex_lock(&progctx->lock);
		if (err)
			return (void *) err;

		should_quit = progctx->reaper_should_exit;
		pthread_mutex_unlock(&progctx->lock);

		// Clear unused database entries
		err = reserveDataDBClearUnused(progctx->dbase, &is_empty);

		// If we should keep running, then continue iterating.
		if (!should_quit)
			goto do_sleep;

		// If we should quit and database clear was error, quit anyways.
		if (err)
			break;

		// No error in last purge operation. If database is now empty, quit.
		if (is_empty)
			break;

		// Else, if we should quit, but database not empty, keep looping.
do_sleep:
		sleep(RESERVE_REAPER_DELAY_SECONDS);
	}

	// Before returning, signal that the reaper is quitting.
	sem_post(&progctx->reaper_thread_exited);
	return (void *) 0;
}

static void *
receiver_thread_loop(void *arg)
{
	int err, keep_running = 1, connected_sock;
	struct sockaddr_un remote_addr;	
	PAContext *progctx = arg;
	socklen_t remote_addrlen;
	
	for (;;) {	
		// First check if we keep running
		err = pthread_mutex_lock(&progctx->lock);
		keep_running = progctx->active;
		pthread_mutex_unlock(&progctx->lock);

		// If we're supposed to stop, then stop
		if (!keep_running)
			break;

		// Initialize
		memset(&remote_addr, 0, sizeof(remote_addr));
		remote_addrlen = sizeof(remote_addr);

		// Accept connection
		connected_sock = accept(progctx->listen_sock, (struct sockaddr *) &remote_addr,
			&remote_addrlen);

		// If error, log and continue
		if (connected_sock == -1) {
			err = errno;
			printf("Unable to accept connection: %d\n", err);
			LOG(LOG_INFO, PA_LOG_TAG, "Unable to accept connection: %d\n", err);
			continue;
		}

		// Handle client in its own thread
		err = handle_remote_client(connected_sock, &remote_addr, remote_addrlen, progctx);
		if (err) {
			printf("Unable to create client thread: %d\n", err);
			LOG(LOG_INFO, PA_LOG_TAG, "Unable to create client thread: %d\n", err);
		}
	}
	// If we're done, just return. Cleanup is performed by main thread.
	return (void *) 0;
}

int main(void)
{
	int err;
	pthread_t receiver_thread, reaper_thread;
	// Note progctx and keep_running_semaphore are globals.

	// Create keep_running_semaphore
	err = sem_init(&keep_running_semaphore, 0, 0); // Unshared, init 0
	if (err) {
		printf("Error creating keep_running_semaphore: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating keep_running_semaphore: %d\n", err);
		exit(EXIT_FAILURE);
	}

	// Set up sigterm handler
	err = __setup_sigterm_handler();
	if (err) {
		printf("Error setting up SIGTERM handler: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error setting up SIGTERM handler: %d\n", err);
		exit(EXIT_FAILURE);
	}

	// Create program context
	err = powerArbiterInitialize(&progctx);
	if (err) {
		printf("Error creating Power Arbiter Daemon: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating Power Arbiter Daemon: %d\n", err);
		exit(EXIT_FAILURE);
	}

	// Create reaper thread
	err = pthread_create(&reaper_thread, NULL, reaper_thread_loop, progctx);
	if (err) {
		printf("Error creating reserve reaper thread: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating reserve reaper thread: %d\n", err);
		exit(EXIT_FAILURE);
	}

	// Create thread for listening to packets
	err = pthread_create(&receiver_thread, NULL, receiver_thread_loop, progctx);
	if (err) {
		printf("Error creating receiver thread: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error creating receiver thread: %d\n", err);
		exit(EXIT_FAILURE);
	}

	// Block on sigterm for exiting
	err = __wait_for_sigterm();
	if (err) {
		printf("Error waiting for SIGTERM: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error waiting for SIGTERM: %d\n", err);
		exit(EXIT_FAILURE);
	}

	// Call destroy on program context
	err = powerArbiterDestroy(progctx);
	if (err) {
		printf("Error destroying Power Arbiter daemon: %d\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error destroying Power Arbiter daemon: %d\n", err);
		exit(EXIT_FAILURE);
	}

	// And we're done.
	printf("Exiting Power Arbiter daemon: %d\n", err);
	LOG(LOG_INFO, PA_LOG_TAG, "Exiting Power Arbiter daemon: %d\n", err);
	return 0;
}

