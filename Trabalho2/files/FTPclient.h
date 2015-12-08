#ifndef __FTPclient
#define __FTPclient

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FTP
{
    int socket_fd; // file descriptor to control socket
    int data_fd; // file descriptor to data socket
} ftp;

extern int ftpConnect(ftp* ftp, const char* ip, int port);
extern int ftpLoginHost(ftp* ftp, const char* user, const char* password);
extern int ftpCWD(ftp* ftp, const char* path);
extern int ftpPasv(ftp* ftp);
extern int ftpRetr(ftp* ftp, const char* filename);
extern int ftpDownloadFile(ftp* ftp, const char* filename);
extern int ftpDisconnect(ftp* ftp);

extern int ftpSend(ftp* ftp, const char* str, size_t size);
extern int ftpRead(ftp* ftp, char* str, size_t size);

#endif
