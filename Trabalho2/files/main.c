#include <stdio.h>
#include "URLparser.h"
#include "FTPclient.h"

void errorMessage(char* argv0) {
	printf("Usage Anonymous: %s ftp://<host>/<url-path>\n",argv0);
	printf("Usage Normal: %s ftp://[<user>:<password>@]<host>/<url-path>\n",argv0);
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Wrong number of arguments.\n\n");
		errorMessage(argv[0]);
		return 1;
	}

	/* --------------- URL ------------------- */
	url url;
	initURL(&url);

	if (!URLparser(&url, argv[1]))
		return -1;

	if (!getIp(&url)) {
		printf("ERROR: Cannot find ip to hostname %s.\n", url.host);
		return -1;
	}

	printf("User: %s\n", url.user);
	printf("Password: %s\n", url.pass);
	printf("Host: %s\n", url.host);
	printf("Ip: %s\n", url.ip);
	printf("path: %s\n", url.path);
	printf("Filename: %s\n", url.file);

	/*------------------- FTP CLIENT -------------------------*/

	ftp ftp;
	ftpConnect(&ftp, url.ip, url.port);

	// Verifying username
	const char* user;
	if(strlen(url.user) != 0){
		user = url.user;
	}
	else
		user = "anonymous";

	// Verifying password
	char* password;
	if (strlen(url.pass)) {
		password = url.pass;
	} else {
		char buf[100];
		printf("You are now entering in anonymous mode.\n");
		printf("Please insert your college email as password: ");
		while (strlen(fgets(buf, 100, stdin)) < 20)
			printf("\nIncorrect mail, please try again: ");
		password = (char*) malloc(strlen(buf));
		strncat(password, buf, strlen(buf) - 1);
	}

	// Sending credentials to server
	if (ftpLoginHost(&ftp, user, password)) {
		printf("ERROR: Cannot login user %s\n", user);
		return -1;
	}

	// Changing directory
	if (ftpCWD(&ftp, url.path)) {
		printf("ERROR: Cannot change directory to the folder of %s\n",
				url.file);
		return -1;
	}

	// Entry in passive mode
	if (ftpPasv(&ftp)) {
		printf("ERROR: Cannot entry in passive mode\n");
		return -1;
	}

	// Begins transmission of a file from the remote host
	ftpRetr(&ftp, url.file);

	// Starting file transfer
	ftpDownloadFile(&ftp, url.file);

	// Disconnecting from server
	ftpDisconnect(&ftp);

	return 0;
}
