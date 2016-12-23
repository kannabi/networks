#include "header.h"

int client()
{
    /* Destination host information */
    unsigned short port = 0;
    char *address = NULL;

    int opt = 0;

    /* Socket file descriptor and address sturctures */
    int socket_fd = 0;
    struct sockaddr_in addr;
    struct hostent *host_info = NULL;
    char **iterator = 0;

    /* Buffer */
    void *buf = NULL;
    char *message = NULL;

    port = PORT;

    /* Allocate buffer */
    buf = malloc(BUFFER_SIZE + 1);
    if (buf == NULL) {
        fprintf(stderr, "Can not allocate buffer with size %d\n", BUFFER_SIZE);
        exit(EXIT_FAILURE);
    }

    /* And fill address stucture */
    bzero(&addr, sizeof(struct sockaddr_in)); /* Don't forget to fill it with zeros */

    /* Fill host information. Pay attention to h_addr field. It represents first ip address
     * for host (h_addr_list[0]) and uses for backward compatibility.
     * It's better to use random address from addresses list */
    addr.sin_family = PF_INET;
    addr.sin_port = htons(port);
    inet_aton(MACHINE_ON_DA_SAME,&addr.sin_addr);

    /* Create socket file description for INET protocol family and TCP protocol */
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    /* Connect to server */
    if (0 > connect(socket_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
        perror("connect");
        close(socket_fd);
        free(buf);
        free(address);
        exit(EXIT_FAILURE);
    }

    /* Message promt */
    message = (char *) buf;
//    bzero(&buf, BUFFER_SIZE + 1);

    while (1){
        send(socket_fd, buf, strlen(message) + 1, 0);
    }

    /* To stop input just send EOF symbol (^D in terminal) */
    //fprintf(stderr, "EOF\n");

    /* Don't forget to free resources */
    close(socket_fd);
    free(buf);
    free(address);

    return EXIT_SUCCESS;
}
