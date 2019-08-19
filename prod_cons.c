#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject {
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	int full;
} so_t;

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) {
			so->full = 1;
			so->line = NULL;
			break;
		}
		so->linenum = i;
		so->line = strdup(line);      /* share the line */
		i++;
		so->full = 1;
	}
	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;

	while (1) {
		line = so->line;
		if (line == NULL) {
			break;
		}
		len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		free(so->line);
		i++;
		so->full = 0;
	}
	printf("Cons: %d lines\n", i);
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int *ret;
	int i;
	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}
	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	rfile = fopen((char *) argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
	if (argv[2] != NULL) {
		Nprod = atoi(argv[2]);
		if (Nprod > 100) Nprod = 100;
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) {
		Ncons = atoi(argv[3]);
		if (Ncons > 100) Ncons = 100;
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;

	share->rfile = rfile;
	share->line = NULL;
	pthread_mutex_init(&share->lock, NULL);
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	pthread_exit(NULL);
	exit(0);
}

