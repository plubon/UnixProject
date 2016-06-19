#include "utils.h"

off_t getfilelength(int fd)
{
    struct stat buf;
    if (fstat(fd, &buf) == -1)
        ERR("Cannot fstat file");
    return buf.st_size;
}


int fileExists(char* path)
{
    if (0 == access(path, 0)) {
        return 1;
    }
    else {
        return 0;
    }
}

char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    if (result == NULL)
        ERR("malloc");
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;

    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;
        buf += c;
        len += c;
        count -= c;
    }
    while (count > 0);

    return len;
}

void setAiocbRead(struct aiocb *aios, char* path)
{
    memset(aios, 0, sizeof(struct aiocb));
    int fd = open(path, O_RDONLY, S_IRUSR|S_IWUSR);
    if(fd < 0)
    {
        printf(path);
        ERR("pwdFd");
    }
    off_t filesize =  getfilelength(fd);
    aios->aio_fildes = fd;
    aios->aio_offset = 0;
    char* buffer = (char*)malloc(filesize+1);
    memset(buffer, '\0', sizeof(buffer));
    aios->aio_buf = buffer;
    aios->aio_nbytes = filesize;
    aios->aio_lio_opcode = LIO_READ;
    if(aio_read(aios) == -1)
    {
    printf("Value of errno: %d\n", errno);
    //ERR("aio read");
    }
    if(errno == EBADF)
    printf("%d", 1);
    if(errno == EINVAL)
    printf("%d", 2);
    if(errno == ENOSYS)
    printf("%d", 3);
}

void setAiocbAppend(struct aiocb *aios, char* buffer, char* path)
{
    memset(aios, 0, sizeof(struct aiocb));
    int fd = open(path, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    if(fd < 0)
        ERR("pwdFd");
    //ERR("pwdFd");
    aios->aio_fildes = fd;
    aios->aio_offset = 0;
    aios->aio_buf = buffer;
    aios->aio_nbytes = strlen(buffer);
    aios->aio_lio_opcode = LIO_WRITE;
    if(aio_write(aios) == -1)
    {
        printf("Value of errno: %d\n", errno);
        //ERR("aio read");
    }
    if(errno == EBADF)
        printf("%d", 1);
    if(errno == EINVAL)
        printf("%d", 2);
    if(errno == ENOSYS)
        printf("%d", 3);
}

void setAiocbWrite(struct aiocb *aios, char* buffer, char* path)
{
    memset(aios, 0, sizeof(struct aiocb));
    int fd = open(path, O_WRONLY|O_CREAT| O_TRUNC, S_IRUSR|S_IWUSR);
    if(fd < 0) {
        printf(path);
        ERR("pwdFd");
    }
    //ERR("pwdFd");
    aios->aio_fildes = fd;
    aios->aio_offset = 0;
    aios->aio_buf = buffer;
    aios->aio_nbytes = strlen(buffer);
    aios->aio_lio_opcode = LIO_WRITE;
    if(aio_write(aios) == -1)
    {
        printf("Value of errno: %d\n", errno);
        //ERR("aio read");
    }
    if(errno == EBADF)
        printf("%d", 1);
    if(errno == EINVAL)
        printf("%d", 2);
    if(errno == ENOSYS)
        printf("%d", 3);
}

void waitAiocb(struct aiocb *aios)
{
    const struct aiocb* constArr[1] = {aios};
    while(aio_suspend(constArr, 1, NULL)!=0);
    if (aio_error(aios) != 0)
        ERR("Suspend error");
    if (aio_return(aios) == -1)
        ERR("Return error");
    close(aios->aio_fildes);
}

void disposeAiocb(struct aiocb *aios)
{
    free((void*)(aios->aio_buf));
}

ssize_t read_line(int fd, char *buf, size_t count)
{
    int c;
    int finished = 0;
    size_t len = 0;
    char readChar[1];
    memset(buf,'\0',count);
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, readChar, 1));
        if (c < 0)
            return c;
        if (c == 0)
            return len;
        if(readChar[0] == '\r' || readChar[0] == '\n')
            finished = 1;
        if(!finished) {
            buf[len] = readChar[0];
            len += c;
            count -= c;
        }
    }
    while (count > 0 && !finished);
    if (len > 0) {
        buf[len] = '\0';
        len++;
    }
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if(c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    }
    while (count > 0);

    return len;
}

void getLoginInfo(struct aiocb *aios)
{
    char buf[1024];
    getcwd(buf, 1024);
    char path[1024];
    sprintf(path, "%s/storage/pwd", buf);
    setAiocbRead(aios, path);
}

int add_new_client(int sfd){
//accept connection equivalent
    int nfd;
    if((nfd=TEMP_FAILURE_RETRY(accept(sfd,NULL,NULL)))<0) {
        if(EAGAIN==errno||EWOULDBLOCK==errno) return -1;
        ERR("accept");
    }
    return nfd;
}

int make_socketServ(int domain, int type){
    int sock;
    sock = socket(domain,type,0);
    if(sock < 0) ERR("socket");
    return sock;
}

int bind_inet_socket(uint16_t port){
//TYPES - SOCK_STREAM, SOCK_DGRAM
    int type = SOCK_STREAM;
    struct sockaddr_in addr;
    int socketfd,t=1;
    socketfd = make_socketServ(PF_INET,type);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) ERR("setsockopt");
    if(bind(socketfd,(struct sockaddr*) &addr,sizeof(addr)) < 0)  ERR("bind");
    if(SOCK_STREAM==type)
    if(listen(socketfd, BACKLOG) < 0) ERR("listen");
    return socketfd;
}

struct sockaddr_in make_address(char *address, uint16_t port)
{
    struct sockaddr_in addr;
    struct hostent *hostinfo;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    hostinfo = gethostbyname(address);
    if (hostinfo == NULL)
        ERR("gethostbyname");

    addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;

    return addr;
}
