#include "util.h"

static int 
__connect_to_arbiter(int *sock)
{
    int socket_fd, err;
    struct sockaddr_un address;
    socklen_t address_length;
    char buffer[256];

    socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0) {
      err = errno;
      perror("socket");
      return err;
    }
 
    address.sun_family = AF_UNIX;
    address_length = sizeof(address.sun_family) +
                  sprintf(address.sun_path, POWER_ARBITER_SOCKPATH);

    if(connect(socket_fd, (struct sockaddr *) &address, address_length) != 0) {
        err = errno;
        perror("connect");
        return err;
    }

    *sock = socket_fd;    
    return 0;
}

static void
__close_client(int socket_fd)
{
    close(socket_fd);
}

int main(void)
{
    int  err, socket_fd, nbytes;
    char input[INPUT_SIZE];
    char *retstr;

    err = __connect_to_arbiter(&socket_fd);
    if (err) {
        printf("Client quit.\n");
        exit(1);
    }

    printf("Accepting commands (type 'help' for more info)\n");
    while(1) {
        retstr = fgets(input, sizeof(input), stdin);
        if (!retstr) {
            if (feof(stdin)) {
                printf("Got EOF, quitting...\n");
                break;
            }
            else if (ferror(stdin)) {
                printf("Error reading input, quitting...\n");
                break;
            }
        }
        chomp(input);

        err = 0;
        if (strcmp(input, "add") == 0) {
            err = handle_add_reserve_mapping(socket_fd);
        }
        else if (strcmp(input, "del") == 0) {
            err = handle_del_reserve_mapping(socket_fd);
        }
        else if (strcmp(input, "get") == 0) {
            err = handle_get_reserve_mapping_data(socket_fd);
        }
        else if (strcmp(input, "set") == 0) {
            err = handle_set_reserve_mapping_data(socket_fd);
        }
        else if (strcmp(input, "connect") == 0) {
            err = handle_connect_to_uid_reserve(socket_fd);
        }
        else if (strcmp(input, "list") == 0) {
            handle_list_reserves();
        }
        else if (strcmp(input, "put") == 0) {
            handle_put_reserve();
        }
        else if (strcmp(input, "info") == 0) {
            handle_info_reserve();
        }
        else if (strcmp(input, "help") == 0) {
            handle_help();
        }
        if (err) {
            printf("Error received while communicating with arbiter. Disconnecting.\n");
            disconnectFromServer(socket_fd);
            break;
        }
    }

    __close_client(socket_fd);
    return 0;
}

int handle_help()
{
    printf("Usage: \n");

    printf("add: create uid to reserve mapping\n");
    printf("del: remove uid to reserve mapping\n");
    printf("get: get uid to reserve mapping data\n");
    printf("set: set uid to reserve mapping data\n");
    printf("connect: get connection to reserve for uid\n");
    printf("list: list reserves we are connected to\n");
    printf("put: drop link to reserve we are connected to\n");
    printf("info: print info for specified reserve\n");
    printf("help: display help\n");

    printf("\n\n");
    return 0;
}
