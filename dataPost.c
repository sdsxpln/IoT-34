#include "stems.h"
#include "request.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <mysql/mysql.h>

//
// This program is intended to help you test your web server.
// 

int InsertQuery(const char *q);

int main(int argc, char *argv[])
{
	int bodyLength;
	char *method = getenv("REQUEST_METHOD");
	char arg[255];
	
	if (strcmp(method, "POST") == 0) {
		bodyLength = atoi(getenv("CONTENT_LENGTH"));
		if(bodyLength > 0) 
			read(STDIN_FILENO, arg, bodyLength + 1);
	}
	
	int np_fd;
	if((np_fd = open("/tmp/namedpipe", O_RDWR)) == -1)  {
		perror("open error : ");
		exit(1);
	}
	
	if(write(np_fd, &bodyLength, sizeof(int)) < 0) {
		perror("sizewrite error : ");
		exit(1);
	}
	if(write(np_fd, arg, bodyLength) < 0 ) {
		perror("write error");
		exit(1);
	}
	close(np_fd);
	
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
	
	int tablenum = 100;
	char tablenum_str[3] = "";
	if(strcmp(name, "temperature") == 0) {
		tablenum = 1;
		strcpy(tablenum_str, "01");
	}
	else if(strcmp(name, "humidity") == 0) {
		tablenum = 2;
		strcpy(tablenum_str, "02");
	}
	
	char q1[1024], q2[1024];
	if(tablenum < 100) 
		sprintf(q1, "insert into sensorData%s(time, data) values (%f, %f);", tablenum_str, time, value);	
	else 
		printf("name error\n");
	
	sprintf(q2, "insert into sensorList(name, count, sum, min, ave, max) values('%s', 1, %f, %f, %f, %f) on duplicate key update name='%s', count=count+1, sum=sum+%f, min=least(min,%f), ave=sum/count, max=greatest(max,%f)", name, value, value, value, value, name, value, value, value) ;
	
	InsertQuery(q1);
	InsertQuery(q2);
	
	printf("HTTP/1.0 200 OK\r\n");
	printf("Server: My Web Server\r\n");
	printf("Content-Length: %d\r\n", bodyLength);
	printf("Content-Type: text/plain\r\n\r\n");
	fflush(stdout);
	
	return(0);
}

int InsertQuery(const char *qString)
{
	MYSQL	*pConnection = NULL;
	MYSQL	conn;
	char query[1024];
	int qstat;

	mysql_init(&conn);
	pConnection = mysql_real_connect(
		&conn, "127.0.0.1", "RPI", "raspberry", "RPI_DB", 3306, (char*)NULL, 0);
	
	if(pConnection == NULL)	{
		printf("ERROR : fail to connect\n");
		return 0;
	}

	strcpy(query, qString);
	qstat = mysql_query(pConnection, query);

	if(qstat != 0) {
		printf("qstat error\n");
		return 0;
	}

	mysql_close(pConnection);
	
	return 0;
}
