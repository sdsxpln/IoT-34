#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/time.h>
#include "stems.h"

void clientSend(int fd, char *filename, char *body)
{
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	/* Form and send the HTTP request */
	sprintf(buf, "POST %s HTTP/1.1\n", filename);
	sprintf(buf, "%sHost: %s\n", buf, hostname);
	sprintf(buf, "%sContent-Type: text/plain; charset=utf-8\n", buf);
	sprintf(buf, "%sContent-Length: %d\n\r\n", buf, (int)strlen(body));
	sprintf(buf, "%s%s\n", buf, body);
	Rio_writen(fd, buf, strlen(buf));
}

/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
	rio_t rio;
	char buf[MAXBUF];
	int length = 0;
	int n;
	
	Rio_readinitb(&rio, fd);
	
	/* Read and display the HTTP Header */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (strcmp(buf, "\r\n") && (n > 0)) {
		/* If you want to look for certain HTTP tags... */
		if (sscanf(buf, "Content-Length: %d ", &length) == 1)
			printf("Length = %d\n", length);
		printf("Header: %s", buf);
		n = Rio_readlineb(&rio, buf, MAXBUF);
	}

	/* Read and display the HTTP Body */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (n > 0) {
		printf("%s", buf);
		n = Rio_readlineb(&rio, buf, MAXBUF);
	}
}

/* currently, there is no loop. I will add loop later */
void userTask(char *hostname, int port, char *filename, char *msg)
{
	int clientfd;

	clientfd = Open_clientfd(hostname, port);
	clientSend(clientfd, filename, msg);
	clientPrint(clientfd);
	Close(clientfd);
}

void getargs_pc(char *hostname, int *port, char *filename, float *threshold)
{
	FILE *fp;

	fp = fopen("config-pc.txt", "r");
	if (fp == NULL)
		unix_error("config-pc.txt file does not open.");

	fscanf(fp, "%s", hostname);
	fscanf(fp, "%d", port);
	fscanf(fp, "%s", filename);
	fscanf(fp, "%f", threshold);
	fclose(fp);
}

int main(void)
{
	char hostname[MAXLINE], filename[MAXLINE];
	int port;
	float threshold;
	float data = 19;
	char data_str[255];
	char msg_[255];
	getargs_pc(hostname, &port, filename, &threshold);
	
	int np_fd;
	int sizearg;
	char np_arg[255] = {0};
	if(-1 == mkfifo("/tmp/namedpipe", 0666))	 {
		perror("mkpipe error : ");
		exit(1);
	}
	if((np_fd = open("/tmp/namedpipe", O_RDWR)) == -1)  {
		perror("open error : ");
		exit(1);
	}
		
	while(1) {
		if(read(np_fd, &sizearg, sizeof(sizearg)) > 0) {
			if(read(np_fd, np_arg, sizearg) > 0) {
				strcpy(msg_, np_arg);
				strtok(np_arg, "=");
				strtok(NULL, "=");
				strtok(NULL, "=");
				strcpy(data_str, strtok(NULL, "\n"));
				data = atof(data_str);
				if (strstr(msg_, "temperature") && data > threshold) 
					userTask(hostname, port, filename, msg_);
			}
		}
	}
	
	close(np_fd);
	
	return(0);
}
