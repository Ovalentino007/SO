#include <stdio.h>
#include <pthread.h>
#include "scheduler.h"
#include "clock.h"
#include "timer.h"
#include "structures.h"

#define QUANTUM 4

pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sched_cond = PTHREAD_COND_INITIALIZER;
//int run_scheduler=1;
void *scheduler_function(void *arg){

    ScheduleConfig *config = (ScheduleConfig *)arg;
    ProcessQueue *queue = config->queue;
    Machine *machine = config->machine;

    while(1){

        pthread_mutex_lock(&sched_mutex);
        pthread_cond_wait(&sched_cond,&sched_mutex);
        if (!run_scheduler)
        {
            pthread_mutex_unlock(&sched_mutex);
            break;
        }

        printf("Scheduler activado con politica: %d \n", config->policy);
        if (config->policy != ROUND_ROBIN && config->policy != PRIORITY) {
            printf("Error: política de planificación no válida (%d)\n", config->policy);
            pthread_mutex_unlock(&sched_mutex);
            return NULL;
        }
        switch (config->policy) {
            case ROUND_ROBIN:
                schedule_round_robin(queue, machine);
                break;
            case PRIORITY:
                schedule_priority(queue);
                break;
        }
        pthread_mutex_unlock(&sched_mutex);
    }
    return NULL;

}

void schedule_round_robin(ProcessQueue *queue, Machine *machine){

    pthread_mutex_lock(&queue->lock);
    int found_ready=0;
    for (int i = 0; i < queue->capacidad_max; i++)
    {
        PCB *proceso = &queue->procesos[i];
        if (proceso->state == READY && proceso->pid > 0)
        {
            found_ready=1;
            proceso->state = RUNNING;
            int run_time = (proceso->tiempo_restante < QUANTUM) ? proceso->tiempo_restante : QUANTUM;
            proceso->tiempo_restante -= run_time;
            printf("Ejecutando proceso PID= %d durante Quantum= %d \n", proceso->pid, run_time);
            if (proceso->tiempo_restante <=0)
            {
                proceso->state = TERMINATED;
                printf("Proceso PID= %d terminado \n", proceso->pid);
            }else{
                proceso->state = READY;
            }
            
        }   
    }
    if (!found_ready)
    {
        printf("No hay procesos READY para ejecutar \n");
    }
    
    dispatch_process(queue,machine);
    pthread_mutex_unlock(&queue->lock);
    
}

void schedule_priority(ProcessQueue *queue) {
    pthread_mutex_lock(&queue->lock);
    PCB *highest_priority = NULL;

    for (int i = 0; i < queue->cont; i++) {
        PCB *proceso = &queue->procesos[i];
        if (proceso->state == READY && (!highest_priority || proceso->prioridad > highest_priority->prioridad)) {
            highest_priority = proceso;
        }
    }

    if (highest_priority) {
        highest_priority->state = RUNNING;
        printf("Ejecutando proceso PID=%d con prioridad=%d\n", highest_priority->pid, highest_priority->prioridad);
        highest_priority->state = TERMINATED;
    } else {
        printf("No hay procesos READY para ejecutar.\n");
    }

    pthread_mutex_unlock(&queue->lock);
}

void dispatch_process(ProcessQueue *queue, Machine *machine) {
    pthread_mutex_lock(&queue->lock);
    if (!machine)
    {
        printf("Error: maquina no inicializada. \n");
        return;
    }
    

    for (int i = 0; i < queue->cont; i++) {
        PCB *proceso = &queue->procesos[i];
        if (proceso->state == RUNNING) {
            printf("Despachando proceso PID=%d a un hilo\n", proceso->pid);

            Thread *available_thread = find_available_thread(machine);
            if (available_thread) {
                available_thread->procesos = proceso;
                printf("Proceso PID=%d asignado a un hilo\n", proceso->pid);
            } else {
                printf("No hay hilos disponibles para el proceso PID=%d\n", proceso->pid);
            }
        }
    }

    pthread_mutex_unlock(&queue->lock);
}
Thread *find_available_thread(Machine *machine) {
    for (int i = 0; i < machine->num_cpus; i++) {
        for (int j = 0; j < machine->num_cores; j++) {
            for (int k = 0; k < machine->num_threads; k++) {
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if (!thread->procesos) {  // Hilo libre
                    return thread;
                }
            }
        }
    }
    return NULL;
}

void notify_scheduler(){
    pthread_mutex_lock(&sched_mutex);
    pthread_cond_signal(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);
}
void stop_scheduler(){
    //pthread_mutex_lock(&sched_mutex);
    //run_scheduler=0;
    //pthread_cond_signal(&sched_cond);
    //pthread_mutex_unlock(&sched_mutex);
}