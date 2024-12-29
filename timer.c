#include <stdio.h>
#include <pthread.h>
#include "timer.h"
#include "scheduler.h"

extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;
extern int run;

void *timer_function(void *arg){ //Función que ejecutará un hilo dedicado a simular el temporizador
    
    Timerconf *config = (Timerconf*)arg;

    if (!config)
    {
        printf("Error: configuracion de timer no valida \n");
        return NULL;
    }
    
    int frecuencia_tick = config->frecuencia_tick; //Frecuencia en ciclos en el que se generará un tick
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

        pthread_cond_wait(&clock_cond,&clock_mutex); //El hilo del temporizador queda en espera de recibir una señal
        cont++;
        
        if (cont>=frecuencia_tick) //Generación del tick cuando alcanza la frecuencia establecida
        {
            num++;
            printf("Tick %d generado por el timer. Cada  %d ciclos \n", num, cont);
            notify_scheduler(); //Mandamos señal al scheduler
            cont=0;
        }
        
        pthread_mutex_unlock(&clock_mutex);
    }
    
    return NULL;

}

void stop_timer(){ //Detención del temporizador notificando al hilo que lo simula

    pthread_mutex_lock(&clock_mutex);
    run=0;
    pthread_cond_signal(&clock_cond);
    pthread_mutex_unlock(&clock_mutex);

}