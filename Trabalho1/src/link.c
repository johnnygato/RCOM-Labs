#include "link.h"
#include "alarm.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

LinkLayer ll;

/*
 * OPEN
 */
int llopen(int porta, ConnectionFlag flag) {
    if (flag != CONN_TRANSMITTER && flag != CONN_RECEIVER) {
        printf("FLAG not supported (must be either LL_TRANSMISSER or LL_RECEIVER)\n");
        return -1;
    }

    // Open port
    char port_name[strlen(DEVICE) + 1];
    sprintf(port_name, "%s%d", DEVICE, porta);

    printf("Opening port : '%s'\n", port_name);

    int fd = open(port_name, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(port_name);
        printf("Could not open port\n");
        return -1;
    }

    /*
     * Port configuration
     */
    printf("Starting port configuration...\n");

    if (tcgetattr(fd, &ll.oldtio) == -1) {
        perror("tcgetattr");
        return -1;
    }

    if (!configureTermios(fd, &ll.newtio)) return -1;

    // Transmitter
    if (flag == CONN_TRANSMITTER) {
        linkLayer_constructor(&ll, port_name, 1, 3, CONN_TRANSMITTER);
        return llopen_as_transmitter(fd);
    }
    // Receiver
    else {
        linkLayer_constructor(&ll, port_name, 1, 3, CONN_RECEIVER);
        return llopen_as_receiver(fd);
    }
}

int llopen_as_transmitter(int fd) {
    printf("! Port set as transmitter !\n");

    printf("Establishing connection...\n");

    LLFrame* set = LLFrame_create_command(A_COM_T, C_SET, 0);
    int res = send_with_retransmission(fd, set, C_UA);

    if (res) return fd;

    return -1;
}

int llopen_as_receiver(int fd) {
    printf("! Port set as receiver !\n");

    char c;
    do {
        read(fd, &c, sizeof(char));
    } while (c != FLAG);

    LLFrame* comm = LLFrame_from_fd(fd);

    if (LLFrame_is_command(comm, C_SET)) {
        LLFrame* ua = LLFrame_create_command(A_ANS_R, C_UA, 0);
        write(fd, ua->data.message, ua->data.size);
        LLFrame_delete(&ua);
    }
    else {
        LLFrame_print_msg(comm, "Invalid response: ");
        fd = -1;
    }

    LLFrame_delete(&comm);
    return fd;
}

/*
 * WRITE
 */
int llwrite(int fd, const char* buffer, uint length) {
    return 0;
}

/*
 * READ
 */
int llread(int fd, char* buffer) {
    bool done = false;
    while (!done) {
        char c;
        do {
            read(fd, &c, sizeof(c));
        }while(c != FLAG);

        LLFrame* buf = LLFrame_from_fd(fd);
        if (LLFrame_is_command(buf, C_DISC)) {
            LLFrame* disc = LLFrame_create_command(A_COM_R, C_DISC, 0);
            send_with_retransmission(fd, disc, C_UA);
            LLFrame_delete(&disc);
            done = true;
        }
    }

    return 0;
}

/*
 * CLOSE
 */
int llclose(int fd) {
    if (ll.mode == CONN_TRANSMITTER) {
        printf("Closing connection...\n");

        LLFrame* disc = LLFrame_create_command(A_COM_T, C_DISC, 0);
        int res = send_with_retransmission(fd, disc, C_DISC);
        LLFrame_delete(&disc);

        if (!res) goto close;

        LLFrame* ua = LLFrame_create_command(A_ANS_T, C_UA, 0);
        write(fd, ua->data.message, ua->data.size);
        LLFrame_delete(&ua);
    }

close:
    printf("Restoring old port configuration...\n");
    if (tcsetattr(fd, TCSANOW, &ll.oldtio) == -1 ){
        perror("tcsetattr");
        return -1;
    }

    printf("Closing port...\n");
    printf("============\n");
    printf("Disconnected\n");
    printf("============\n");
    close(fd);
    return 1;
}

int send_with_retransmission(int fd, LLFrame* msg, LL_C answer) {
    uint numTries = 0;
    int res = 0;
    bool done = false;
    (void) signal(SIGALRM, alarm_handler);

    while (!done) {
        if (numTries == 0 || alarmRing) {
            if (numTries > ll.numTransmissions) {
                alarm(0);
                printf("!!!ERROR::RETRANSMISSION - Maximum number of retransmissions exceeded!!!!\n");
                done = true;
                break;
            }
            if (alarmRing)
                printf("!!!ERROR::TIMEOUT - Retrying (%dx)\n", numTries);

            alarmRing = false;
            alarm(ll.timeout);

            write(fd, msg->data.message, msg->data.size);

            ++numTries;
        }

        char c;
        read(fd, &c, sizeof(c));
        if (c == FLAG) {
            LLFrame* response = LLFrame_from_fd(fd);
            if (LLFrame_is_command(response, answer)) {
                done = true;
                res = 1;
            }
            LLFrame_delete(&response);
        }
    }
    alarm(0);

    return res;
}

int configureTermios(int fd, struct termios *t) {
    bzero(t, sizeof(*t));
    t->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    t->c_iflag = IGNPAR;
    t->c_oflag = OPOST;

    t->c_lflag = 0; // non-canonical
    t->c_cc[VTIME]    = 0;   // inter-character timer unused
    t->c_cc[VMIN]     = 0;   // blocking read until 5 chars received

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,t) == -1) {
        perror("tcsetattr");
        return false;
    }

    // Print configuration
    printf("  | Port configurated successfully:\n");
    printf("  |\t"); t->c_lflag ? printf("Canonical\n") : printf("Non-canonical\n");
    printf("  |\tVTIME | %d\n", t->c_cc[VTIME]);
    printf("  |\tVMIN | %d\n", t->c_cc[VMIN]);

    return true;
}