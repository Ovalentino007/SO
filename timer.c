#include <stdio.h>
#include <pthread.h>
#include "timer.h"
#include "scheduler.h"

extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;
extern int run;

void *timer_function(void *arg){
    
    Timerconf *config = (Timerconf*)arg;
    if (!config)
    {
        printf("Error: configuracion de timer no valida \n");
        return NULL;
    }
    
    int frecuencia_tick = config->frecuencia_tick;
    int cont=0;
    int num=0;

    while(1){
        pthread_mutex_lock(&clock_mutex);
        if (!run)
        {
            pthread_mutex_unlock(&clock_mutex);
            printf("Timer detenido \n");
            break;
        }
        pthread_cond_wait(&clock_cond,&clock_mutex);
        cont++;
        if (cont>=frecuencia_tick)
        {
            num++;
            printf("Tick %d generado por el timer. Cada  %d ciclos \n", num, cont);
            notify_scheduler();
            cont=0;
        }
        pthread_mutex_unlock(&clock_mutex);
    }
    return NULL;


}

void stop_timer(){
    pthread_mutex_lock(&clock_mutex);
    run=0;
    pthread_cond_signal(&clock_cond);
    pthread_mutex_unlock(&clock_mutex);
}