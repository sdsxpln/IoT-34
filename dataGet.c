#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <mysql/mysql.h>

#define LIST 1
#define INFO 2
#define GET 3

//
// This program is intended to help you test your web server.
// You can use it to test that you are correctly having multiple 
// threads handling http requests.
//
// htmlReturn() is used if client program is a general web client
// program like Google Chrome. textReturn() is used for a client
// program in a embedded system.
//
// Standalone test:
// # export QUERY_STRING="name=temperature&time=3003.2&value=33.0"
// # ./dataGet.cgi

void htmlReturn(char *body)
{
	char content[MAXLINE];
	char *buf;
	char *ptr;
	
	/* Make the response body */
	sprintf(content, "%s<html>\r\n<head>\r\n", content);
	sprintf(content, "%s<title>CGI test result</title>\r\n", content);
	sprintf(content, "%s</head>\r\n", content);
	sprintf(content, "%s<body>\r\n", content);
	sprintf(content, "%s<h2>Welcome to the CGI program</h2>\r\n", content);
	buf = getenv("QUERY_STRING");
	sprintf(content, "%s<p>Env : %s</p>\r\n", content, buf);
	ptr = strsep(&buf, "&");
	while (ptr != NULL){
		sprintf(content, "%s%s\r\n", content, ptr);
		ptr = strsep(&buf, "&");
	}
	ptr = strsep(&body, "&"); 
	while (ptr != NULL){
		sprintf(content, "%s%s\r\n", content, ptr);
		ptr = strsep(&body, "&");
	}
	sprintf(content, "%s</body>\r\n</html>\r\n", content);
	
	/* Generate the HTTP response */
	printf("Content-Length: %d\r\n", (int)strlen(content));
	printf("Content-Type: text/html\r\n\r\n");
	printf("%s", content);
	fflush(stdout);
}

void textReturn(void)
{
	char content[MAXLINE];
	char *buf;
	char *ptr;

	buf = getenv("QUERY_STRING");
	sprintf(content, "%sEnv : %s\n", content, buf);
	ptr = strsep(&buf, "&");
	while (ptr != NULL){
		sprintf(content, "%s%s\n", content, ptr);
		ptr = strsep(&buf, "&");
	}

	/* Generate the HTTP response */
	printf("Content-Length: %d\n", (int)strlen(content));
	printf("Content-Type: text/plain\r\n\r\n");
	printf("%s", content);
	fflush(stdout);
}

int SelectQuery(const char *qString, int query_num, char* body) 
{
	MYSQL		*pConnection = NULL;
	MYSQL		conn;
	MYSQL_RES	*qresult;
	MYSQL_ROW	qrow;
	
	char query[1024];
	int qstat;

	mysql_init(&conn);
	pConnection = mysql_real_connect(&conn, "127.0.0.1", "RPI", "raspberry", "RPI_DB", 3306, (char*)NULL, 0);
	
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
	qresult = mysql_store_result(pConnection);
	
	sprintf(body, "-------------------- CLI Programming ---------------------&"); 
	if(query_num == LIST) {
		while((qrow=mysql_fetch_row(qresult)) != NULL) 
			sprintf(body, "%s%s&", body, qrow[0]);
	}
	else if(query_num == INFO) {
		sprintf(body, "%scount\t\tmin\t\tave\t\tmax&", body);
		qrow=mysql_fetch_row(qresult);
		sprintf(body, "%s%-10.10s\t%-10.10s\t%-10.10s\t%-10.10s&", body, qrow[0], qrow[1], qrow[2], qrow[3]);
	}
	else if(query_num == GET) {
		sprintf(body, "%stime\t\tdata&", body);
		while((qrow=mysql_fetch_row(qresult)) != NULL)
			sprintf(body, "%s%-10.10s\t%-10.10s&", body, qrow[0], qrow[1]);
	}
	sprintf(body, "%s----------------------------------------------------------&", body);
	
	mysql_free_result(qresult);
	mysql_close(pConnection);
	return 0;
}

int main(void)
{
	char *arg_ = getenv("QUERY_STRING");
	char arg[MAXBUF];
	strcpy(arg, arg_);
	char cmd[MAXBUF], sname[MAXBUF], count_str[MAXBUF];
	int count, query_num, arg_num;
	if (!strstr(arg, "count")) {
		arg_num = 2;
		count = 0;
		if (!strstr(arg, "sname")) {
			arg_num = 1;
			strcpy(sname, "");
		}
	}
	else {
		arg_num = 3;
	}
	strcpy(cmd, strtok(arg, "&"));
	if(arg_num > 1) {
		strcpy(sname, strtok(NULL, "&"));
		if(arg_num > 2) {
			strcpy(count_str, strtok(NULL, "&"));
			strtok(count_str, "=");
			strcpy(count_str, strtok(NULL, "\n"));
			count = atoi(count_str);
		}
		strtok(sname, "=");
		strcpy(sname, strtok(NULL, "\n"));
	}
	strtok(cmd, "=");
	strcpy(cmd, strtok(NULL, "\n"));
		
	char tablenum_str[3] = "";
	if(strcmp(sname, "temperature") == 0) strcpy(tablenum_str, "01");
	else if(strcmp(sname, "humidity") == 0) strcpy(tablenum_str, "02");
	
	char query1[1024];
	if(strcmp(cmd, "LIST") == 0 ) {
		sprintf(query1, "select name from sensorList");
		query_num = LIST;
	}
	else if(strcmp(cmd, "INFO") == 0 ) {
		sprintf(query1, "select count, min, ave, max from sensorList where name='%s'", sname);
		query_num = INFO;	
	}
	else if(strcmp(cmd, "GET") == 0 && count == 0) {
		sprintf(query1, "select sData%s.time, sData%s.data from sensorList sList, sensorData%s sData%s where sList.count=sData%s.seq_num and sList.name='%s'",
																						tablenum_str, tablenum_str, tablenum_str, tablenum_str, tablenum_str, sname);
		query_num = GET;
	}
	else if(strcmp(cmd, "GET") == 0 && count != 0) {
		sprintf(query1, "select sData%s.time, sData%s.data from sensorList sList, sensorData%s sData%s where sList.name='%s' and sList.count-%d < sData%s.seq_num", 
																						tablenum_str, tablenum_str, tablenum_str, tablenum_str, sname, count, tablenum_str);
		query_num = GET;
	}

	char body[MAXBUF];
	SelectQuery(query1, query_num, body);
	
	htmlReturn(body);
	//textReturn();
	
	return(0);
}
