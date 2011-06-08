#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>

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
    printf("Done sending %lld bytes\n", sent);
	return 0;
}
