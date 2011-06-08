#include "PowerArbiter.h"

static int
__handle_stat_reserve(ClientData *ctx)
{
	int err;
	PAStatReserveQuery query;
	PAStatReserveResponse response;
	uint64_t num_bytes_read = 0, bytes_to_read;
	uint64_t num_bytes_sent = 0, bytes_to_send;
	uid_t uid;
	int32_t rid;
	uint32_t flags;

	bytes_to_read = (uint64_t) sizeof(query);

	// Read request
	err = streamsock_read(ctx->client_sock, (uint8_t *) &query, 
		bytes_to_read, bytes_to_read, &num_bytes_read);
	if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_STAT_RESERVE: Could not read input : %d\n", err);
		return err;
	}
	if (num_bytes_read < bytes_to_read) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_STAT_RESERVE: Unexpected bytes when reading input : %d\n", err);
		return -EPROTO; // Unexpected shutdown is a protocol error...
	}

	// Get UID
	uid = (uid_t) query.uid;

	// Stat the reserve
	err = statReserve(ctx->progctx, uid, ctx->pid, 
		ctx->uid, &rid, &flags);

	// If error occurred, send error response.
	if (err) {
		response.rid = 0;
		response.error = PA_FAILURE;
		response.flags = 0;
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_STAT_RESERVE: Operation failed : %d\n", err);
	}
	else {
		// Success, either target process has been exposed access to (possibly 
		// newly created) reserve or must use root reserve. Send back response.
		response.rid = rid;
		response.error = PA_NO_ERROR;
		response.flags = flags;
	}

	bytes_to_send = sizeof(response);

	return streamsock_send(ctx->client_sock, (uint8_t *) &response, bytes_to_send, 
		&num_bytes_sent);
}

static int
__handle_get_tap_params(ClientData *ctx)
{
	int err;
	PAGetTapParamsQuery query;
	PAGetTapParamsResponse response;
	uint64_t num_bytes_read = 0, bytes_to_read;
	uint64_t num_bytes_sent = 0, bytes_to_send;
	uid_t uid;
	uint64_t tap_type;
	int64_t tap_value;

	bytes_to_read = (uint64_t) sizeof(query);

	// Read request
	err = streamsock_read(ctx->client_sock, (uint8_t *) &query, 
		bytes_to_read, bytes_to_read, &num_bytes_read);
	if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_GET_TAP_PARAMS: Could not read input : %d\n", err);
		return err;
	}
	if (num_bytes_read < bytes_to_read) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_GET_TAP_PARAMS: Unexpected bytes when reading input : %d\n", err);
		return -EPROTO; // Unexpected shutdown is a protocol error...
	}

	// Get UID
	uid = (uid_t) query.uid;

	// Get tap parameters
	err = getTapParams(ctx->progctx, uid, ctx->pid, 
		ctx->uid, &tap_type, &tap_value);
	if (err == -ENOENT) {
		// Reserve not found
		response.tap_type = 0;
		response.tap_value = 0;
		response.error = PA_UID_NOT_FOUND;
		err = 0;
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_GET_TAP_PARAMS: UID mapping not found : %d\n", err);
	}
	else if (err == -EPERM) {
		// Not allowed
		response.tap_type = 0;
		response.tap_value = 0;
		response.error = PA_BAD_PERMISSIONS;
		err = 0;
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_GET_TAP_PARAMS: Bad permissions for client %d\n", ctx->pid);
	}
	else if (err) {
		// Failed to get data
		response.tap_type = 0;
		response.tap_value = 0;
		response.error = PA_FAILURE;
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_GET_TAP_PARAMS: Operation failed : %d\n", err);
	}
	else {
		// No error, return tap parameters.
		response.tap_type = tap_type;
		response.tap_value = tap_value;
		response.error = PA_NO_ERROR;
	}

	bytes_to_send = sizeof(response);

	return streamsock_send(ctx->client_sock, (uint8_t *) &response, bytes_to_send, 
		&num_bytes_sent);
}

static int
__handle_set_tap_params(ClientData *ctx)
{
	int err;
	PASetTapParamsQuery query;
	PASetTapParamsResponse response;
	uint64_t num_bytes_read = 0, bytes_to_read;
	uint64_t num_bytes_sent = 0, bytes_to_send;
	uid_t uid;
	uint64_t tap_type;
	int64_t tap_value;
	uint32_t error;

	bytes_to_read = (uint64_t) sizeof(query);

	// Read request
	err = streamsock_read(ctx->client_sock, (uint8_t *) &query, 
		bytes_to_read, bytes_to_read, &num_bytes_read);
	if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_SET_TAP_PARAMS: Could not read input : %d\n", err);
		return err;
	}
	if (num_bytes_read < bytes_to_read) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_SET_TAP_PARAMS: Unexpected bytes when reading input : %d\n", err);
		return -EPROTO; // Unexpected shutdown is a protocol error...
	}

	// Get UID
	uid = (uid_t) query.uid;
	tap_type = query.tap_type;
	tap_value = query.tap_value;

	err = setTapParams(ctx->progctx, uid, ctx->pid, ctx->uid, tap_type, 
		tap_value, &error);
	if (err == -EPERM) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_SET_TAP_PARAMS: Bad permissions for client %d\n", ctx->pid);
		response.error = PA_BAD_PERMISSIONS;
	}
	else if (err == -EINVAL && error == PA_INVALID_INPUT) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_SET_TAP_PARAMS: Bad input\n");
		response.error = PA_INVALID_INPUT;
	}
	else if (err == -ENOENT) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_SET_TAP_PARAMS: UID mapping not found for %d\n", uid);
		response.error = PA_UID_NOT_FOUND;
	}
	else if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_SET_TAP_PARAMS: Operation failed : %d\n", err);
		response.error = PA_FAILURE;
	}
	else {
		response.error = PA_NO_ERROR;
	}

	bytes_to_send = sizeof(response);

	return streamsock_send(ctx->client_sock, (uint8_t *) &response, bytes_to_send, 
		&num_bytes_sent);
}

static int
__handle_add_params_for_uid(ClientData *ctx)
{
	int err;
	PAAddUIDParamsQuery query;
	PAAddUIDParamsResponse response;
	uint64_t num_bytes_read = 0, bytes_to_read;
	uint64_t num_bytes_sent = 0, bytes_to_send;
	uid_t uid;
	uint64_t tap_type;
	int64_t tap_value;
	uint32_t error;

	bytes_to_read = (uint64_t) sizeof(query);

	// Read request
	err = streamsock_read(ctx->client_sock, (uint8_t *) &query, 
		bytes_to_read, bytes_to_read, &num_bytes_read);
	if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_ADD_PARAMS: Could not read input : %d\n", err);
		return err;
	}
	if (num_bytes_read < bytes_to_read) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_ADD_PARAMS: Unexpected bytes when reading input : %d\n", err);
		return -EPROTO; // Unexpected shutdown is a protocol error...
	}

	// Get UID
	uid = (uid_t) query.uid;
	tap_type = query.tap_type;
	tap_value = query.tap_value;

	err = addReserveParamsForUID(ctx->progctx, uid, ctx->pid, ctx->uid, 
		tap_type, tap_value, &error);
	if (err == -EEXIST) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_ADD_PARAMS: UID mapping already exists for %d\n", uid);
		response.error = PA_UID_EXISTS;
	}
	else if (err == -EINVAL && error == PA_INVALID_INPUT) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_ADD_PARAMS: Bad input\n");
		response.error = PA_INVALID_INPUT;
	}
	else if (err == -EPERM) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_ADD_PARAMS: Bad permissions for client %d\n", ctx->pid);
		response.error = PA_BAD_PERMISSIONS;
	}
	else if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_ADD_PARAMS: Operation failed : %d\n", err);
		response.error = PA_FAILURE;
	}
	else {
		response.error = PA_NO_ERROR;
	}

	bytes_to_send = sizeof(response);

	return streamsock_send(ctx->client_sock, (uint8_t *) &response, bytes_to_send, 
		&num_bytes_sent);
}

static int
__handle_remove_params_for_uid(ClientData *ctx)
{
	int err;
	PARemoveUIDParamsQuery query;
	PARemoveUIDParamsResponse response;
	uint64_t num_bytes_read = 0, bytes_to_read;
	uint64_t num_bytes_sent = 0, bytes_to_send;
	uid_t uid;

	bytes_to_read = (uint64_t) sizeof(query);

	// Read request
	err = streamsock_read(ctx->client_sock, (uint8_t *) &query, 
		bytes_to_read, bytes_to_read, &num_bytes_read);
	if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_REMOVE_PARAMS: Could not read input : %d\n", err);
		return err;
	}
	if (num_bytes_read < bytes_to_read) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_REMOVE_PARAMS: Unexpected bytes when reading input : %d\n", err);
		return -EPROTO; // Unexpected shutdown is a protocol error...
	}

	// Get UID
	uid = (uid_t) query.uid;

	err = removeReserveParamsForUID(ctx->progctx, uid, ctx->pid, ctx->uid);
	if (err == -EPERM) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_REMOVE_PARAMS: Bad permissions for client %d\n", ctx->pid);
		response.error = PA_BAD_PERMISSIONS;
	}
	else if (err == -ENOENT) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_REMOVE_PARAMS: UID mapping for %d not found\n", uid);
		response.error = PA_UID_NOT_FOUND;
	}
	else if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "HANDLE_REMOVE_PARAMS: Operation failed : %d\n", err);
		response.error = PA_FAILURE;
	}
	else {
		response.error = PA_NO_ERROR;
	}

	bytes_to_send = sizeof(response);

	return streamsock_send(ctx->client_sock, (uint8_t *) &response, bytes_to_send, 
		&num_bytes_sent);
}

static int
__read_pa_request(ClientData *ctx, PARequest *request)
{
	int err;
	uint64_t num_bytes_read = 0, bytes_to_read;

	bytes_to_read = (uint64_t) sizeof(*request);

	err = streamsock_read(ctx->client_sock, (uint8_t *) request, 
		bytes_to_read, bytes_to_read, &num_bytes_read);
	if (err) {
		LOG(LOG_INFO, PA_LOG_TAG, "Failed to read command from client with PID %d: %d\n", ctx->pid, err);
		return err;
	}
	if (num_bytes_read < bytes_to_read) {
		LOG(LOG_INFO, PA_LOG_TAG, "Unexpected bytes when reading command from client with PID %d: %d\n", ctx->pid, err);
		return -EPROTO; // Unexpected shutdown is a protocol error...
	}

	// If we read all we wanted, we're done
	return 0;
}

static void *
__do_handle_client(void *arg)
{
	// Protocol errors mean we fail fast, send no response, and kill
	// the socket. Operation errors mean we continue the session and send
	// a response giving the status of how the failure occured.
	int err;
	PARequest request;
	ClientData *ctx = arg;

	for (;;) {
		err = __read_pa_request(ctx, &request);
		if (err)
			goto out;

		if (request.request_type == PA_GOODBYE) {
			LOG(LOG_INFO, PA_LOG_TAG, "Client with PID %d disconnected.\n", ctx->pid);
			break;
		}

		switch (request.request_type) {
			case PA_STAT_RESERVE:
				LOG(LOG_INFO, PA_LOG_TAG, "Client with PID %d called stat reserve.\n", ctx->pid);
				err = __handle_stat_reserve(ctx);
				if (err)
					goto out;
				break;
			case PA_GET_TAP_PARAMS:
				LOG(LOG_INFO, PA_LOG_TAG, "Client with PID %d wants reserve params.\n", ctx->pid);
				err = __handle_get_tap_params(ctx);
				if (err)
					goto out;
				break;
			case PA_SET_TAP_PARAMS:
				LOG(LOG_INFO, PA_LOG_TAG, "Client with PID %d wants to set reserve params.\n", ctx->pid);
				err = __handle_set_tap_params(ctx);
				if (err)
					goto out;
				break;
			case PA_ADD_UID_PARAMS:
				LOG(LOG_INFO, PA_LOG_TAG, "Client with PID %d wants to add UID<->reserve mapping.\n", ctx->pid);
				err = __handle_add_params_for_uid(ctx);
				if (err)
					goto out;
				break;
			case PA_REMOVE_UID_PARAMS:
				LOG(LOG_INFO, PA_LOG_TAG, "Client with PID %d wants to remove UID<->reserve mapping.\n", ctx->pid);
				err = __handle_remove_params_for_uid(ctx);
				if (err)
					goto out;
				break;
				printf("NOTE: Should not come here.\n");
			default:
				// Fail fast if invalid request
				LOG(LOG_INFO, PA_LOG_TAG, "Error: Client with PID %d made invalid request.\n", ctx->pid);
				err = -EINVAL;
				goto out;
		}
	}

	err = 0;
out:
	LOG(LOG_INFO, PA_LOG_TAG, "Closing connection to client with PID %d.\n", ctx->pid);	
	close(ctx->client_sock);
	free(ctx);
	return (void *) err;
}

int 
handle_remote_client(int connsock, struct sockaddr_un *remote_addr, 
	socklen_t remote_addrlen, PAContext *progctx)
{
	int err, ret;
	uid_t uid;
	gid_t gid;
	pid_t pid;
	ClientData *ctx;

	// Get peer credentials
	err = get_peer_id(connsock, &pid, &uid, &gid);
	if (err) {
		ret = err;
		printf("Unable to get peer id: %d\n", ret);
		close(connsock);
		return ret;
	}

	// Allocate data for client handler thread
	ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		close(connsock);
		return -ENOMEM;
	}

	ctx->pid = pid;
	ctx->uid = uid;
	ctx->gid = gid;
	ctx->client_sock = connsock;
	ctx->progctx = progctx;

	// Create handler thread, pass it context
	err = pthread_create(&ctx->thread, NULL, __do_handle_client, ctx);
	if (err) {
		close(connsock);
		free(ctx);
		printf("Unable to create thread: %d\n", err);
		return err;
	}

	// Success
	return 0;
}

