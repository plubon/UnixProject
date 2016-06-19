//
// Created by piotr on 18.06.16.
//

#ifndef UNIXPROJECT_UTILS_H
#define UNIXPROJECT_UTILS_H
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <aio.h>
#include <sys/stat.h>
#include <dirent.h>

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(0))

#define BACKLOG 3

typedef struct node {
	int val;
	struct node * next;
	char* username;
} listNode;

typedef struct _tempNode{
	char* path;
	pthread_mutex_t* mutex;
	struct _tempNode* next;
} mtxList;

typedef struct
{
    int socketFd;
	listNode* head;
	pthread_mutex_t* listMutex;
	mtxList* fileList;
	pthread_mutex_t* fileListMutex;
} thread_arg;



#endif //UNIXPROJECT_UTILS_H
