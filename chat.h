#ifndef CHAT_H
# define CHAT_H

//includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h> //time info
#include <pthread.h> //threads
#include <signal.h> //signals

//macros
#define MAX_CLIENTS 5
#define MAX_USER 200
#define MAX_MSG 255
#define MSG_CAPACITY 1000

//Client Structure
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client_t;

#endif
