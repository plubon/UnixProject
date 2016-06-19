#include "utils.h"

volatile sig_atomic_t dowork=1;

void lockTwoFiles(thread_arg targ, char* path1, char* path2)
{
    pthread_mutex_lock(targ.fileListMutex);
    mtxList* first = NULL;
    mtxList* second = NULL;
    mtxList *curr = targ.fileList;
    while(curr!=NULL)
    {
        if(strcmp(curr->path, path1) == 0)
            first = curr;
        if(strcmp(curr->path, path2) == 0)
            second = curr;
        curr = curr->next;
    }
    curr = targ.fileList;
    while(curr->next!=NULL)
        curr = curr->next;
    if(first == NULL)
    {
        first = malloc(sizeof(mtxList));
        pthread_mutex_t *newMtx = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(newMtx, NULL);
        first->mutex = newMtx;
        first->path = malloc(strlen(path1) + 1);
        memcpy(first->path, path1, strlen(path1) + 1);
        first->next = NULL;
        curr->next = first;
        curr = first;
    }
    if(second == NULL)
    {
        second = malloc(sizeof(mtxList));
        pthread_mutex_t *newMtx = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(newMtx, NULL);
        second->mutex = newMtx;
        second->path = malloc(strlen(path2) + 1);
        memcpy(second->path, path2, strlen(path2) + 1);
        second->next = NULL;
        curr->next = second;
    }
    pthread_mutex_unlock(targ.fileListMutex);
    if(strcmp(path1, path2)<0)
    {
        pthread_mutex_lock(first->mutex);
        pthread_mutex_lock(second->mutex);
    }
    else
    {
        pthread_mutex_lock(second->mutex);
        pthread_mutex_lock(first->mutex);
    }
}

void unlockTwoFiles(thread_arg targ, char* path1, char* path2)
{
    mtxList* first = NULL;
    mtxList* second = NULL;
    mtxList *curr = targ.fileList;
    while(curr!=NULL)
    {
        if(strcmp(curr->path, path1) == 0)
            first = curr;
        if(strcmp(curr->path, path2) == 0)
            second = curr;
        curr = curr->next;
    }
    if(strcmp(path1, path2)<0)
    {
        pthread_mutex_unlock(first->mutex);
        pthread_mutex_unlock(second->mutex);
    }
    else
    {
        pthread_mutex_unlock(second->mutex);
        pthread_mutex_unlock(first->mutex);
    }
}

void lockFile(thread_arg targ, char* path)
{
    pthread_mutex_lock(targ.fileListMutex);
    mtxList *curr = targ.fileList;
    while(curr!=NULL)
    {
        if(strcmp(curr->path, path) == 0)
        {
            pthread_mutex_unlock(targ.fileListMutex);
            pthread_mutex_lock(curr->mutex);
            return;
        }
        else
            curr = curr->next;
    }
    curr = targ.fileList;
    while(curr->next!=NULL)
        curr = curr->next;
    mtxList *new = malloc(sizeof(mtxList));
    pthread_mutex_t* newMtx = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(newMtx, NULL);
    new->mutex = newMtx;
    pthread_mutex_lock(new->mutex);
    new->path = malloc(strlen(path)+1);
    memcpy(new->path, path, strlen(path)+1);
    new->next = NULL;
    curr->next = new;
    pthread_mutex_unlock(targ.fileListMutex);
}

void unlockFile(thread_arg targ, char* path)
{
    pthread_mutex_lock(targ.fileListMutex);
    mtxList *curr = targ.fileList;
    while(curr!=NULL)
    {
        if(strcmp(curr->path, path) == 0)
        {
            pthread_mutex_unlock(curr->mutex);
            pthread_mutex_unlock(targ.fileListMutex);
            return;
        }
        else
            curr = curr->next;
    }
    pthread_mutex_unlock(targ.fileListMutex);
}

int validateLoginInfo(char* file, char* login, char* pwd)
{
    char *saveptr;
    char *line;
    line = strtok_r(file, "\n", &saveptr);
    while(line != NULL)
    {
        int index = strcspn(line, ";");
        int loginOk = 1;
        int i =0 ;
        while(i<index && login[i]!='\r' && login[i]!='\0')
        {
            if(line[i]!=login[i])
            {
                loginOk = 0;
                break;
            }
            i++;
        }
        if(loginOk)
        {
            int pwdOk=1;
            i=0;
            while(line[i+index+1]!='\0' && line[i+index+1]!='\n' && pwd[i]!='\r' && pwd[i]!='\0')
            {
                if(line[i+index+1]!=pwd[i])
                {
                    pwdOk = 0;
                    break;
                }
                i++;
            }
            if(pwdOk)
                return 1;
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }
    return 0;
}

void updateProfileFile(char* path, int no, char* value, thread_arg targ)
{
    char *values[6] = {"|", "|", "|", "|", "|", "|"};
    if(fileExists(path)) {
        struct aiocb aios;
        lockFile(targ, path);
        setAiocbRead(&aios, path);
        waitAiocb(&aios);
        unlockFile(targ, path);
        char *saveptr;
        char *val;
        int i = 0;
        val = strtok_r((char*)aios.aio_buf, "\t", &saveptr);
        while(val != NULL && i < 6)
        {
            values[i] = val;
            i++;
            val = strtok_r(NULL, "\t", &saveptr);
        }
        //disposeAiocb(&aios);
    }
    values[no - 1] = value;
    char buffer[4096];
    sprintf(buffer,"%s\t%s\t%s\t%s\t%s\t%s", values[0], values[1], values[2], values[3], values[4], values[5]);
    struct aiocb aios;
    lockFile(targ, path);
    setAiocbWrite(&aios, buffer, path);
    waitAiocb(&aios);
    unlockFile(targ, path);
        //disposeAiocb(&aios);
}

void setProfile(thread_arg targ, char* username, char* args)
{
    int clientFd = targ.socketFd;
    int argsOk = 1;
    char wd[1024];
    getcwd(wd, 1024);
    char path[1024];
    sprintf(path, "%s/storage/profiles/%s", wd, username);
    char *saveptr;
    char *strNo;
    strNo = strtok_r(args, " ", &saveptr);
    int no = atoi(strNo);
    if(no > 6 || no < 1)
        argsOk = 0;
    if(argsOk) {
        updateProfileFile(path, no, saveptr, targ);
        if (0 >= bulk_write(clientFd, "Successfully updated profile\n", sizeof("Successfully updated profile\n")))
            ERR("Write error");
    }
    else {
        if (0 >= bulk_write(clientFd, "invalid arguments for set profile\n", sizeof("invalid arguments for set profile\n")))
            ERR("Write error");
        return;
    }
}

void printProfile(char* path, int fd, char* username, thread_arg targ)
{
    char** names = (char *[]){"1","2","3","4","5","6"};
    struct aiocb aios;
    lockFile(targ, path);
    setAiocbRead(&aios, path);
    waitAiocb(&aios);
    unlockFile(targ, path);
    char *saveptr;
    char *line;
    line = strtok_r((char*)aios.aio_buf, "\t", &saveptr);
    int i =0;
    if(0 >= bulk_write(fd, username, strlen(username)))
        ERR("Write error");
    if(0 >= bulk_write(fd, ":\n", sizeof(":\n")))
        ERR("Write error");
    while(line != NULL && i < 6)
    {
        if(0 >= bulk_write(fd, names[i], strlen(names[i])))
            ERR("Write error");
        if(0 >= bulk_write(fd, ":", sizeof(":")))
            ERR("Write error");
        if(0 >= bulk_write(fd, line, strlen(line)))
            ERR("Write error");
        if(0 >= bulk_write(fd, "\n", sizeof("\n")))
            ERR("Write error");
        line = strtok_r(NULL, "\t", &saveptr);
        i++;
    }
}

void showProfile(thread_arg targ, char* username)
{
    char path[1024];
    char wd[1024];
    getcwd(wd, 1024);
    sprintf(path, "%s/storage/profiles/%s", wd, username);
    if(!fileExists(path))
    {
        if(0 >= bulk_write(targ.socketFd, "invalid username\n", sizeof("invalid username\n")))
            ERR("Write error");
    }
    else
    {
        printProfile(path, targ.socketFd, username, targ);
    }
}

void showAllProfiles(thread_arg targ)
{
    struct dirent *dp;
    DIR *dirp = NULL;
    char wd[1024];
    getcwd(wd, 1024);
    char path[1024];
    sprintf(path, "%s/storage/profiles", wd);
    if (NULL == (dirp = opendir(path)))
        ERR("opening dir");
    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            if(strcmp(dp->d_name, ".") !=0 && strcmp(dp->d_name,"..") !=0)
                showProfile(targ, dp->d_name);
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("directories");
}



void viewProfile(thread_arg targ, char* username, char* args)
{
    char *saveptr;
    char* opt = strtok_r(args, " ", &saveptr);
    if(opt[0] == 'a')
        showAllProfiles(targ);
    else
        showProfile(targ, saveptr);
}

int checkProfile(char* username, int n, char* value, thread_arg targ)
{
    char path[1024];
    char wd[1024];
    getcwd(wd, 1024);
    sprintf(path, "%s/storage/profiles/%s", wd, username);
    struct aiocb aios;
    lockFile(targ, path);
    setAiocbRead(&aios, path);
    waitAiocb(&aios);
    unlockFile(targ, path);
    char *saveptr;
    char *line;
    line = strtok_r((char*)aios.aio_buf, "\t", &saveptr);
    int i=0;
    while(line != NULL && i < 6)
    {
        if(i == n-1)
        {
            if(strcmp(line, value) == 0)
                return 1;
            else
                return 0;
        }
        line = strtok_r(NULL, "\t", &saveptr);
        i++;
    }
}

void findProfile(thread_arg targ, char* username, char* args)
{
    char *saveptr;
    char* no = strtok_r(args, " ", &saveptr);
    int n = atoi(no);
    if(n<0 || n>6)
    {
        if(0 >= bulk_write(targ.socketFd, "invalid command\n", sizeof("invalid command\n")))
            ERR("Write error");
    }
    else
    {
        struct dirent *dp;
        DIR *dirp = NULL;
        char wd[1024];
        getcwd(wd, 1024);
        char path[1024];
        sprintf(path, "%s/storage/profiles/", wd);
        if (NULL == (dirp = opendir(path)))
            ERR("opening dir");
        do {
            errno = 0;
            if ((dp = readdir(dirp)) != NULL)
            {
                if(strcmp(dp->d_name, ".") !=0 && strcmp(dp->d_name,"..") !=0)
                {
                    if(checkProfile(dp->d_name, n, saveptr, targ))
                    {
                        if(0 >= bulk_write(targ.socketFd, dp->d_name, strlen(dp->d_name)))
                            ERR("Write error");
                        if(0 >= bulk_write(targ.socketFd, "\n", strlen("\n")))
                            ERR("Write error");
                    }
                }
            }
        } while (dp != NULL);
    }
}

int findFdInList(listNode* head, char* username)
{
    while(head!=NULL)
    {
        if(strcmp(head->username, username) == 0)
            return head->val;
        head = head->next;
    }
    return -1;
}

void inviteProfile(thread_arg targ, char* username, char* args)
{
    pthread_mutex_lock(targ.listMutex);
    int fd = findFdInList(targ.head, args);
    pthread_mutex_unlock(targ.listMutex);
    if(fd > 0) {
        if (0 >= bulk_write(fd, "You have pending invites\n", strlen("You have pending invites\n")))
            ERR("Write error");
    }
    struct aiocb aios;
    char buffer[1024];
    char path[1024];
    char wd[1024];
    getcwd(wd, 1024);
    sprintf(path, "%s/storage/invites/%s", wd, args);
    sprintf(buffer, "%s\n", username);
    lockFile(targ, path);
    setAiocbAppend(&aios, buffer, path);
    waitAiocb(&aios);
    unlockFile(targ, path);
}

void getCalendar(char* calendar, char* path)
{
    if(fileExists(path)) {
        struct aiocb aios;
        setAiocbRead(&aios, path);
        waitAiocb(&aios);
        char* buf = (char*)aios.aio_buf;
        for(int i=0; i<21; i++)
            calendar[i]=buf[i];
    }
    else
    {
        char buf[22];
        for(int i=0; i<21; i++) {
            calendar[i] = '0';
            buf[i] = '0';
        }
        buf[21] = '\0';
        struct aiocb aios;
        setAiocbWrite(&aios, buf,path);
        waitAiocb(&aios);
    }
}

void printIntersection(thread_arg targ, char* cal1, char* cal2)
{
    char result[21];
    char* days[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    char* hours[] = {"17", "18", "19"};
    for(int i=0; i<21; i++)
    {
        if(cal1[i]=='0' && cal2[i]=='0')
            result[i]='1';
        else
            result[i]='0';
    }
    for(int i=0; i<21; i++)
    {
        char buf[1024];
        sprintf(buf, "%s %s:%c\n", days[i/3], hours[i%3], result[i]);
        if (0 >= bulk_write(targ.socketFd, buf, strlen(buf)))
            ERR("Write error");
    }
}

int getNoOfDay(char* line)
{
    char* days[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    for(int i=0; i<7; i++)
    {
        if(strcmp(line, days[i]) == 0)
            return i;
    }
    return -1;
}

int updateCalendars(char* username, char* cal1, char* usr2, char* cal2, char* time)
{
    char *saveptr;
    char *line;
    line = strtok_r((char*)time, " ", &saveptr);
    int h = atoi(saveptr);
    int d = getNoOfDay(line);
    int idx = d*3 + h-17;
    if(cal1[idx] == '0' && cal2[idx] == '0')
    {
        cal1[idx]='1';
        cal2[idx]='1';
        char buf1[22], buf2[22];
        memcpy(buf1, cal1, 21);
        memcpy(buf2, cal2, 21);
        buf1[21] = '\0';
        buf2[21] = '\0';
        struct aiocb aios1, aios2;
        char path[1024];
        char wd[1024];
        getcwd(wd, 1024);
        sprintf(path, "%s/storage/calendars/%s", wd, username);
        setAiocbWrite(&aios1, buf1, path);
        sprintf(path, "%s/storage/calendars/%s", wd, usr2);
        setAiocbWrite(&aios2, buf2, path);
        waitAiocb(&aios1);
        waitAiocb(&aios2);
        return 1;
    }
    else
        return 0;
}

void removeInvite(thread_arg targ, char* username, char* toRemove)
{
    struct aiocb aios;
    char path[1024];
    char wd[1024];
    getcwd(wd, 1024);
    sprintf(path, "%s/storage/invites/%s", wd, username);
    lockFile(targ, path);
    setAiocbRead(&aios, path);
    waitAiocb(&aios);
    unlockFile(targ, path);
    char *saveptr;
    char *line;
    line = strtok_r((char*)aios.aio_buf, "\n", &saveptr);
    int i=0;
    while(line != NULL)
    {
        if(strcmp(line, toRemove) != 0)
        {
            if(i==0)
            {
                struct aiocb aios;
                lockFile(targ, path);
                setAiocbWrite(&aios, line, path);
                waitAiocb(&aios);
                unlockFile(targ, path);
            }
            else
            {
                lockFile(targ, path);
                setAiocbAppend(&aios, line, path);
                waitAiocb(&aios);
                unlockFile(targ, path);
            }
            i++;
        }
        line = strtok_r(NULL, "\t", &saveptr);
    }
    if(i == 0)
    {
        remove(path);
    }
}

void scheduleDate(thread_arg targ, char* username, char* args)
{
    struct aiocb aios;
    char path[1024], path1[1024];
    char wd[1024];
    getcwd(wd, 1024);
    sprintf(path, "%s/storage/invites/%s", wd, username);
    lockFile(targ, path);
    setAiocbRead(&aios, path);
    waitAiocb(&aios);
    unlockFile(targ, path);
    if (0 >= bulk_write(targ.socketFd, (char*)aios.aio_buf, strlen((char*)aios.aio_buf)))
        ERR("Write error");
    char line[1024];
    memset(line, '\0', 1024);
    while(read_line(targ.socketFd, line, 1024)<=0);
    char cal1[21], cal2[21];
    sprintf(path, "%s/storage/calendars/%s", wd, username);
    sprintf(path1, "%s/storage/calendars/%s", wd, line);
    lockTwoFiles(targ, path, path1);
    getCalendar(cal1, path);
    getCalendar(cal2, path1);
    printIntersection(targ, cal1, cal2);
    char line1[1024];
    while(read_line(targ.socketFd, line1, 1024)<=0);
    while(!updateCalendars(username, cal1, line, cal2, line1)) {
        if (0 >= bulk_write(targ.socketFd, "Provided date is taken\n", strlen("Provided date is taken\n")))
            ERR("Write error");
        while (read_line(targ.socketFd, line, 1024) <= 0);
    }
    removeInvite(targ, username, line);
    unlockTwoFiles(targ, path, path1);
}

void handleCommands(thread_arg targ, char* username)
{
    int doWork = 1;
    int clientFd = targ.socketFd;
    if(0>=bulk_write(clientFd, "logged in\n", sizeof("logged in\n")))
        ERR("Write error");
    while(doWork)
    {
        char line[1024];
        while(read_line(clientFd, line, 1024)<=0);
        char *saveptr;
        char *token;
        token = strtok_r(line, " ", &saveptr);
        if(strcmp(token, "set") == 0)
            setProfile(targ, username, saveptr);
        else if(strcmp(token, "view") == 0)
            viewProfile(targ, username, saveptr);
        else if(strcmp(token, "find") == 0)
            findProfile(targ, username, saveptr);
        else if(strcmp(token, "invite") == 0)
            inviteProfile(targ, username, saveptr);
        else if(strcmp(token, "schedule") == 0)
            scheduleDate(targ, username, saveptr);
        else {
            if (0 >= bulk_write(clientFd, "invalid command\n", sizeof("invalid command\n")))
                ERR("Write error");
        }
    }

}

void appendToList(listNode* head, int fd, char* username)
{
    listNode* current = head;
    while(current->next!=NULL)
        current = current->next;
    listNode* new = malloc(sizeof(listNode));
    new->val = fd;
    new->username = username;
    new->next=NULL;
    current->next = new;
}

void checkInvites(thread_arg targ, char* username)
{
    char path[1024];
    char wd[1024];
    getcwd(wd, 1024);
    sprintf(path, "%s/storage/invites/%s", wd, username);
    if(fileExists(path))
    {
        if (0 >= bulk_write(targ.socketFd, "You have pending invites\n", strlen("You have pending invites\n")))
            ERR("Write error");
    }
}

int validateRegisterInfo(char* file, char* login, char* pwd, thread_arg targ)
{
    char *saveptr;
    char *line;
    line = strtok_r(file, "\n", &saveptr);
    while(line != NULL)
    {
        char* rest;
        char* oldLogin;
        oldLogin = strtok_r(line, ";", &rest);
        if(strcmp(login, oldLogin) == 0)
            return 0;
        line = strtok_r(NULL, "\n", &saveptr);
    }
    char wd[1024];
    getcwd(wd, 1024);
    char path[1024];
    sprintf(path, "%s/storage/pwd", wd);
    lockFile(targ, path);
    struct aiocb aios;
    char* buffer[1024];
    memset(buffer, '\0', 1024);
    sprintf(buffer, "%s;%s\n", login, pwd);
    setAiocbAppend(&aios, buffer, path);
    waitAiocb(&aios);
    unlockFile(targ, path);
    return 1;
}

void registerUser(thread_arg targ)
{
    char loginBuffer[1024];
    char pwdBuffer[1024];
    int clientFd = targ.socketFd;
    memset(loginBuffer, '\0', 1024);
    memset(pwdBuffer, '\0', 1024);
    char msg[] = "login:\n";
    if (0 >= bulk_write(clientFd, msg, 7))
        ERR("Write error");
    struct aiocb aios;
    char wd[1024];
    getcwd(wd, 1024);
    char path[1024];
    sprintf(path, "%s/storage/pwd", wd);
    lockFile(targ, path);
    getLoginInfo(&aios);
    while (read_line(clientFd, loginBuffer, 1024) <= 0);
    char msg1[] = "password:\n";
    if (0 >= bulk_write(clientFd, msg1, 10))
        ERR("Write error");
    while (read_line(clientFd, pwdBuffer, 1024) <= 0);
    waitAiocb(&aios);
    unlockFile(targ, path);
    while (!validateRegisterInfo((char *) aios.aio_buf, loginBuffer, pwdBuffer, targ))
    {
        if(0>=bulk_write(clientFd, "This username is taken\n", strlen("This username is taken\n")))
            ERR("Write error");
    }
    disposeAiocb(&aios);
    pthread_mutex_lock(targ.listMutex);
    appendToList(targ.head, clientFd, loginBuffer);
    pthread_mutex_unlock(targ.listMutex);
    checkInvites(targ, loginBuffer);
    handleCommands(targ, loginBuffer);
}

void *readingThreadFunc(void *arg)
{
    thread_arg targ;
    memcpy(&targ, arg, sizeof(targ));
    int clientFd = targ.socketFd;
    if(0>=bulk_write(clientFd, "(L)ogin/(R)egister?\n", strlen("(L)ogin/(R)egister?\n")))
        ERR("Write error");
    char command[1024];
    while(read_line(clientFd, command, 1024)<=0);
    if(command[0]=='L' && command[0]=='l') {
        char loginBuffer[1024];
        char pwdBuffer[1024];
        memset(loginBuffer, '\0', 1024);
        memset(pwdBuffer, '\0', 1024);
        memcpy(&targ, arg, sizeof(targ));
        char msg[] = "login:\n";
        if (0 >= bulk_write(clientFd, msg, 7))
            ERR("Write error");
        struct aiocb aios;
        char wd[1024];
        getcwd(wd, 1024);
        char path[1024];
        sprintf(path, "%s/storage/pwd", wd);
        lockFile(targ, path);
        getLoginInfo(&aios);
        while (read_line(clientFd, loginBuffer, 1024) <= 0);
        char msg1[] = "password:\n";
        if (0 >= bulk_write(clientFd, msg1, 10))
            ERR("Write error");
        while (read_line(clientFd, pwdBuffer, 1024) <= 0);
        waitAiocb(&aios);
        unlockFile(targ, path);
        if (validateLoginInfo((char *) aios.aio_buf, loginBuffer, pwdBuffer)) {
            disposeAiocb(&aios);
            pthread_mutex_lock(targ.listMutex);
            appendToList(targ.head, clientFd, loginBuffer);
            pthread_mutex_unlock(targ.listMutex);
            checkInvites(targ, loginBuffer);
            handleCommands(targ, loginBuffer);
        }
        else {
            disposeAiocb(&aios);
            if (0 >= bulk_write(clientFd, "invalid credentials\n", sizeof("invalid credentials\n")));
            ERR("Write error");
        }
    }
    else
        registerUser(targ);
    //close(clientFd);
}

void initThread(pthread_t *thread, int socketFd, thread_arg *targ, int no, pthread_mutex_t* mtx, listNode* head, pthread_mutex_t* mtx1, mtxList* lst)
{
    targ[no].socketFd = socketFd;
    targ[no].listMutex = mtx;
    targ[no].head = head;
    targ[no].fileListMutex = mtx1;
    targ[no].fileList=lst;
    if(pthread_create(thread, NULL, readingThreadFunc, (void *) &targ[no]) != 0)
        ERR("pthread_create");
}

int main(int argc, char **argv)
{
    pthread_t thread[10];
    thread_arg targ[10];
    pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER;
    listNode *head = malloc(sizeof(listNode));
    head->next=NULL;
    head->val=-1;
    head->username = "|";
    mtxList *mtxlst = malloc(sizeof(mtxList));
    mtxlst->next = NULL;
    mtxlst->path="1";
    mtxlst->mutex = NULL;
    int threads = 0;
    int port;
    if(argc!=2)
    {
        /*fprintf(stderr,"Please provide a port number\n");
        fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);
        printf("USAGE : ./server portNumber\n");
        return EXIT_FAILURE;*/
        port = 8001;
    }
    else
        port = atoi(argv[1]);
    if(port <= 0)
    {
        fprintf(stderr,"Incorrect port number provided : FAILURE\n");
        fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);
        printf("USAGE : ./lubonpLab4Server portNumber\n");
        return EXIT_FAILURE;
    }
    int socketFd = bind_inet_socket(port);
    while(dowork)
    {
        int clientFd = add_new_client(socketFd);
        if(threads < 10)
        {
            initThread(&thread[threads], clientFd, targ, threads, &mutex, head, &mtx1, mtxlst);
            //pthread_detach(thread[threads]);
            threads++;
            printf("Server accepted connection\n");
            //close(clientFd);
            //printf("Server closed connection\n");
        }
    }
    return EXIT_SUCCESS;
}
