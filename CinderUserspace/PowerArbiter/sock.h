#ifndef __SOCK_H__
#define __SOCK_H__

#define POWER_ARBITER_SOCKPATH "/data/power_arbiter"
#define DEFAULT_LISTEN_BACKLOG 5

int create_listening_ud_sock(const char *path, int backlog, int *sock_loc);
int dispose_listening_ud_sock(int socket_fd, const char *path);

int get_peer_id(int s, pid_t *pid, uid_t *uid, gid_t *gid);

int streamsock_read(int sock, uint8_t *buf, uint64_t bufsiz, uint64_t to_read, 
	uint64_t *num_bytes_read);

int streamsock_send(int sock, const uint8_t *buf, uint64_t to_send, 
	uint64_t *num_bytes_sent);

#endif /* ifndef __SOCK_H__ */

