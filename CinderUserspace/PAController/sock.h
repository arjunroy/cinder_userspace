#ifndef __SOCK_H__
#define __SOCK_H__

int streamsock_read(int sock, uint8_t *buf, uint64_t bufsiz, uint64_t to_read, 
	uint64_t *num_bytes_read);

int streamsock_send(int sock, const uint8_t *buf, uint64_t to_send, 
	uint64_t *num_bytes_sent);

#endif /* ifndef __SOCK_H__ */

