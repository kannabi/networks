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


#include <netinet/in.h>
#include <arpa/inet.h>


#define NULL 0
#define BUFFER_SIZE (1024)
#define TIME_OF_EXPECT 5000
#define PORT 1488
#define POLL_WAIT_TIME 6
#define POLL_ERROR -1
#define POLL_TIMEOUT 0

int *socket_pointer = NULL; //It is a pointer which we use for free port


void sig_handler(int signal){
    close (*socket_pointer);
    exit(1);
}

int print_info (int cur, int prev){
    if (cur < prev)
        for (int i = 0; i < (prev - cur); ++i)
            printf ("One instance went down. Total: %d\n", cur + i + 1);
    else
        for (int i = 0; i < (cur - prev); ++i)
            printf ("New instance available. Total: %d\n", prev + i + 1);
}

int hi_there (int socket_fd, struct sockaddr_in *b_addr,
              struct pollfd *sock_poll, struct sockaddr_in *addr, socklen_t addr_size) {

    int innerCount = 0;
    double t0 = 0, t1 = 0;
    int i = 0;
    int check = 0;

    uint32_t source_ip = 0;
    unsigned short source_port = 0;

    void *buf = NULL;
    char *message = NULL;
    int size = 0;

    /* Allocate buffer for receiving datagrams */
    buf = malloc(BUFFER_SIZE + 1);
    if (buf == NULL) {
        fprintf(stderr, "Can not allocate buffer with size %d\n", BUFFER_SIZE);
        return EXIT_FAILURE;
    }

//    printf ("HI_THERE\n");

    sendto(socket_fd, "hello", strlen("hello") + 1, 0,
           (struct sockaddr *) b_addr, sizeof(struct sockaddr_in));

//    printf ("SEND HELLO\n");

    while (1){
        check = poll(sock_poll, 1, 500);
        switch (check) {
            case POLL_TIMEOUT : {
                free (buf);
                return innerCount;
            }
            case POLL_ERROR : {
                perror("poll");
            }
            default: {
                ++i;
                size = recvfrom(socket_fd, buf, BUFFER_SIZE, 0, (struct sockaddr *) addr, &addr_size);
                if (size == -1) {
                    perror("recvfrom");
                    continue;
                }
                message = (char *) buf;
                message[size] = 0;

                /* Source address information (client's address and port) are stored in s_addr and size of structure is stored in s_addr_size */
                if (addr->sin_family != PF_INET) {
                    fprintf(stderr,
                            "Datagramm with non-inet protocol family received\n"); /* Paranoid check. Just for example */
                    continue;
                }
                source_ip = ntohl(addr->sin_addr.s_addr); /* Don't forget to convert byte order */
                source_port = ntohs(addr->sin_port); /* and port value too. htonl - for uint32, htons - for uint16 */

                ++innerCount;
            }
        }
    }
}


int main(int argc, char *argv[])
{
    unsigned short port = 0;


    /* File descriptor for socket, address structures for server and client, etc */
    int socket_fd = 0;
    struct sockaddr_in addr;
    struct sockaddr_in s_addr;
    struct sockaddr_in b_addr;
    socklen_t addr_size = 0;


    int bcastPermission = 1;
    int subNum = 1;  //Number of net's subscribers
    int innerCount = 0; // Number of hello-packet which we had got in this period

    struct pollfd sock_poll;

    double t0 = 0, t1 = 0;
    int check = 0;


    port = PORT;

    fprintf(stderr, "Starting UDP app on any IP address with port %d\n", port);

    /* We must fill addr structures by zeros */
    bzero(&addr  , sizeof(struct sockaddr_in));
    bzero(&s_addr, sizeof(struct sockaddr_in));
    addr_size = sizeof(struct sockaddr_in);

    /* Create socket file descriptor for INET protocols and UDP (SOCK_DGRAM) protocol */
    socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
    socket_pointer = &socket_fd;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, (void *) &bcastPermission, sizeof(bcastPermission)) < 0){
        perror("cannot apply the broadcast for socket");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &bcastPermission, sizeof(bcastPermission));

    /* Fill server address structure: INET family, port (in network byte order) and binding address (any on this machine) */
    s_addr.sin_family = PF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    b_addr.sin_family = PF_INET;
    b_addr.sin_port = htons(port);
    b_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    sock_poll.fd = socket_fd;
    sock_poll.events = POLLIN;

    /* Bind socket to specified addresses and port */
    if (0 > bind(socket_fd, (struct sockaddr *) &s_addr, sizeof(struct sockaddr_in))) {
        perror("bind");         /* See man perror for details */
        exit(EXIT_FAILURE);
    }

    sigset(SIGINT, sig_handler);

    fprintf(stderr, "App has started\n");

    subNum = hi_there(socket_fd, &b_addr ,&sock_poll, &addr, addr_size);

    printf ("Total: %d\n", subNum);

    while (1){
        check = poll(&sock_poll, 1, POLL_WAIT_TIME);

        if (POLL_ERROR == check){
            perror("poll");
            return EXIT_FAILURE;
        }
        else{
            innerCount = hi_there(socket_fd, &b_addr, &sock_poll, &addr, addr_size);
            print_info(innerCount, subNum);
            subNum = innerCount;
            innerCount = 0;
        }
    }

    /* We will never execute this code, but it's good style to use free for each malloc */
    close(socket_fd);
    //free(buf);

    return EXIT_SUCCESS;
}
