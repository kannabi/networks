//
// Created by kannabi on 09.09.16.
//

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "header.h"

struct Connection {
    int connected;
    double numberOfCame, avgSpeed;
    int ip;
    int socket;
    struct sockaddr_in addr;
    int speedCounter;
};

int accept_connection (struct Connection* connection,
        struct pollfd* sock_poll,
        int listeningSocket);

int print_speed (clock_t t0, clock_t t1, struct Connection * connection, int connectionNumber);

int print_connect_info (struct sockaddr_in addr);

int clear_connection (struct Connection* connection, struct pollfd* sock_poll);

int init_connection (struct Connection* connection);

int server()
{
    int bytes = 0;
    unsigned short port = 0;
    int opt = 0;
    int check = 0;
    int descriptorCounter = 0;
    int i, j, checkConnect;
    double time = 0;

    struct Connection connection[SOMAXCONN];
    struct pollfd sock_poll[SOMAXCONN];
    clock_t t0,t1;

    struct sockaddr_in s_addr;
    socklen_t addr_size = 0;
    uint32_t source_ip = 0;
    unsigned short source_port = 0;

    /* Buffers */
    void *buf = NULL;
    char *message = NULL;
    int size = 0;

    port = PORT;

    fprintf(stderr, "Starting TCP server on any IP address with port %d\n", port);

    /* Allocate buffer for receiving data */
    buf = malloc((BUFFER_SIZE*4) + 1);
    if (buf == NULL) {
        fprintf(stderr, "Can not allocate buffer with size %d\n", BUFFER_SIZE);
        exit(EXIT_FAILURE);
    }

    for (i = 0;i < SOMAXCONN; ++i){
        init_connection(&connection[i]);
    }

    addr_size = sizeof(struct sockaddr_in);

    /* Create socket file descriptor for INET protocols and TCP (SOCK_STREAM) protocol */
    connection[LISTENING_SOCKET].socket = socket(PF_INET, SOCK_STREAM, 0);

    /* Fill server address structure: INET family, port (in network byte order) and binding address (any on this machine) */
    connection[LISTENING_SOCKET].addr.sin_family = PF_INET;
    connection[LISTENING_SOCKET].addr.sin_port = htons(port);
    connection[LISTENING_SOCKET].addr.sin_addr.s_addr = htonl(INADDR_ANY);


    int ok = 1;
    if (setsockopt(connection[LISTENING_SOCKET].socket, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    /* Bind socket to specified addresses and port */
    if (0 > bind(connection[LISTENING_SOCKET].socket, (struct sockaddr *) &connection[LISTENING_SOCKET].addr, sizeof(struct sockaddr_in))) {
        perror("bind");         /* See man perror for details */
        exit(EXIT_FAILURE);
    }

    /* Start listening */
    if (0 > listen(connection[LISTENING_SOCKET].socket, SOMAXCONN)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Server started\n");

    /* OK, now we will accept client connections */
    ++descriptorCounter;

    accept_connection(&connection[descriptorCounter],
            &sock_poll[descriptorCounter],
            connection[LISTENING_SOCKET].socket);

    sock_poll[LISTENING_SOCKET].fd = connection[LISTENING_SOCKET].socket;
    sock_poll[LISTENING_SOCKET].events = POLLIN;

    t0 = clock();
    j = 0;

    while(TRUE){
        check = poll(sock_poll, descriptorCounter + 1, POLL_WAIT_TIME);

        if (POLL_ERROR == check)
            return EXIT_FAILURE;

        if (check > 0){
            if (sock_poll[LISTENING_SOCKET].revents == POLLIN) {
                ++descriptorCounter;
                accept_connection(
                        &connection[descriptorCounter],
                        &sock_poll[descriptorCounter],
                        connection[LISTENING_SOCKET].socket);
                --check;
            }

            for (i = 1; i < descriptorCounter + 1 && j < check; ++i) {
                if (sock_poll[i].revents == POLLIN) {
                    //										fprintf(stderr, "check for socket %d\n", sock_poll[i].fd);
                    ++j;
                    bytes = recv(sock_poll[i].fd, buf, BUFFER_SIZE, 0);
                    //										fprintf(stderr, "Recived: %d bytes\n", bytes);
                    if (bytes == 0){
                        printf("%d\n", bytes);
                        clear_connection(&connection[i], &sock_poll[i]);
                        //												fprintf(stderr, "%d\n", connection[i].connected);
                        fprintf(stderr, "Connection %i has losted\n", i);
                    }
                    connection[i].numberOfCame += bytes;
                }
            }
            j = 0;
        }
        t1 = clock();
        time = (double)(t1 - t0)/CLOCKS_PER_SEC;
        if (time > ONE_SEC) {
            for (i = 1; i < descriptorCounter + 1; ++i)
                if (connection[i].connected == TRUE){
                    print_speed(t0, t1, &connection[i], i);
                    connection[i].numberOfCame = 0;
                }
            time = 0;
            t0 = t1;
            bzero (buf, BUFFER_SIZE + 1);
        }
    }
}


int print_speed (clock_t t0, clock_t t1, struct Connection * connection, int connectionNumber){
    printf("inPS\n");
    int speedCounter = connection->speedCounter;
    double speed = 0;

    speed = connection->numberOfCame * 8 / (1024 * 1024);
    connection->avgSpeed = (connection->avgSpeed * speedCounter + speed) /
        (speedCounter + 1);
    printf ("Connection: № %d speed: %lf Mb/s, Avg: %lf Mb/s\n", connectionNumber, speed, connection->avgSpeed);

    //        (speedCounter < INT_MAX) ? ++speedCounter : speedCounter = 0;
    if (speedCounter < INT_MAX)
        ++speedCounter;
    else
        speedCounter = 0;

    return EXIT_SUCCESS;
}

int print_connect_info (struct sockaddr_in addr){
    /* Source address information (client's address and port) are stored in addr and size of structure is stored in addr_size */
    if (addr.sin_family != PF_INET) {
        fprintf(stderr, "Connection with non-inet protocol family\n"); /* Paranoid check. Just for example */
        return EXIT_FAILURE;
    }
    int source_ip   = ntohl(addr.sin_addr.s_addr); /* Don't forget to convert byte order */
    int source_port = ntohs(addr.sin_port); /* and port value too. htonl - for uint32, htons - for uint16 */

    printf("New connection from %d.%d.%d.%d:%d\n",      /* Print source address information */
            (source_ip & 0xff000000) >> 24,
            (source_ip & 0x00ff0000) >> 16,
            (source_ip & 0x0000ff00) >> 8,
            (source_ip & 0x000000ff),
            source_port);

    return EXIT_SUCCESS;
}

int accept_connection (struct Connection* connection,
        struct pollfd* sock_poll, 
        int listeningSocket){
    int addr_size = sizeof(struct sockaddr_in);

    connection->socket = accept(listeningSocket,
            (struct sockaddr*)&connection->addr,
            &addr_size);

    if (connection->socket < 0){
        perror("accept() in accept_connection()");
        return EXIT_FAILURE;
    }

    connection->connected = TRUE;

    sock_poll->fd = connection->socket;
    sock_poll->events = POLLIN;

    print_connect_info(connection->addr);

    return EXIT_SUCCESS;
}

int clear_connection (struct Connection* connection, struct pollfd* sock_poll){
    init_connection(connection);
    sock_poll->fd = sock_poll->events = sock_poll->revents = 0;
}

int init_connection (struct Connection* connection){
    connection->numberOfCame = connection->avgSpeed = 0;
    connection->ip = connection->socket =
        connection->speedCounter = connection->connected = 0;
    bzero(&connection->addr, sizeof(struct sockaddr_in));
}
