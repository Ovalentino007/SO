#include <stdio.h>
#include <pthread.h>
#include "scheduler.h"
#include "clock.h"
#include "timer.h"
#include "structures.h"



pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sched_cond = PTHREAD_COND_INITIALIZER;
int run_scheduler=1;
void *scheduler_function(void *arg){

    ScheduleConfig *config = (ScheduleConfig *)arg;
    ProcessQueue *queue = config->queue;
    Machine *machine = config->machine;

    while(run_scheduler){

        pthread_mutex_lock(&sched_mutex);
        pthread_cond_wait(&sched_cond,&sched_mutex);
        if (!run_scheduler)
        {
            pthread_mutex_unlock(&sched_mutex);
            break;
        }
        if (config->policy == ROUND_ROBIN)
        {
            printf("Scheduler activado con politica: Round Robin \n");
        }else if (config->policy == PRIORITY)
        {
            printf("Scheduler activado con politica: Priority \n");
        }
        
        if (config->policy != ROUND_ROBIN && config->policy != PRIORITY) {
            printf("Error: política de planificación no válida (%d)\n", config->policy);
            pthread_mutex_unlock(&sched_mutex);
            return NULL;
        }
        switch (config->policy) {
            case ROUND_ROBIN:
                schedule_round_robin(queue, machine,config->quantum);
                break;
            case PRIORITY:
                schedule_priority(queue, machine);
                break;
        }
        pthread_mutex_unlock(&sched_mutex);
    }
    return NULL;

}

void schedule_round_robin(ProcessQueue *queue, Machine *machine, int quantum){

    pthread_mutex_lock(&queue->lock);
    int procesos_ejecutados=0;
    
    for (int i = 0; i < queue->capacidad_max; i++)
    {
        PCB *proceso = &queue->procesos[i];
        if (proceso->state == READY && proceso->pid > 0)
        {
            proceso->state = RUNNING;
            procesos_ejecutados++;
        }
    }
    if (procesos_ejecutados > 0)
    {
        printf("Scheduler seleccionó %d procesos para ejecutar \n", procesos_ejecutados);
        pthread_mutex_unlock(&queue->lock);
        dispatch_process_rr(queue,machine,quantum);
        pthread_mutex_lock(&queue->lock);
        for (int i = 0; i < queue->cont; ) {
            if (queue->procesos[i].state == TERMINATED) {
                // Reestructurar el arreglo
                for (int j = i; j < queue->cont - 1; j++) {
                    queue->procesos[j] = queue->procesos[j + 1];
                }
                queue->cont--; // Reducir el contador de procesos
            } else {
                i++; // Solo avanzar si no eliminamos un proceso
            }
        }
        pthread_mutex_unlock(&queue->lock);

    }else{
        printf("No hay procesos READY para ejecutar \n");
        pthread_mutex_unlock(&queue->lock);
    }
    
        
}

void dispatch_process_rr(ProcessQueue *queue, Machine *machine, int quantum){
    
    if (!machine)
    {
        printf("Error: maquina no inicializada. \n");
        return;
    }
    pthread_mutex_lock(&queue->lock);
    for (int i = 0; i < queue->capacidad_max; i++) {
        PCB *proceso = &queue->procesos[i];
        if (proceso->state == RUNNING) {
            
            Thread *available_thread = find_available_thread(machine);
            if (available_thread) {
                available_thread->procesos = proceso;
                int run_time = (proceso->tiempo_restante < quantum) ? proceso->tiempo_restante : quantum;
                proceso->tiempo_restante -= run_time;
                printf("Despachando proceso PID=%d a un hilo\n", proceso->pid);
                printf("Ejecutando proceso PID= %d durante Quantum= %d \n", proceso->pid, quantum);
                if (proceso->tiempo_restante <=0)
                {
                    proceso->state = TERMINATED;
                    printf("Proceso PID= %d terminado \n", proceso->pid);

                }else{

                    proceso->state = READY;
                    printf("Proceso PID=%d se reprograma con tiempo restante= %d \n", proceso->pid, proceso->tiempo_restante);
                }
                available_thread->procesos=NULL; //liberar el hilo despues de ejecución
            }else{
                printf("No hay hilos disponibles para el proceso PID=%d \n", proceso->pid);
            }  
        }
    
    }
    pthread_mutex_unlock(&queue->lock);
}

void dispatch_process_priority(PCB *proceso, Machine *machine){
    
    if (!machine)
    {
        printf("Error: maquina no inicializada. \n");
        return;
    }
    
    Thread *available_thread = find_available_thread(machine);
        if (available_thread) {
            available_thread->procesos = proceso;
            printf("Despachando proceso PID=%d a un hilo\n", proceso->pid);
            printf("Ejecutando proceso PID= %d hasta su finalización \n", proceso->pid);
            proceso->tiempo_restante=0;
            proceso->state = TERMINATED;
            printf("Proceso PID= %d terminado \n", proceso->pid);
            available_thread->procesos=NULL; //liberar el hilo despues de ejecución
        }else{
            printf("No hay hilos disponibles para el proceso PID=%d \n", proceso->pid);
        }  
        
    
}

void schedule_priority(ProcessQueue *queue, Machine *machine) {
    pthread_mutex_lock(&queue->lock);
    PCB *highest_priority = NULL;
    int highest_priority_index=-1;

    for (int i = 0; i < queue->cont; i++) {
        PCB *proceso = &queue->procesos[i];
        if (proceso->state == READY && (!highest_priority || proceso->prioridad > highest_priority->prioridad)) {
            highest_priority = proceso;
            highest_priority_index=i;
        }
    }

    if (highest_priority) {
        highest_priority->state = RUNNING;
        printf("Ejecutando proceso PID=%d con prioridad=%d\n", highest_priority->pid, highest_priority->prioridad);
        pthread_mutex_unlock(&queue->lock);
        dispatch_process_priority(highest_priority,machine);
        pthread_mutex_lock(&queue->lock);
        highest_priority->state = TERMINATED;
        for (int j = highest_priority_index; j < queue->cont - 1; j++) {
            queue->procesos[j] = queue->procesos[j + 1];
        }
        queue->cont--;
        pthread_mutex_unlock(&queue->lock);
    } else {
        printf("No hay procesos READY para ejecutar.\n");
        pthread_mutex_unlock(&queue->lock);
    }
    
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
void update_process_state(PCB *proceso, ProcessState new_state) {
    if (!proceso) return;

    proceso->state = new_state;

    switch (new_state) {
        case READY:
            printf("Proceso PID=%d marcado como READY\n", proceso->pid);
            break;
        case RUNNING:
            printf("Proceso PID=%d marcado como RUNNING\n", proceso->pid);
            break;
        case TERMINATED:
            printf("Proceso PID=%d marcado como TERMINATED\n", proceso->pid);
            break;
        default:
            printf("Proceso PID=%d en estado desconocido\n", proceso->pid);
    }
}
void notify_scheduler(){
    pthread_mutex_lock(&sched_mutex);
    pthread_cond_signal(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);
}
void stop_scheduler(){
    pthread_mutex_lock(&sched_mutex);
    run_scheduler=0;
    pthread_cond_signal(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);
}