#ifndef __CLIENT_H__
#define __CLIENT_H__

struct __pa_context;

typedef struct __uds_client_data {
	int client_sock;
	uid_t uid;
	gid_t gid;
	pid_t pid;
	pthread_t thread;
	struct __pa_context *progctx;
} ClientData;

int handle_remote_client(int connsock, struct sockaddr_un *remote_addr, 
	socklen_t remote_addrlen, struct __pa_context *progctx);

#endif /* ifndef __CLIENT_H__ */

