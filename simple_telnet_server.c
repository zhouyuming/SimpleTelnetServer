#include <stdio.h>
#include <sys/socket.h>

#include "simple_telnet_server.h"

int main(int argc, char *argv[])
{
    int ret;
    int max_sd;
    fd_set readfds;
    int opt = TRUE;
    int master_socket;
    struct sockaddr_in address;

    // 1、create a master socket
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket == 0) {
        printf("master socket create failed!\n");
        exit(EXIT_FAILURE);
    }

    // 2、set master socket to allow multiple connections
    ret = setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if (ret < 0) {
        printf("setsockopt failed!");
        exit(EXIT_FAILURE);
    }

    // 3、type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(LISTEN_IP);
    address.sin_port = htons(PORT);

    // 4、bind the socket
    ret = bind(master_socket, (struct sockaddr *)&address, sizeof(address));
    if (ret < 0) {
        printf("bind failed!");
        exit(EXIT_FAILURE);
    }
    printf("Listener at %s on port %d \n", LISTEN_IP, PORT);

    // 5、try to specify maximum of 3 pending connections for the master socket
    ret = listen(master_socket, 3);
    if (ret < 0) {
        printf("listen failed!\n");
        exit(EXIT_FAILURE);
    }

    while (TRUE) {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(master_socket, &readfds);

    }
    return 0;
}
