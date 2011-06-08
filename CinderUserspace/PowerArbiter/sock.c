#include "PowerArbiter.h"

int
create_listening_ud_sock(const char *path, int backlog, int *sock_loc)
{
	int err, ret;
	struct sockaddr_un address;
	int socket_fd, connection_fd;
	socklen_t address_length;

	if (!path || !sock_loc)
		return -EINVAL;

	LOG(LOG_INFO, PA_LOG_TAG, "Creating listening unix domain socket at %s\n", 
		path);

	// Create socket
	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		ret = errno;
		LOG(LOG_INFO, PA_LOG_TAG, "Error : socket() : %d\n", ret);
		return ret;
	}

	// Unlink existing socket
	unlink(path);
	/*	
	if (err) {
		ret = errno;
		close(socket_fd);
		LOG(LOG_INFO, PA_LOG_TAG, "Error : unlink() : %d\n", ret);
		return ret;
	}
	*/

	// Set up address
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, sizeof(address.sun_path), "%s", path);
	address.sun_path[sizeof(address.sun_path) - 1] = '\0';
	address_length = sizeof(address.sun_family) + strlen(address.sun_path);

	// Bind socket
	err = bind(socket_fd, (struct sockaddr *) &address, address_length);
	if (err) {
		ret = errno;
		close(socket_fd);
		LOG(LOG_INFO, PA_LOG_TAG, "Error : bind() : %d\n", ret);
		return ret;		
	}

	// Listen for connections
	err = listen(socket_fd, backlog);
	if (err) {
		ret = errno;
		close(socket_fd);
		LOG(LOG_INFO, PA_LOG_TAG, "Error : listen() : %d\n", ret);
		return ret;
	}

	LOG(LOG_INFO, PA_LOG_TAG, "Created listening unix domain socket.\n");
	*sock_loc = socket_fd;
	return 0;
}

int 
dispose_listening_ud_sock(int socket_fd, const char *path)
{
	if (!path || socket_fd == -1)
		return -EINVAL;

	close(socket_fd);
	unlink(path);
	LOG(LOG_INFO, PA_LOG_TAG, "Disposed listening unix domain socket at %s.\n",
		path);
	return 0;
}

int 
get_peer_id(int s, pid_t *pid, uid_t *uid, gid_t *gid)
{
	struct ucred credentials;
	int err, ret, ucred_length = sizeof(struct ucred);

	if (s == -1 || !uid || !gid || !pid)
		return -EINVAL;
	
	err = getsockopt(s, SOL_SOCKET, SO_PEERCRED, &credentials, &ucred_length);
	if (err) {
		ret = errno;
		return ret;
	}

	*pid = credentials.pid;
	*uid = credentials.uid;
	*gid = credentials.gid;
	return 0;
}

int
streamsock_read(int sock, uint8_t *buf, uint64_t bufsiz, uint64_t to_read, 
	uint64_t *num_bytes_read)
{
	int err;
	ssize_t recv_ret;
	uint8_t *buf_offset = buf;
	uint64_t rcvd = 0, remaining = to_read;

	if (sock == -1 || !buf || to_read > bufsiz || !num_bytes_read)
		return -EINVAL;

	while (rcvd < to_read) {
		recv_ret = recv(sock, buf_offset, remaining, 0);
		if (recv_ret == -1) {
			// Error
			err = errno;
			return err;
		}
		if (recv_ret == 0) {
			// Orderly shutdown
			break;
		}

		rcvd += (uint64_t) recv_ret;
		remaining -= (uint64_t) recv_ret;
		buf_offset += (ptrdiff_t) recv_ret;
	}

	*num_bytes_read = rcvd;
	printf("Read %lld bytes.\n", rcvd);
	return 0;
}

int
streamsock_send(int sock, const uint8_t *buf, uint64_t to_send, 
	uint64_t *num_bytes_sent)
{
	int err;
	ssize_t send_ret;
	const uint8_t *buf_offset = buf;
	uint64_t sent = 0, remaining = to_send;

	if (sock == -1 || !buf || !num_bytes_sent)
		return -EINVAL;

	while (sent < to_send) {
		send_ret = send(sock, buf_offset, remaining, 0);
		if (send_ret == -1) {
			err = errno;
			return err;
		}

		sent += (uint64_t) send_ret;
		remaining -= (uint64_t) send_ret;
		buf_offset += (ptrdiff_t) send_ret;
	}

	*num_bytes_sent = sent;
	return 0;
}

