//
// Created by kannabi on 30.09.16.
//

#ifndef TASK2_HEADER_H
#define TASK2_HEADER_H

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

#define POLL_WAIT_TIME 5
#define POLL_ERROR -1
#define PORT 3015
#define BUFFER_SIZE (1024)
#define ONE_SEC 1


#define MACHINE_IP_0 "10.4.10.70"
#define MACHINE_IP_1 "10.4.10.71"
#define MACHINE_IP_2 "10.4.10.72"
#define MACHINE_IP_3 "10.4.10.73"
#define MACHINE_ON_DA_SAME "127.0.0.1"

int client ();
int server ();

#endif //TASK2_HEADER_H
