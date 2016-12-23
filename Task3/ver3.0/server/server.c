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
#include <stdint.h>

#include "header.h"

struct Connection {
  int connected;
  int ip;
  int socket;
  struct sockaddr_in addr;
  uint32_t size;
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

int create_listening_socket(unsigned short port);

int get_file_info(struct Connection* connection);

int get_number(int sock, uint32_t* number);


int main()
{
  int bytes = 0;
  int check = 0;
  int descriptorCounter = 0;
  int i, j;

  struct Connection connection[SOMAXCONN];
  struct pollfd sock_poll[SOMAXCONN];

  int listening_socket;

  /* Buffers */
  void *buf = NULL;

  fprintf(stderr, "Starting TCP server on any IP address with port %d\n", PORT);

  /* Allocate buffer for receiving data */
  buf = malloc((BUFFER_SIZE) + 1);
  if (buf == NULL) {
    fprintf(stderr, "Can not allocate buffer with size %d\n", BUFFER_SIZE);
    exit(EXIT_FAILURE);
  }

  for (i = 0;i < SOMAXCONN; ++i){
    init_connection(&connection[i]);
  }

  listening_socket = create_listening_socket(PORT);

  fprintf(stderr, "Server started\n");

  sock_poll[LISTENING_SOCKET].fd = listening_socket;
  sock_poll[LISTENING_SOCKET].events = POLLIN;

  j = 0;

  while(TRUE){
    check = poll(sock_poll, descriptorCounter + 1, POLL_WAIT_TIME);

    if (POLL_ERROR == check)
      return EXIT_FAILURE;

    if (check > 0){
      if (sock_poll[LISTENING_SOCKET].revents == POLLIN) {
       if (accept_connection(
            &connection[descriptorCounter],
            &sock_poll[descriptorCounter + 1],
            listening_socket) == EXIT_FAILURE)
          fprintf(stderr, "cannot accept connection\n");

        if (get_file_info(&(connection[descriptorCounter])) == EXIT_FAILURE){
          fprintf(stderr, "Connection %d has down\n", descriptorCounter);
          clear_connection(&connection[i], &sock_poll[i + 1]);
        }
        ++descriptorCounter;
        --check;
      }

      for (i = 0; i < descriptorCounter && j < check; ++i) {
        if (sock_poll[i + 1].revents == POLLIN) {
//            fprintf(stderr, "check for socket %d\n", sock_poll[i].fd);
          ++j;
          bytes = recv(sock_poll[i + 1].fd, buf, BUFFER_SIZE, 0);
          connection[i].received += bytes;
          write(connection[i].file, buf, bytes);
//          fprintf(stderr, "Recived: %d bytes\n", bytes);
          if (bytes == 0 || (connection[i].received == connection[i].size)){
            check_file_size(connection[i].file, connection[i].size, connection[i].socket);             
            close (connection[i].file);
//            printf("%d\n", bytes);
            clear_connection(&connection[i], &sock_poll[i + 1]);
            //fprintf(stderr, "%d\n", connection[i].connected);
            fprintf(stderr, "Connection %i has ended sending file\n", i);
          }
//          fprintf(stderr,"%s\n", (char*)buf);
        }
      }
      j = 0;
    }
  }

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

int create_listening_socket(unsigned short port){

  int sock;
  socklen_t addr_size = 0;
  struct sockaddr_in addr;

  addr_size = sizeof(struct sockaddr_in);

  /* Create socket file descriptor for INET protocols and TCP (SOCK_STREAM) protocol */
  sock = socket(PF_INET, SOCK_STREAM, 0);

  /* Fill server address structure: INET family, port (in network byte order) and binding address (any on this machine) */
  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);


  int ok = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  /* Bind socket to specified addresses and port */
  if (0 > bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
    perror("bind");         /* See man perror for details */
    exit(EXIT_FAILURE);
  }

  /* Start listening */
  if (0 > listen(sock, SOMAXCONN)) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return sock;
}

int get_file_info(struct Connection* connection){
  int readed_bytes = 0;
  uint32_t file_name_lentgh = 0;
  char* file_name;

  if(get_number(connection->socket, &file_name_lentgh) == EXIT_FAILURE)
    return EXIT_FAILURE;

  fprintf(stderr, "%d\n", file_name_lentgh);


  if((file_name = (char*)malloc(connection->size)) == NULL){
    fprintf(stderr, "get_file_info: cannot allocate memory\n");
    return EXIT_FAILURE;
  }

  while (readed_bytes < file_name_lentgh){
    readed_bytes += recv(connection->socket, file_name + readed_bytes, file_name_lentgh - readed_bytes, 0);
    if (readed_bytes == 0){
      fprintf(stderr, "Lost connection\n");
      return EXIT_FAILURE;
    }
  }

  fprintf(stderr, "NAME OF FILE %s\n", file_name);

  connection->file = open(file_name,O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

  if(get_number(connection->socket, &(connection->size)) == EXIT_FAILURE)
    return EXIT_FAILURE;

  fprintf(stderr, "SIZE: %ld\n", connection->size);

  fprintf(stderr, "DESCRIPTOR %d\n", connection->file);

  return EXIT_SUCCESS;
}

int get_number(int sock, uint32_t* number){
  char* buf;
  short readed_bytes = 0;
  uint32_t inter;

  if((buf = (char*)malloc(sizeof(uint32_t))) == NULL){
    fprintf(stderr, "cannot allocate memory\n");
    return EXIT_FAILURE;
  }

  while(readed_bytes < sizeof(uint32_t)){
    readed_bytes += recv(sock, buf + readed_bytes, sizeof(uint32_t) - readed_bytes, 0);
    if (readed_bytes == 0){
      fprintf(stderr, "connection losted\n");
      return EXIT_FAILURE;
    }
  }

  inter = *((uint32_t*)buf);

  fprintf(stderr, "BEFORE 0x%x %d\n", inter, inter);

  *number = ntohl(inter);

  fprintf(stderr, "AFTER 0x%x %d\n", (*number), (*number));

  // *number = (uint32_t)(*buf);

  return EXIT_SUCCESS;
}