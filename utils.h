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
/*
 * list of all logged in users and fd to sockets that they connected to
 * used for sending real time notifications
 */
typedef struct node {
	int val;
	struct node * next;
	char* username;
} listNode;

/*
 * list of mutexes locking access to files
 * when thread tries to lock file it looks for entry with corresponding path in this list
 * if it does not exist it adds it
 * the in tries to lock the mutex
 */
typedef struct _tempNode{
	char* path;
	pthread_mutex_t* mutex;
	struct _tempNode* next;
} mtxList;

/*
 * mutexes are used for synchronizing access to the lists
 */
typedef struct
{
    int socketFd;
	listNode* head;
	pthread_mutex_t* listMutex;
	mtxList* fileList;
	pthread_mutex_t* fileListMutex;
} thread_arg;

off_t getfilelength(int);
int fileExists(char*);
void setAiocbRead(struct aiocb*, char*);
void setAiocbAppend(struct aiocb*, char*, char*);
void setAiocbWrite(struct aiocb*, char*, char*);
void waitAiocb(struct aiocb*);
void disposeAiocb(struct aiocb*);
ssize_t read_line(int, char*, size_t);
ssize_t bulk_write(int, char*, size_t);
void getLoginInfo(struct aiocb*);
int add_new_client(int);
int make_socketServ(int, int);
int bind_inet_socket(uint16_t);
struct sockaddr_in make_address(char*, uint16_t);
int getIntForFieldName(char*);
#endif //UNIXPROJECT_UTILS_H
