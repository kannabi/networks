#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "header.h"

//There will be socket of server for cleaning
int sig_sock;
int sig_handler(int signal);
int client();


int main(int argc, char* argv[]){

    if (argc < 7) {
        fprintf(stderr, "usage: ./transfer [-f file] [-d directory]\n[-h ip address] [-p port]");
        return EXIT_FAILURE;
    }



    return EXIT_SUCCESS;
}



int sig_handler(int signal){
    close(sig_sock);
    exit(NULL);
}

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

    /* And fill address stucture */
    bzero(&addr, sizeof(struct sockaddr_in)); /* Don't forget to fill it with zeros */

    /* Fill host information. Pay attention to h_addr field. It represents first ip address
     * for host (h_addr_list[0]) and uses for backward compatibility.
     * It's better to use random address from addresses list */
    addr.sin_family = PF_INET;
    addr.sin_port = htons(port);
    inet_aton(MACHINE_ON_DA_SAME,&addr.sin_addr);
//    inet_aton(MACHINE_IP_0, &addr.sin_addr);
	

    /* Create socket file description for INET protocol family and TCP protocol */
    sig_sock = socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    if(SIG_ERR == sigset(SIGINT, sig_handler)){
        perror("Cannot change signal reaction");
        return EXIT_FAILURE;
    }

    /* Connect to server */
    if (0 > connect(socket_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
        perror("connect");
        close(socket_fd);
        free(buf);
        free(address);
        exit(EXIT_FAILURE);
    }

    /* Message promt */
//    message = (char *) buf;
//    bzero(&buf, BUFFER_SIZE + 1);
    message = malloc(BUFFER_SIZE + 1);
    if (message == NULL){
        printf("message");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < BUFFER_SIZE + 1; ++i)
        message[i] = '1';

    while (1){
        //printf("in while");
        send(socket_fd, message, strlen(message) + 1, 0);
        //sleep(1);
    }

    /* To stop input just send EOF symbol (^D in terminal) */
    //fprintf(stderr, "EOF\n");

    /* Don't forget to free resources */
    close(socket_fd);
    free(buf);
    free(address);

    return EXIT_SUCCESS;
}
