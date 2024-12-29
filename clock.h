#ifndef clock_h
#define clock_h
#include <pthread.h>

extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;

void *clock_function(void *arg);
void init_clock();
void stop_clock();
int get_clock_cycles();

#endif 