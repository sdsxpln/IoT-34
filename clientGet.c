/*
 * clientGet.c: A very, very primitive HTTP client for console.
 * 
 * To run, prepare config-cg.txt and try: 
 *      ./clientGet
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * For testing your server, you will want to modify this client.  
 *
 * When we test your server, we will be using modifications to this client.
 *
 */


#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include "stems.h"

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
  char buf[MAXLINE];
  char hostname[MAXLINE];

  Gethostname(hostname, MAXLINE);

  /* Form and send the HTTP request */
  sprintf(buf, "GET %s HTTP/1.1\n", filename);
  sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
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
    printf("Header: %s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);

    /* If you want to look for certain HTTP tags... */
    if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
      printf("Length = %d\n", length);
    }
  }

  /* Read and display the HTTP Body */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (n > 0) {
    printf("%s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }
}

/* currently, there is no loop. I will add loop later */
void userTask(char hostname[], int port, char webaddr[], pid_t pid)
{
	
	char cmd[5][15] = { "LIST", "INFO", "GET", "quit", "exit"};
	char filename[1024];
	int clientfd;
	char cmdLine[1024]; // 입력 명령 전체를 저장하는 배열
	char *cmdTokens[128]; // 입력 명령을 공백으로 분리하여 저장하는 배열
	char delim[] = " \t\n\r"; // 토큰 구분자 - strtok에서 사용
	char *token; // 하나의 토큰을 분리하는데 사용
	int tokenNum; // 입력 명령에 저장된 토큰
	int i;
	strcpy(filename, webaddr);
	
	while(1) {
		printf("# ");
		fgets(cmdLine, 1024, stdin);
		
		if(cmdLine[0] == '\n' || cmdLine[0] == ' ' || cmdLine[0] == '\t' )
			continue;

		tokenNum = 0;
		token = strtok(cmdLine, delim); 
		while (token) { 						// 문자열이 있을 동안 반복
			cmdTokens[tokenNum++] = token; 	// 분리된 문자열을 배열에 저장
			token = strtok(NULL, delim); 	// 연속하여 입력 명령의 문자열 하나 분리
		}
		cmdTokens[tokenNum] = NULL;

		for (i = 0; i < 5; i++) { 				// 내장 명령인지 검사하여 명령이 있으면 해당 함수 호출
			if (strcmp(cmdTokens[0], cmd[i]) == 0) 
				break;
		}
		
		if(i==0) 
			sprintf(webaddr, "%scommand=LIST", filename );
		else if(i==1)
			sprintf(webaddr, "%scommand=INFO&sname=%s", filename, cmdTokens[1]);
		else if(i==2)
			if(tokenNum == 2)
				sprintf(webaddr, "%scommand=GET&sname=%s&count=1", filename, cmdTokens[1]);
			else
				sprintf(webaddr, "%scommand=GET&sname=%s&count=%s", filename, cmdTokens[1], cmdTokens[2]);
		else if(i==3 || i==4) {
			printf("Bye\r\n");
			kill(pid, SIGKILL);
			return ;
		}
		else {
			printf("command not exist\r\n");
			continue;
		}
		clientfd = Open_clientfd(hostname, port);
		clientSend(clientfd, webaddr);
		clientPrint(clientfd);
		Close(clientfd);
	}

}

void getargs_cg(char hostname[], int *port, char webaddr[])
{
  FILE *fp;

  fp = fopen("config-cg.txt", "r");
  if (fp == NULL)
    unix_error("config-cg.txt file does not open.");

  fscanf(fp, "%s", hostname);
  fscanf(fp, "%d", port);
  fscanf(fp, "%s", webaddr);
  fclose(fp);
}

int main(void)
{
	char hostname[MAXLINE], webaddr[MAXLINE];
	int port;
	pid_t pid;
	char *argv1[] = { NULL };

	pid = Fork();
	if (pid == 0) {
		Execve("./pushServer", argv1, argv1);
	}
	
	getargs_cg(hostname, &port, webaddr);
	userTask(hostname, port, webaddr, pid);
  
  return(0);
}
