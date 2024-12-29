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

void init_clock(){ //Inicialización del clock

    pthread_mutex_lock(&clock_mutex);
    cont = 0; //Reinicia el contador
    run = 1; //Activa el reloj
    pthread_mutex_unlock(&clock_mutex);
    printf("Clock inicializado \n");
    pthread_create(&clock_thread, NULL, clock_function, NULL); //Creamos el hilo del clock

}

void stop_clock(){ //Detención del clock

    pthread_mutex_lock(&clock_mutex);
    run = 0; //Detiene el reloj
    pthread_mutex_unlock(&clock_mutex);
    pthread_join(clock_thread,NULL); //Esperamos a que el hilo termine
    
    printf("Clock detenido \n");
    
}

void *clock_function (void *arg){

    while(1){

        pthread_mutex_lock(&clock_mutex);

        if(!run){ //Si run es 0, salimos del bucle

            pthread_mutex_unlock(&clock_mutex);
            break;

        }
        cont++; //Por cada segundo el clock aumenta en 1
        printf("Ciclo %d de clock generado \n", cont);

        pthread_cond_signal(&clock_cond); //Señal que notifica a otros hilos
        pthread_mutex_unlock(&clock_mutex);
        
        sleep(1); //Pausa de 1 segundo
    }

    return NULL;

}

//no es necesario del todo
int get_clock_cycles() {

    pthread_mutex_lock(&clock_mutex);
    int current_cycles = cont; //Obtiene el valor actual del contador
    pthread_mutex_unlock(&clock_mutex);
    return current_cycles;

}