#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

FILE * fp = NULL;
pthread_mutex_t fp_lock = PTHREAD_MUTEX_INITIALIZER;

extern void _final_() {
	pthread_mutex_lock(&fp_lock);
		//fprintf(fp, "---------------------From _final_---------------------\n");
		fclose(fp);
	pthread_mutex_unlock(&fp_lock);
}

extern void _init_() {
	pthread_mutex_lock(&fp_lock);
		fp = fopen("log", "w");
		//fprintf(fp, "---------------------From _init_---------------------\n");
	pthread_mutex_unlock(&fp_lock);

	atexit(_final_);
}

extern void _probe_(int line, char *file, char *op, void * addr) {
	pthread_mutex_lock(&fp_lock);
		fprintf(fp, "%p,%d,%s,%p,%s\n", (void *) pthread_self(), line, file, addr, op);
		//fprintf(fp, "Thread ID: %p | Line: %6d | Variable '%20s' | Address: %20p | Operation: %s\n", (void *) pthread_self(), line, file, addr, op);
	pthread_mutex_unlock(&fp_lock);
}

extern void _probe_lock_(int line, char *file, char* op, void * addr) {
	pthread_mutex_lock(&fp_lock);
		fprintf(fp, "%p,%d,%s,%p,%s\n", (void *) pthread_self(), line, file, addr, op);
		//fprintf(fp, "Thread ID: %p | Line: %6d | Variable '%20s' | Address: %20p | Operation: %s\n", (void *) pthread_self(), line, file, addr, op);
	pthread_mutex_unlock(&fp_lock);
}

