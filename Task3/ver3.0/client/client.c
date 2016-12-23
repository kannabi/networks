#include "header.h"


#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

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
#include <string.h>
#include <stdint.h>

#include <netinet/in.h>
#include <arpa/inet.h>


int sig_sock;
int sig_handler(int signal){
    close(sig_sock);
    exit(0);
}


int send_file(char* path_to_file, char* ip, int port);

char* get_file_name(char* path_to_file, uint32_t* length);


int main(int argc, char* argv[]){
    char* usage = "usage:\n./transfer [[-f <path to file>] or [-d <path to directory>]] [-h ip] [-p port]\n";
    char* path_to_file;
    char* ip;
    int port;

    path_to_file = (char*)malloc(sizeof(char) * 4095);
    ip = (char *)malloc(sizeof(char) * 16);

    if (argc < 7){
        fprintf(stdout, "Incorrect arguments\n%s", usage);
        return EXIT_FAILURE;
    }

    //  for (int i = 0; i < argc; ++i)
    //    fprintf(stdout, "%s\n", argv[i]);

//    fprintf(stdout, "%d %d\n", strcmp(argv[1], "-f"), strcmp(argv[1], "-d"));
    if (strcmp(argv[1], "-f") != 0 && strcmp(argv[1], "-d") != 0){
        fprintf(stdout, "First flag is incorrect\n%s", usage);
        return EXIT_FAILURE;
    }

    if (argv[1][1] == 'f')
        strcpy(path_to_file, argv[2]);
    //    else
    //      continue; 

    if (strcmp(argv[3], "-h") == 0)
        strcpy(ip, argv[4]);
    else{
        fprintf(stdout, "Second flag is incorrect\n%s", usage);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[5], "-p") == 0)
        port = atoi(argv[6]);
    else{
        fprintf(stdout, "Third flag is incorrect\n%s", usage);
        return EXIT_FAILURE;
    }

//    fprintf(stderr, "before send\n");

    send_file(path_to_file, ip, port);
    free(path_to_file);
    free(ip);

    return EXIT_FAILURE;
}

int send_file(char* path_to_file, char* ip, int port)
{
    /* Destination host information */
    char *address = NULL;

    int opt = 0;
    int ans;

    /* Socket file descriptor and address sturctures */
    int socket_fd = 0;
    struct sockaddr_in addr;
    struct hostent *host_info = NULL;

    /* Buffer */
    void *buf = NULL;
    char *message = NULL;
    int sending_file;
    int readed_bytes;
    int sended_bytes;
    uint32_t file_size;
    uint32_t name_length;

    struct stat st;

    /* And fill address stucture */
    bzero(&addr, sizeof(struct sockaddr_in)); /* Don't forget to fill it with zeros */

    /* Fill host information. Pay attention to h_addr field. It represents first ip address
     * for host (h_addr_list[0]) and uses for backward compatibility.
     * It's better to use random address from addresses list */
    addr.sin_family = PF_INET;
    addr.sin_port = htons(port);
    inet_aton(ip,&addr.sin_addr);

    /* Create socket file description for INET protocol family and TCP protocol */
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);

//    fprintf(stderr, "%s\n", path_to_file);

    sending_file = open(path_to_file, O_RDONLY);
    char* file_name = get_file_name(path_to_file, &name_length);
    fstat(sending_file, &st);


    /* Connect to server */
    if (0 > connect(socket_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
        perror("connect");
        close(socket_fd);
        free(buf);
        free(address);
        exit(EXIT_FAILURE);
    }

    /* Message promt */
    buf = malloc(BUFFER_SIZE + 1);
    if (buf == NULL){
        fprintf(stderr, "message");
        return EXIT_FAILURE;
    }

    // fprintf(stderr, "BEFORE 0x%x %d\n", name_length, name_length);
    name_length = htonl(name_length);
    // fprintf(stderr, "AFTER 0x%x %d\n", name_length, name_length);

    send(socket_fd, &name_length, sizeof(name_length), 0);
    send(socket_fd, file_name, strlen(file_name), 0);
    free(file_name);

    fprintf(stderr, "SIZE %ld\n", st.st_size);

    file_size = htonl(st.st_size);
    send(socket_fd, &file_size, sizeof(uint32_t), 0);

    while ((readed_bytes = read(sending_file, buf, BUFFER_SIZE + 1)) != 0){
        sended_bytes = send(socket_fd, buf, readed_bytes, 0);
        if(sended_bytes == SEND_FAILURE){
            perror("send readed bytes to server:");
            return EXIT_FAILURE;
        }
        while (sended_bytes < readed_bytes){
            sended_bytes += send(socket_fd, buf + sended_bytes, readed_bytes - sended_bytes, 0);
        }
    }


    recv(socket_fd, &ans, sizeof(int), 0);
    if (ans)
        fprintf (stdout, "%d\nFile has been transfered.\n", ans);
    else
        fprintf (stdout, "%d\nFile hasn't been transfered.\n", ans);


    /* Don't forget to free resources */
    close(sending_file);
    close(socket_fd);
    free(buf);
    free(address);

    return EXIT_SUCCESS;
}


char* get_file_name(char* path_to_file, uint32_t* length){
    int i = strlen(path_to_file);
    long int max_name_length = pathconf("/",_PC_NAME_MAX);
    char *file_name;
    int j = 0;

//    fprintf(stderr, "PATH_TO_FILE %s\n", path_to_file);

    if ((file_name = (char *)malloc(max_name_length - 1)) == NULL){
        perror("get_file_name");
        fprintf(stderr, "Cannot allocate memory\n");
        return NULL;
    }

    for (i = strlen(path_to_file) - 1; 
            i >= 0 && 
            path_to_file[i] != '/' &&
            j < max_name_length;
            --i){
//        fprintf(stderr, "I %d %d\n", i, j);
        //    fprintf(stderr,"%c\n", path_to_file[i]);
        //    file_name[j] = path_to_file[i];
        ++j;
    }

//    fprintf(stderr, "I %d %d\n", i, j);
    *length = (uint32_t)(j);
    for (int k = 0; k < j; ++k){
        ++i;
        file_name[k] = path_to_file[i];
//        fprintf(stderr,"%c\n", path_to_file[i]);
    }
//    fprintf(stderr, "FN %s\n", file_name);

    return file_name;
}
