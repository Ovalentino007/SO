#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "clock.h"
#include "timer.h"

pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clock_cond = PTHREAD_COND_INITIALIZER;

int cont = 0;
int run = 0;
pthread_t clock_thread;

void init_clock(){

    pthread_mutex_lock(&clock_mutex);
    cont = 0;
    run = 1;
    pthread_mutex_unlock(&clock_mutex);
    printf("Clock inicializado \n");
    pthread_create(&clock_thread, NULL, clock_function, NULL);

}

void stop_clock(){

    pthread_mutex_lock(&clock_mutex);
    run = 0;
    pthread_mutex_unlock(&clock_mutex);
    pthread_join(clock_thread,NULL);
    printf("Clock detenido \n");
    
}

void *clock_function (void *arg){

    while(1){
        pthread_mutex_lock(&clock_mutex);
        if(!run){
            pthread_mutex_unlock(&clock_mutex);
            break;
        }
        cont++;
        printf("Ciclo %d de clock generado \n", cont);
        pthread_cond_signal(&clock_cond);
        pthread_mutex_unlock(&clock_mutex);
        sleep(1);
    }
    return NULL;

}
//no es necesario del todo
int get_clock_cycles() {
    pthread_mutex_lock(&clock_mutex);
    int current_cycles = cont;
    pthread_mutex_unlock(&clock_mutex);
    return current_cycles;
}