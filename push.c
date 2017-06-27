#include "stems.h"
#include "request.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

int main(void)
{
	char arg[255];
	int bodyLength = atoi(getenv("CONTENT_LENGTH"));
	if(bodyLength > 0) 
		read(STDIN_FILENO, arg, bodyLength +1);
	setenv("QUERY_STRING", arg, 1);
	char name[255], time_str[255], value_str[255];
	double time, value;
	
	strcpy(name, strtok(arg, "&"));
	strcpy(time_str, strtok(NULL, "&"));
	strcpy(value_str, strtok(NULL, "&"));
	strtok(name, "=");
	strcpy(name, strtok(NULL, "\n"));
	strtok(time_str, "=");
	strcpy(time_str, strtok(NULL, "\n"));
	strtok(value_str, "=");
	strcpy(value_str, strtok(NULL, "\n"));
	time = atof(time_str);
	value = atof(value_str);
	
	fprintf(stderr, "[WARNING!] %s sensor, time : %f, value : %f \r\n", name, time, value);
	
	printf("HTTP/1.0 200 OK\r\n");
	printf("Server: My Web Server\r\n");
	printf("Content-Length: %d\r\n", bodyLength);
	printf("Content-Type: text/plain\r\n\r\n");
	fflush(stdout);
	
	return(0);
}
