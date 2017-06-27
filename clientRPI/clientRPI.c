#include <wiringPi.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "stems.h"

#define MAXTIMINGS 83
#define DHTPIN 7
int dht11_dat[5] = {0, } ;
struct timeval startTime;

void initWatch(void)
{
	gettimeofday(&startTime, NULL);
}

double getWatch(void)
{
	struct timeval curTime;
	double tmp;

	gettimeofday(&curTime, NULL);
	tmp = (curTime.tv_sec - startTime.tv_sec) * 1000.0;
	return (tmp + (curTime.tv_usec - startTime.tv_usec) / 1000.0);
}

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
void userTask(char *myname, char *hostname, int port, char *filename, float time, float value)
{
	int clientfd;
	char msg[MAXLINE];
	char filemsg[256];

	sprintf(msg, "name=%s&time=%f&value=%f", myname, time, value);
	clientfd = Open_clientfd(hostname, port);
	clientSend(clientfd, filename, msg);
	clientPrint(clientfd);
	Close(clientfd);
	
}

void getargs_pi(char *hostname, int *port, char *filename, int *period)
{
	FILE *fp;

	fp = fopen("config-pi.txt", "r");
	if (fp == NULL)
		unix_error("config-pi.txt file does not open.");

	fscanf(fp, "%s", hostname);
	fscanf(fp, "%d", port);
	fscanf(fp, "%s", filename);
	fscanf(fp, "%d", period);
	fclose(fp);
}
int read_dht11_dat(char *humidity, char *temperature)
{
	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t j = 0, i;
	
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
	pinMode(DHTPIN, OUTPUT);
	digitalWrite(DHTPIN, LOW);
	delay(18);
	digitalWrite(DHTPIN, HIGH);
	delayMicroseconds(30);
	pinMode(DHTPIN, INPUT);
	for (i = 0; i < MAXTIMINGS; i++) {
		counter = 0;
		while ( digitalRead(DHTPIN) == laststate) {
			counter++;
			delayMicroseconds(1);
			if (counter == 200) 
				break;
		}
		laststate = digitalRead(DHTPIN);
		if (counter == 200)
			break;
		if ((i >= 4) && (i % 2 == 0)) {
			dht11_dat[j / 8] <<= 1;
			if (counter > 20) dht11_dat[j / 8] |= 1;
			j++;
		}
	}
	if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xff))) {
		printf("humidity = %d.%d %% Temperature = %d.%d *C \n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
		sprintf(humidity, "%d.%d", dht11_dat[0], dht11_dat[1]);
		sprintf(temperature, "%d.%d", dht11_dat[2], dht11_dat[3]);
		return 1;
	}
	else {
		printf("Data get failed\n");
		return 0;
	}
}

int main(void)
{
	char myname[MAXLINE] = {"", }, hostname[MAXLINE], filename[MAXLINE];
	int port, period, pass;
	char humidity[MAXBUF], temperature[MAXBUF];
	
	initWatch();
	if (wiringPiSetup() == -1)
		exit(1);
	getargs_pi(hostname, &port, filename, &period);
	
	while(1) {
		pass = read_dht11_dat(humidity, temperature);
		if(pass > 0) {
			strcpy(myname, "humidity");
			userTask(myname, hostname, port, filename, getWatch(), atof(humidity));
			strcpy(myname, "temperature");
			userTask(myname, hostname, port, filename, getWatch(), atof(temperature));
			sleep(period);
		}
		delay(1000);
	}
	return(0);
}
