#include "stems.h"
#include "request.h"

sem_t push_c;
sem_t pop_c;
sem_t delete_c;
int p_seq = 0, c_seq = 0, queue_size;

typedef struct {
	int connfd;
	long in_time;
}element;
element *buffer;

// 
// To run:
// 1. Edit config-ws.txt with following contents
//    <port number>
// 2. Run by typing executable file
//    ./server
// Most of the work is done within routines written in request.c
//

void getargs_ws(int *port, int *pool_size)
{
	FILE *fp;

	if ((fp = fopen("config-ws.txt", "r")) == NULL)
		unix_error("config-ws.txt file does not open.");

	fscanf(fp, "%d", port);
	fscanf(fp, "%d", pool_size);
	fscanf(fp, "%d", &queue_size);
	
	fclose(fp);
}

void consumer()
{
	requestHandle(buffer[c_seq].connfd, buffer[c_seq].in_time);
	Close(buffer[c_seq].connfd);
	c_seq++;
	if(queue_size == c_seq)
		c_seq = 0;
}
void producer(int connfd)
{
	buffer[p_seq].connfd = connfd;
	buffer[p_seq].in_time = getWatch();
	p_seq++;
	if(queue_size == p_seq)
		p_seq = 0;
}

void *thread(void *data) 
{
	while(1) {
		sem_wait(&pop_c);
		sem_wait(&delete_c);
		consumer();
		sem_post(&delete_c);
		sem_post(&push_c);
	}
}

int main(void)
{
	int listenfd, connfd, port, clientlen, pool_size, i, res;
	struct sockaddr_in clientaddr;
	pid_t pid;
	char *argv1[] = { NULL };
	
	initWatch();
	getargs_ws(&port, &pool_size);
	
	pid = Fork();
	if (pid == 0) {
		Execve("./pushClient", argv1, argv1);
	}
	res = sem_init(&push_c, 0, queue_size);
	if(res != 0) {
		perror("semaphore initialization failed\n");
		exit(EXIT_FAILURE);
	}
	res = sem_init(&pop_c, 0, 0);
	if(res != 0) {
		perror("semaphore initialization failed\n");
		exit(EXIT_FAILURE);
	}
	res = sem_init(&delete_c, 0, 1);
	if(res != 0) {
		perror("semaphore initialization failed\n");
		exit(EXIT_FAILURE);
	}
	pthread_t worker_threads[pool_size];
	for(i=0; i<pool_size; i++) {
		res = pthread_create(&worker_threads[i], NULL, thread, (void*)&i);
		if(res != 0) {
			perror("thread creation failed\n");
			exit(EXIT_FAILURE);
		}
	}
	buffer = (element*)malloc(sizeof(element)*queue_size);
	
	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
		sem_wait(&push_c);
		producer(connfd);
		sem_post(&pop_c);
	}
	
	for(i=0; i<pool_size; i++) {
		res = pthread_join(worker_threads[i], NULL);
		if(res != 0) {
			perror("thread join failed\n");
			exit(EXIT_FAILURE);
		}
	}
	
	free(buffer);
	sem_destroy(&push_c);
	sem_destroy(&pop_c);
	sem_destroy(&delete_c);
	
	return 0;
}
