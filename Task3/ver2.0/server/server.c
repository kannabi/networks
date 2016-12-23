//
// Created by kannabi on 09.09.16.
//

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
  int ip;
  int socket;
  struct sockaddr_in addr;
  int long size;
  int long received;
//  struct stat file_info;
  int file;
};

int accept_connection (struct Connection* connection,
    struct pollfd* sock_poll,
    int listeningSocket);

int print_connect_info (struct sockaddr_in addr);

int clear_connection (struct Connection* connection, struct pollfd* sock_poll);

int init_connection (struct Connection* connection);

int check_file_size(int fd, long int size, int socket);


int main()
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
  void *name_of_file = NULL;
  char *message = NULL;
  int size = 0;

  port = PORT;

  fprintf(stderr, "Starting TCP server on any IP address with port %d\n", port);

  /* Allocate buffer for receiving data */
  buf = malloc((BUFFER_SIZE) + 1);
  if (buf == NULL) {
    fprintf(stderr, "Can not allocate buffer with size %d\n", BUFFER_SIZE);
    exit(EXIT_FAILURE);
  }
  name_of_file = malloc(BUFFER_SIZE);
  if(name_of_file == NULL){
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

  sock_poll[LISTENING_SOCKET].fd = connection[LISTENING_SOCKET].socket;
  sock_poll[LISTENING_SOCKET].events = POLLIN;

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

        recv(connection[descriptorCounter].socket, name_of_file, BUFFER_SIZE, 0);
        fprintf(stderr, "NAME OF FILE %s\n", (char*)name_of_file);

        recv(connection[descriptorCounter].socket, &connection[descriptorCounter].size, sizeof(long int), 0);

        fprintf(stderr, "size: %ld\n", connection[descriptorCounter].size);

        connection[descriptorCounter].file = open(name_of_file,O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
        fprintf(stderr, "DESCRIPTOR %d\n", connection[descriptorCounter].file);

        bzero(name_of_file, BUFFER_SIZE);
        --check;
      }

      for (i = 1; i < descriptorCounter + 1 && j < check; ++i) {
        if (sock_poll[i].revents == POLLIN) {
//            fprintf(stderr, "check for socket %d\n", sock_poll[i].fd);
          ++j;
          bytes = recv(sock_poll[i].fd, buf, BUFFER_SIZE, 0);
          connection[i].received += bytes;
          write(connection[i].file, buf, bytes);
//          fprintf(stderr, "Recived: %d bytes\n", bytes);
          if (bytes == 0 || (connection[i].received == connection[i].size)){
            check_file_size(connection[i].file, connection[i].size, connection[i].socket); 
            
            close (connection[i].file);
//            printf("%d\n", bytes);
            clear_connection(&connection[i], &sock_poll[i]);
            //fprintf(stderr, "%d\n", connection[i].connected);
            fprintf(stderr, "Connection %i has ended sending file\n", i);
          }
//          fprintf(stderr,"%s\n", (char*)buf);
          //write(connection[i].file, buf, bytes);
        }
      }
      j = 0;
    }
  }
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
  connection->ip = connection->socket = connection->connected = 0;
  connection->size = connection->received = 0;
  bzero(&connection->addr, sizeof(struct sockaddr_in));
}


int check_file_size(int fd, long int size, int socket){
    struct stat st;
    fstat(fd, &st);
    int ans;
    
    fprintf(stderr, "%ld\n", st.st_size);

    if (size == st.st_size){
        ans = TRUE;
        send (socket, &ans, sizeof(int), 0);
    } else {
        ans = FALSE;
        send (socket, &ans, sizeof(int), 0);
    }

    return EXIT_SUCCESS;
}
