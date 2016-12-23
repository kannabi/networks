//
// Created by kannabi on 09.09.16.
//


#include "header.h"

int server()
{
    unsigned short port = 0;
    int opt = 0;
    int check = 0;
    double numberOfCame = 0;
    double speed, avgSpeed = 0;
    int speedCounter = 0;

    struct pollfd sock_poll;
    struct timespec t0,t1;

    /* File descriptor for socket, address structures for server and client, etc */
    int socket_fd = 0;
    int csocket_fd = 0;
    struct sockaddr_in addr;
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
    buf = malloc(BUFFER_SIZE + 1);
    if (buf == NULL) {
        fprintf(stderr, "Can not allocate buffer with size %d\n", BUFFER_SIZE);
        exit(EXIT_FAILURE);
    }

    /* We must fill address structures by zeros */
    bzero(&  addr, sizeof(struct sockaddr_in));
    bzero(&s_addr, sizeof(struct sockaddr_in));
    addr_size = sizeof(struct sockaddr_in);

    /* Create socket file descriptor for INET protocols and TCP (SOCK_STREAM) protocol */
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    /* Fill server address structure: INET family, port (in network byte order) and binding address (any on this machine) */
    s_addr.sin_family = PF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

//    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
//        perror("setsockopt(SO_REUSEADDR) failed");

    /* Bind socket to specified addresses and port */
    if (0 > bind(socket_fd, (struct sockaddr *) &s_addr, sizeof(struct sockaddr_in))) {
        perror("bind");         /* See man perror for details */
        exit(EXIT_FAILURE);
    }

    /* Start listening */
    if (0 > listen(socket_fd, SOMAXCONN)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Server started\n");

    /* OK, now we will accept client connections */

    csocket_fd = accept(socket_fd, (struct sockaddr *) &addr, &addr_size);
    if (csocket_fd < 0) {
        perror("accept");
//        continue;
    }

    /* Source address information (client's address and port) are stored in addr and size of structure is stored in addr_size */
    if (addr.sin_family != PF_INET) {
        fprintf(stderr, "Connection with non-inet protocol family\n"); /* Paranoid check. Just for example */
//        continue;
    }
    source_ip   = ntohl(addr.sin_addr.s_addr); /* Don't forget to convert byte order */
    source_port = ntohs(addr.sin_port); /* and port value too. htonl - for uint32, htons - for uint16 */

    printf("New connection from %d.%d.%d.%d:%d\n",      /* Print source address information */
           (source_ip & 0xff000000) >> 24,
           (source_ip & 0x00ff0000) >> 16,
           (source_ip & 0x0000ff00) >> 8,
           (source_ip & 0x000000ff),
           source_port);

    sock_poll.fd = csocket_fd;
    sock_poll.events = POLLIN;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0);

    while (1) {  
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
        if ((t1.tv_sec - t0.tv_sec) > ONE_SEC){
           speed = numberOfCame / 1024;
           avgSpeed = (avgSpeed * speedCounter + speed) / 
                        (speedCounter + 1);
           printf ("Speed: %lf kb/s, Avg: %lf kb/s\n", speed, avgSpeed);
//           speedCounter < INT_MAX ? ++speedCounter : speedCounter = 0;
           if (speedCounter < INT_MAX)
               ++speedCounter;
               else
                    speedCounter = 0;
                              
           numberOfCame = 0;
           t0 = t1;
        }

        size = recv(csocket_fd, buf, BUFFER_SIZE, 0);
        if (size < 0) {
            perror("recv");
            close(csocket_fd);
        }
        numberOfCame += size;

        message = (char *) buf;
        message[size] = 0;

    //    printf("Message: %s\n", message);      /* Print message */

        /* Close client connection */
        //close(csocket_fd);
    }

    /* We will never execute this code, but it's good style to use free for each malloc */
    close(socket_fd);
    free(buf);

    return EXIT_SUCCESS;
}
