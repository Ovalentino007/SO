#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "proc_generator.h"
#include "structures.h"

extern pthread_mutex_t clock_mutex;
extern ProcessQueue proc_queue;
int run_generator=1;
void *proc_generator_function(void *arg){

    Proc_generator_conf *config = (Proc_generator_conf*)arg;
    int frecuencia_gen = config->frecuencia_gen;
    int p_id = 1;
    while(run_generator){
        
        pthread_mutex_lock(&clock_mutex);
        if (frecuencia_gen > 0)
        {
            PCB new_process;
            new_process.pid = p_id++;
            new_process.state = READY;
            new_process.tiempo_restante = rand() % 100 + 1;
            new_process.prioridad = rand() % 5;
            printf("Nuevo proceso creado con PID = %d, con una vida de = %d  y una prioridad de = %d \n", new_process.pid, new_process.tiempo_restante, new_process.prioridad);
            add_process(&proc_queue, new_process);
        }
        
        pthread_mutex_unlock(&clock_mutex);
        sleep(frecuencia_gen);
    }

    printf("Generador de procesos detenido \n");
    return NULL;


}
void stop_proc_generator(){
    run_generator=0;
}