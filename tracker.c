#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <pthread.h>
void error(const char *msg) {
    perror(msg);
    exit(1);
}

pthread_mutex_t files_mutex = PTHREAD_MUTEX_INITIALIZER;
#define FILES_MAX 10000
char files[FILES_MAX][10];

void socket_reuse(int fd) {
    long val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1) {
        perror("setsockopt()");
        exit(1);
    }
}

ssize_t RecvN (int sockfd, void *buf, size_t len, int flags) {
    fd_set rfds;
    struct timeval tv;
    int retval;
    int read_len = 0;
    char *ptr = (char*) buf;

    while (read_len < len) {
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        retval = select(sockfd+1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            perror("select()");
            return 0;
        } else if (retval) {
            int l;
            l = read(sockfd,ptr,len - read_len);
            if (l > 0) {
                ptr += l;
                read_len+=l;
            } else if (l <= 0) {
                return read_len;
            }
        } else {
            printf("No data within 2 seconds.\n");
            return 0;
        }
    }
    return read_len;
}

void client_fail(int clientfd){
    char fail[2] = {0x21, 0};
    write(clientfd, fail, 2);
    close(clientfd);
    puts("Fail to register");
    pthread_exit(0);
}

void process_setup(int sockfd){
#undef CommandLen
#define CommandLen (4+2+4+12)
    char command[CommandLen];
    int clientSockfd, portno, n;
    struct sockaddr_in serv_addr;
    char client_test[2] = {0x2, 0};
    
    puts("-- New client set-up --");

    if(RecvN(sockfd,command,CommandLen,0) != CommandLen){
        puts("Command Length wrong ... ");
        close(sockfd);
        pthread_exit(0);
    } else {
        char command_tmp[10], *ptr;
        ptr = command_tmp;
        if(!(ntohl(*(unsigned*)command) == 4)){
            puts("Wrong msg format. ");
            close(sockfd);
            pthread_exit(0);
        }
        if(!(ntohl(*(unsigned*)(command +  8)) == 2)){
            puts("Wrong msg format. ");
            close(sockfd);
            pthread_exit(0);
        }
        if(!(ntohl(*(unsigned*)(command +  14)) == 4)){
            puts("Wrong msg format. ");
            close(sockfd);
            pthread_exit(0);
        }
        memcpy(ptr,command+4,4);
        ptr+=4;
        memcpy(ptr,command+12,2);
        ptr+=2;
        memcpy(ptr,command+18,4);
        memcpy(command,command_tmp,sizeof(command_tmp));
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    clientSockfd = socket(AF_INET, SOCK_STREAM, 0);
    memcpy(&serv_addr.sin_addr.s_addr, command, 4);
    memcpy(&serv_addr.sin_port, command+4, 2);
    serv_addr.sin_family = AF_INET;

    puts("-- Connect to client --");
    if (connect(clientSockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        char ip[16];
        unsigned short port;
        inet_ntop(AF_INET, &serv_addr.sin_addr, ip, 16);
        port = serv_addr.sin_port;
        printf("IP = %s : %d", ip, ntohs(port));
        perror("connect()");
        client_fail(sockfd);
    }
//    getchar(); 
    puts("-- Reply to client with 0x02 --");
    write(clientSockfd, client_test, 2);
    
    puts("-- Wait for client reply --");
    if (RecvN(clientSockfd, client_test, 2, 0) != 2){
        client_fail(sockfd);
    }
    
    puts("-- Try if client reply 0x12 --");
    if (client_test[0] != 0x12) {
        client_fail(sockfd);
    }

    client_test[0] = 0x11;
    client_test[1] = 0;
    write(sockfd, client_test, 2);
    close(clientSockfd);
    close(sockfd);
    puts("success!");
    pthread_mutex_lock(&files_mutex);

    {
        char tmp[10];
        memset(tmp,0,10);
        for(int i = 0; i < FILES_MAX; i++){
            if(memcmp(files[i], command, 10) == 0){
                puts("peer already here");
                goto finished;
            }
        }
        for(int i = 0; i < FILES_MAX; i++){
            if(memcmp(files[i], tmp, 10) == 0){
                memcpy(files[i], command, 10);
                goto finished;
            }
        }
        puts("OOOOOOOps~~???");
    }
finished:
    pthread_mutex_unlock(&files_mutex);
}
void process_downloadlist(int sockfd){
    char fileID[8];
    char msgBuf[255*10+100];
    char * ptr = msgBuf + 2;
    unsigned char count = 0;
    unsigned len = htonl(6);

    puts("-- Handle downloadlist --");

    if(RecvN(sockfd,fileID,8,0) != 8){
        puts("Cannot get enough length ... ");
        client_fail(sockfd);
    }

    puts("Getting lock .. ");
    pthread_mutex_lock(&files_mutex);
    puts("Got it! ");
    for(int i = 0; i < FILES_MAX; i++){
        if(memcmp(files[i] + 6,fileID+4,4) == 0){
            memcpy(ptr,&len,4);
            ptr+=4;
            memcpy(ptr,files[i],6);
            ptr+=6;
            count++;
        }
    }
    msgBuf[0] = 0x14;
    msgBuf[1] = count;

    pthread_mutex_unlock(&files_mutex);
    fprintf(stderr, "Sending message ... size = %d (item = %d)\n", count * 10 + 2, count);
    write(sockfd,msgBuf,count * 10 + 2);
    close(sockfd);
}

void process_unreg(int sockfd){
    char info[22];
    if(RecvN(sockfd,&info,22,0) != 22){
        client_fail(sockfd);
    }
    else{
        char command_tmp[10], *ptr;
        ptr = command_tmp;
        if(!(ntohl(*(unsigned*)info) == 4)){
            puts("Failed1");
            close(sockfd);
            pthread_exit(0);
        }
        if(!(ntohl(*(unsigned*)(info + 8)) == 2)){
            puts("Failed2");
            close(sockfd);
            pthread_exit(0);
        }
        if(!(ntohl(*(unsigned*)(info + 14)) == 4)){
            puts("Failed3");
            close(sockfd);
            pthread_exit(0);
        }
        memcpy(ptr,info+4,4);
        ptr+=4;
        memcpy(ptr,info+12,2);
        ptr+=2;
        memcpy(ptr,info+18,4);
        memcpy(info,command_tmp,sizeof(command_tmp));

    }
    pthread_mutex_lock(&files_mutex);
    for(int i = 0; i < FILES_MAX; i++){
        if(memcmp(files[i], info, 10) == 0){
            memset(files[i], 0, 10);
            pthread_mutex_unlock(&files_mutex);
            info[0] = 0x13;
            info[1] = 0;
            write(sockfd, info, 2);
            close(sockfd);
            return;
        }
    }
    pthread_mutex_unlock(&files_mutex);
    info[0] = 0x23;
    info[1] = 0;
    write(sockfd, info, 2);
    close(sockfd);
    return;
}

void handle_client(int sockfd){
    char command[2];
    if (RecvN(sockfd, &command, 2, 0) != 2) {
        client_fail(sockfd);
    }
    if (command[0] == 0x01) {
        process_setup(sockfd);
    }
    if (command[0] == 0x03) {
        process_unreg(sockfd);
    }
    if (command[0] == 0x04) {
        puts("downloadlist");
        process_downloadlist(sockfd);
    }
}
void accept_thread(int portno){
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    socket_reuse(sockfd);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0){
        error("ERROR on binding");
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while(1) {
        pthread_t tmp;
        puts("Waiting for client ...");
        newsockfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }
        puts("Client connected");
        pthread_create(&tmp, NULL, (void *(*)(void *)) handle_client, (void*) newsockfd);
    }
    ///...
}

int list() {
    char zeros[10];
    struct in_addr addr;
    short port = 0;
    unsigned fileID = 0;
    memset(zeros, 0, sizeof(zeros));
    pthread_mutex_lock(&files_mutex);
    for(int i = 0; i < FILES_MAX; i++){
        if(memcmp(files[i], zeros, 10) == 0){
            continue;
        }
        memcpy(&addr.s_addr,files[i], 4);
        memcpy(&port, files[i]+4, 2);
        memcpy(&fileID, files[i]+6, 4);

        port = ntohs(port);
        fileID = ntohl(fileID);
        char * ip =  inet_ntoa(addr);

        printf("ip = %s\t:%i\tfileID = %X\n", ip, port, fileID);

    }
    pthread_mutex_unlock(&files_mutex);
}

int command(){
    char command[100];
    while (1) {
        printf(">> ");
        if (fgets(command,100,stdin) != NULL) {
            if (memcmp(command, "list", 4) == 0) {
                list();
            } else if (memcmp(command, "exit", 4) == 0) {
                printf("Bye\n");
                exit(0);
            } else {
                printf("Invalid command. \n");
            }
        } else {
            exit(0);
        }
    }
}

int main(int argc, char * argv[]){
    memset(files,0,sizeof(files));
    pthread_t thread_accept;
    if (argc != 2) {
        printf("Usage: ./tracker portno\n");
        exit(0);
    }
    pthread_create(&thread_accept,NULL,(void * (*)(void *))accept_thread,(void*)atoi(argv[1]));
    printf("Tracker Process\n");
    printf("Port number: %s\n", argv[1]);
    command();
}
