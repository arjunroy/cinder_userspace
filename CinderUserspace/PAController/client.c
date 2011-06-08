#include "util.h"
#include "sock.h"
#include "protocol.h"

static void
__handle_response(uint32_t error)
{
    // Handle response
    if (error == PA_UID_EXISTS) {
        printf("Error: UID already exists.\n");
    }
    else if (error == PA_INVALID_INPUT) {
        printf("Error: Invalid input received.\n");
    }
    else if (error == PA_BAD_PERMISSIONS) {
        printf("Error: Bad permissions.\n");
    }
    else if (error == PA_FAILURE) {
        printf("Error: Operation failed.\n");
    }
    else if (error == PA_NO_ERROR) {
        printf("Success: Operation completed.\n");
    }
    else if (error == PA_UID_NOT_FOUND) {
        printf("Error: UID not found.\n");
    }
    else {
        printf("Error: Unexpected error condition: %ud\n", error);
    }
}

int
handle_add_reserve_mapping(int socket_fd)
{
    int ret;
    uid_t uid = 0;
    long tap_value = 0, tap_type = CINDER_TAP_RATE_CONSTANT;
    char amount_str[22], input[22], *retstr;
    PAAddUIDParamsQuery query;
    PAAddUIDParamsResponse response;
    PARequest requestType;
    uint64_t num_transferred;

    printf("Adding new reserve mapping.\n");

    // Get UID
    memset(amount_str, 0, sizeof(amount_str));
    printf("Input UID for new reserve mapping: ");
    retstr = fgets(amount_str, sizeof(amount_str), stdin);
    if (!retstr) {
        printf("Error reading amount.\n");
        return -EINVAL;
    }
    uid = atol(amount_str);

    // Get tap value
    memset(amount_str, 0, sizeof(amount_str));
    printf("Input tap value for new reserve mapping: ");
    retstr = fgets(amount_str, sizeof(amount_str), stdin);
    if (!retstr) {
        printf("Error reading amount.\n");
        return -EINVAL;
    }
    tap_value = atol(amount_str);
    if (tap_value < 0) {
        printf("Tap value must be > 0\n");
        return -EINVAL;
    }

    // Get tap type
    memset(input, 0, sizeof(input));
    printf("Enter reserve type : (c)onstant/(p)roportional\n");
    retstr = fgets(input, sizeof(input), stdin);
    if (!retstr) {
        printf("Error reading input.\n");
        return -EINVAL;
    }
    chomp(input);
    if (strcmp("c", input) == 0 || strcmp("C", input) == 0) {
        tap_type = CINDER_TAP_RATE_CONSTANT;
    }
    else if (strcmp("p", input) == 0 || strcmp("P", input) == 0) {
        tap_type = CINDER_TAP_RATE_PROPORTIONAL;
    }
    else {
        printf("Invalid tap type specified.\n");
        return -EINVAL;
    }

    // Set up packet
    query.uid = uid;
    query.tap_value = tap_value;
    query.tap_type = tap_type;

    requestType.request_type = PA_ADD_UID_PARAMS;

    // Send request
    ret = streamsock_send(socket_fd, (uint8_t *)&requestType, sizeof(requestType),
        &num_transferred);
    if (ret || num_transferred != sizeof(requestType)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    ret = streamsock_send(socket_fd, (uint8_t *)&query, sizeof(query),
        &num_transferred);
    if (ret || num_transferred != sizeof(query)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    // Get response
    ret = streamsock_read(socket_fd, (uint8_t *)&response, sizeof(response), sizeof(response),
        &num_transferred);
    if (ret || num_transferred != sizeof(response)) {
        printf("Error receiving bytes: %d\n", ret);
        return ret;
    }

    // Handle response
    __handle_response(response.error);
    return 0;
}

int
handle_del_reserve_mapping(int socket_fd)
{
    int ret;
    uid_t uid = 0;
    char amount_str[22], *retstr;
    PARemoveUIDParamsQuery query;
    PARemoveUIDParamsResponse response;
    PARequest requestType;
    uint64_t num_transferred;

    printf("Deleting reserve mapping.\n");

    // Get UID
    memset(amount_str, 0, sizeof(amount_str));
    printf("Input UID: ");
    retstr = fgets(amount_str, sizeof(amount_str), stdin);
    if (!retstr) {
        printf("Error reading amount.\n");
        return -EINVAL;
    }
    uid = atol(amount_str);

    // set up packet
    query.uid = uid;
    requestType.request_type = PA_REMOVE_UID_PARAMS;

    // Send request
    ret = streamsock_send(socket_fd, (uint8_t *)&requestType, sizeof(requestType),
        &num_transferred);
    if (ret || num_transferred != sizeof(requestType)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    ret = streamsock_send(socket_fd, (uint8_t *)&query, sizeof(query),
        &num_transferred);
    if (ret || num_transferred != sizeof(query)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    // Get response
    ret = streamsock_read(socket_fd, (uint8_t *)&response, sizeof(response), sizeof(response),
        &num_transferred);
    if (ret || num_transferred != sizeof(response)) {
        printf("Error receiving bytes: %d\n", ret);
        return ret;
    }

    // Handle response
    __handle_response(response.error);
    return 0;
}

int
handle_get_reserve_mapping_data(int socket_fd)
{
    int ret;
    uid_t uid = 0;
    char amount_str[22], *retstr;
    PAGetTapParamsQuery query;
    PAGetTapParamsResponse response;
    PARequest requestType;
    uint64_t num_transferred;

    printf("Get reserve <-> UID parameters.\n");

    // Get UID
    memset(amount_str, 0, sizeof(amount_str));
    printf("Input UID: ");
    retstr = fgets(amount_str, sizeof(amount_str), stdin);
    if (!retstr) {
        printf("Error reading amount.\n");
        return -EINVAL;
    }
    uid = atol(amount_str);

    // set up packet
    query.uid = uid;
    requestType.request_type = PA_GET_TAP_PARAMS;

    // Send request
    ret = streamsock_send(socket_fd, (uint8_t *)&requestType, sizeof(requestType),
        &num_transferred);
    if (ret || num_transferred != sizeof(requestType)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    ret = streamsock_send(socket_fd, (uint8_t *)&query, sizeof(query),
        &num_transferred);
    if (ret || num_transferred != sizeof(query)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    // Get response
    ret = streamsock_read(socket_fd, (uint8_t *)&response, sizeof(response), sizeof(response),
        &num_transferred);
    if (ret || num_transferred != sizeof(response)) {
        printf("Error receiving bytes: %d\n", ret);
        return ret;
    }

    __handle_response(response.error);
    if (response.error == PA_NO_ERROR) {
        printf("UID %d has mapping with tap type %s and tap value %lld\n", uid,
               (response.tap_type == CINDER_TAP_RATE_CONSTANT ? "Constant" :
               (response.tap_type == CINDER_TAP_RATE_PROPORTIONAL ? "Proportion" :
               "Unknown")), response.tap_value);
    }
    return 0;
}

int
handle_set_reserve_mapping_data(int socket_fd)
{
    int ret;
    uid_t uid = 0;
    long tap_value = 0, tap_type = CINDER_TAP_RATE_CONSTANT;
    char amount_str[22], input[22], *retstr;
    PASetTapParamsQuery query;
    PASetTapParamsResponse response;
    PARequest requestType;
    uint64_t num_transferred;

    printf("Setting reserve <-> UID mapping params.\n");

    // Get UID
    memset(amount_str, 0, sizeof(amount_str));
    printf("Input UID for new reserve mapping: ");
    retstr = fgets(amount_str, sizeof(amount_str), stdin);
    if (!retstr) {
        printf("Error reading amount.\n");
        return -EINVAL;
    }
    uid = atol(amount_str);

    // Get tap value
    memset(amount_str, 0, sizeof(amount_str));
    printf("Input tap value for new reserve mapping: ");
    retstr = fgets(amount_str, sizeof(amount_str), stdin);
    if (!retstr) {
        printf("Error reading amount.\n");
        return -EINVAL;
    }
    tap_value = atol(amount_str);
    if (tap_value < 0) {
        printf("Tap value must be > 0\n");
        return -EINVAL;
    }

    // Get tap type
    memset(input, 0, sizeof(input));
    printf("Enter reserve type : (c)onstant/(p)roportional\n");
    retstr = fgets(input, sizeof(input), stdin);
    if (!retstr) {
        printf("Error reading input.\n");
        return -EINVAL;
    }
    chomp(input);
    if (strcmp("c", input) == 0 || strcmp("C", input) == 0) {
        tap_type = CINDER_TAP_RATE_CONSTANT;
    }
    else if (strcmp("p", input) == 0 || strcmp("P", input) == 0) {
        tap_type = CINDER_TAP_RATE_PROPORTIONAL;
    }
    else {
        printf("Invalid tap type specified.\n");
        return -EINVAL;
    }

    // Set up packet
    query.uid = uid;
    query.tap_value = tap_value;
    query.tap_type = tap_type;

    requestType.request_type = PA_SET_TAP_PARAMS;

    // Send request
    ret = streamsock_send(socket_fd, (uint8_t *)&requestType, sizeof(requestType),
        &num_transferred);
    if (ret || num_transferred != sizeof(requestType)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    ret = streamsock_send(socket_fd, (uint8_t *)&query, sizeof(query),
        &num_transferred);
    if (ret || num_transferred != sizeof(query)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    // Get response
    ret = streamsock_read(socket_fd, (uint8_t *)&response, sizeof(response), sizeof(response),
        &num_transferred);
    if (ret || num_transferred != sizeof(response)) {
        printf("Error receiving bytes: %d\n", ret);
        return ret;
    }

    // Handle response
    __handle_response(response.error);
    return 0;
}

int
handle_connect_to_uid_reserve(int socket_fd)
{
    int ret;
    uid_t uid = 0;
    char amount_str[22], *retstr;
    PAStatReserveQuery query;
    PAStatReserveResponse response;
    PARequest requestType;
    uint64_t num_transferred;

    printf("Stat reserve.\n");

    // Get UID
    memset(amount_str, 0, sizeof(amount_str));
    printf("Input UID: ");
    retstr = fgets(amount_str, sizeof(amount_str), stdin);
    if (!retstr) {
        printf("Error reading amount.\n");
        return -EINVAL;
    }
    uid = atol(amount_str);

    // set up packet
    query.uid = uid;
    requestType.request_type = PA_STAT_RESERVE;

    // Send request
    ret = streamsock_send(socket_fd, (uint8_t *)&requestType, sizeof(requestType),
        &num_transferred);
    if (ret || num_transferred != sizeof(requestType)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    ret = streamsock_send(socket_fd, (uint8_t *)&query, sizeof(query),
        &num_transferred);
    if (ret || num_transferred != sizeof(query)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    // Get response
    ret = streamsock_read(socket_fd, (uint8_t *)&response, sizeof(response), sizeof(response),
        &num_transferred);
    if (ret || num_transferred != sizeof(response)) {
        printf("Error receiving bytes: %d\n", ret);
        return ret;
    }

    __handle_response(response.error);
    if (response.error == PA_NO_ERROR) {
        printf("For UID %d: ", uid);
        if (response.flags & PA_USE_ROOT_RESERVE) {
            printf("Use Root Reserve.\n");
        }
        else if (response.flags & PA_RESERVE_GRANTED) {
            printf("Granted access to reserve with ID %d\n", response.rid);
        }
        else {
            printf("Error: bad flags: %d\n", response.flags);
            return -EINVAL;
        }
    }
    return 0;
}

int
disconnectFromServer(int socket_fd)
{
    int ret;
    uint64_t num_transferred;
    PARequest requestType;
    requestType.request_type = PA_GOODBYE;

    ret = streamsock_send(socket_fd, (uint8_t *)&requestType, sizeof(requestType),
        &num_transferred);
    if (ret || num_transferred != sizeof(requestType)) {
        printf("Error transmitting bytes: %d\n", ret);
        return ret;
    }

    return 0;
}
