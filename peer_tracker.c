#include "peer.h"

int tracker_reg(struct in_addr tip, in_port_t tport, in_port_t listen_port) {
    char msg[24];
    struct sockaddr_in tracker;
    int tmplen = sizeof(tracker);
    char localip[16];
    int sockfd;
    unsigned int argl, tmp;
    unsigned short port;

    fprintf(stderr, "== Peer register ==\n");
    fprintf(stderr, "\tConnection\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[REG] socket()");
        return -1;
    }
    
    memset(&tracker, sizeof(tracker), 0);
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tip;
    tracker.sin_port = htons(tport);
   
    inet_ntop(AF_INET, &tip, localip, 16); 
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("[REG] connect()");
        close(sockfd);
        return -1;
    }

    fprintf(stderr, "\tCompose the message\n");
    msg[0] = 0x01;
    msg[1] = 3;

    argl = htonl(4);
    memcpy(msg + 2, &argl, 4);
    memcpy(msg + 2 + 4, &tracker.sin_addr, 4);
    
    argl = htonl(2);
    port = htons(listen_port);
    memcpy(msg + 2 + 4 + 4, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4, &port, 2);
    
    argl = htonl(4);
    tmp = htonl(fileID);
    memcpy(msg + 2 + 4 + 4 + 4 + 2, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4 + 2 + 4, &tmp, 4);

    fprintf(stderr, "\tSent the message\n");
    write(sockfd, msg, 24);
    
    fprintf(stderr, "\tReceive the return\n");
    if (read(sockfd, msg, 2) != 2) {
        puts("No response ...");
        close(sockfd);
        return -1;
    }
    if (msg[1]) {
        puts("Wrong type response ... Check the tracker. b");
        close(sockfd);
        return -1;
    }

    switch (msg[0]) {
        case 0x11: 
            puts("Peer has registered to tracker successfully. ");
            close(sockfd);
            return 0;
        case 0x21: 
            puts("Peer could not be registered to tracker. ");
            break;
        default: 
            puts("Wrong type response ... Check the tracker. c");
    }

    fprintf(stderr, "\tClose the socket\n");
    close(sockfd);
    return -1;
}

int tracker_unreg(struct in_addr tip, in_port_t tport, in_port_t listen_port) {
    char msg[24];
    struct sockaddr_in tracker;
    int tmplen = sizeof(tracker);
    char localip[16];
    int sockfd;
    unsigned int argl, tmp;
    unsigned short port;

    fprintf(stderr, "-- Peer unregister --\n");
    fprintf(stderr, "\tConnection\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[UNREG] socket()");
        return -1;
    }
    
    memset(&tracker, sizeof(tracker), 0);
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tip;
    tracker.sin_port = htons(tport);
   
    inet_ntop(AF_INET, &tip, localip, 16); 
    printf("IP = %s : %d\n", localip, tport);
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("[UNREG] connect()");
        close(sockfd);
        return -1;
    }

    fprintf(stderr, "\tCompose the message\n");
    msg[0] = 0x03;
    msg[1] = 3;

    argl = htonl(4);
    memcpy(msg + 2, &argl, 4);
    memcpy(msg + 2 + 4, &tracker.sin_addr, 4);
    
    argl = htonl(2);
    port = htons(listen_port);
    memcpy(msg + 2 + 4 + 4, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4, &port, 2);
    
    argl = htonl(4);
    tmp = htonl(fileID);
    memcpy(msg + 2 + 4 + 4 + 4 + 2, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4 + 2 + 4, &tmp, 4);

    fprintf(stderr, "\tSent the message\n");
    write(sockfd, msg, 24);
    
    fprintf(stderr, "\tReceive the return\n");
    if (read(sockfd, msg, 2) != 2) {
        puts("No response ...");
        close(sockfd);
        return -1;
    }
    if (msg[1]) {
        puts("Wrong type response ... Check the tracker. b");
        close(sockfd);
        return -1;
    }

    switch (msg[0]) {
        case 0x13: 
            puts("Peer has unregistered from tracker. ");
            close(sockfd);
            return 0;
        case 0x23: 
            puts("Peer could not be unregistered from tracker. ");
            break;
        default: 
            puts("Wrong type response ... Check the tracker. c");
    }

    fprintf(stderr, "\tClose the socket\n");
    close(sockfd);
    return -1;
}

int tracker_list(struct in_addr tip, in_port_t tport) {
    char msg[10];
    int i, sockfd, n;
    struct sockaddr_in tracker;
    char localip[16];
    unsigned int tmp;
    unsigned int *ips;
    unsigned short *ports;

    puts("-- Tracker List Request --");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[LIST] socket()");
        return -1;
    }
    
    memset(&tracker, sizeof(tracker), 0);
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tip;
    tracker.sin_port = htons(tport);
   
    inet_ntop(AF_INET, &tip, localip, 16); 
    printf("IP = %s : %d\n", localip, tport);
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("[LIST] connect()");
        close(sockfd);
        return -1;
    }

    msg[0] = 0x04;
    msg[1] = 1;
    tmp = htonl(4);
    memcpy(msg + 2, &tmp, 4);
    tmp = htonl(fileID);
    memcpy(msg + 2 + 4, &tmp, 4);
    write(sockfd, msg, 10);

    read(sockfd, msg, 2);
    if (msg[0] != 0x14) {
        puts("Wrong type response ... Check the tracker. ");
        close(sockfd);
        return -1;
    }
    n = msg[1];
    printf("Number of registered peer = %d\n", n);

    ips = malloc(sizeof(unsigned int) * n);
    ports = malloc(sizeof(unsigned short) * n);

    for (i = 0; i < n; ++i) {
        unsigned int tmp;
        char ip[16];

        read(sockfd, &tmp, 4);
        tmp = ntohl(tmp);
        if (tmp != 6) {
            printf("tmp = %x\n", tmp);
            puts("Wrong format in message length ... ");
            return -1;
        }
        read(sockfd, ips + i, 4);
        read(sockfd, ports + i, 2);
       
        ports[i] = ntohs(ports[i]);
        inet_ntop(AF_INET, ips + i, ip, 16);
        printf("[%d] %s : %d\n", i, ip, ports[i]); 
    }

    close(sockfd);

    return 0;
}

void test_reply(int sockfd) {
    char msg[2];
    
    fprintf(stderr, "Reply test message to tracker\n"); 
    msg[0] = 0x12;
    msg[1] = 0;

    write(sockfd, msg, 2);

    return;
}