#include "link.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s [PORT NUMBER]\n", argv[0]);
        return -1;
    }
    int port = atoi(argv[1]);
    int fd = -1;
    fd = llopen(port, CONN_TRANSMITTER);
    if (fd > 0) {
        printf("======================\n");
        printf("Connection established\n");
        printf("======================\n");
        char* test = "fffaaafff";
        printf("Sending message: %s\n", test);
        llwrite(fd, test, strlen(test));

        char* test2 = "123123";
        printf("Sending message: %s\n", test2);
        llwrite(fd, test2, strlen(test2));

        llclose(fd);
    }
    else printf("Not successful\n");

    return 0;
}
