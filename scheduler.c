#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "scheduler.h"
#include "clock.h"
#include "timer.h"
#include "structures.h"

pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sched_cond = PTHREAD_COND_INITIALIZER;
int run_scheduler=1;
static int last_thread_used = 0;  

void *scheduler_function(void *arg){

    ScheduleConfig *config = (ScheduleConfig *)arg;
    ProcessQueue *queue = config->queue;
    Machine *machine = config->machine;

    while(run_scheduler){

        pthread_mutex_lock(&sched_mutex);
        pthread_cond_wait(&sched_cond,&sched_mutex); //Espera a ser notificado para continuar

        if (!run_scheduler)
        {
            pthread_mutex_unlock(&sched_mutex);
            break;
        }
        if (config->policy == ROUND_ROBIN) //Politica elegida RR
        {
            printf("Scheduler activado con politica: Round Robin \n");

        }else if (config->policy == PRIORITY) //Politica elegida Prioridad
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
                schedule_round_robin(queue, machine,config->quantum); //Llamamos a la función del scheduler que ejecuta RR
                break;
            case PRIORITY:
                schedule_priority(queue, machine); // //Llamamos a la función del scheduler que ejecuta prioridades
                break;
        }

        pthread_mutex_unlock(&sched_mutex);
    }
    
    return NULL;

}

void schedule_round_robin(ProcessQueue *queue, Machine *machine, int quantum){ //Ejecuta procesos en intervalos de tiempo fijo de forma circular

    pthread_mutex_lock(&queue->lock);
    int procesos_ejecutados=0;
    
    for (int i = 0; i < queue->capacidad_max; i++)
    {
        PCB *proceso = &queue->procesos[i];

        if (proceso->state == READY && proceso->pid > 0)
        {
            proceso->state = RUNNING; //Aquellos procesos en estado Ready son tratados
            procesos_ejecutados++;
        }
    }
    if (procesos_ejecutados > 0)
    {
        printf("Scheduler seleccionó %d procesos para ejecutar \n", procesos_ejecutados);
        
        pthread_mutex_unlock(&queue->lock);
        dispatch_process_rr(queue,machine,quantum); //Llamamos al dispatcher para asignar hilos a los procesos
        pthread_mutex_lock(&queue->lock);

        for (int i = 0; i < queue->cont; ) {
            if (queue->procesos[i].state == TERMINATED) { //Limpiamos la cola de procesos terminados
                
                for (int j = i; j < queue->cont - 1; j++) 
                {
                    queue->procesos[j] = queue->procesos[j + 1];
                }
                queue->cont--; //Reducimos el contador de procesos
            } else {
                i++; //Solo avanzamos si no eliminamos un proceso
            }
        }

        pthread_mutex_unlock(&queue->lock);

    }else
    {
        printf("No hay procesos READY para ejecutar \n");
        pthread_mutex_unlock(&queue->lock);
    }
    
}

void dispatch_process_rr(ProcessQueue *queue, Machine *machine, int quantum){ //Asigna hilos de la máquina a procesos buscando aquellos que esten disponibles
    
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
            if (available_thread->available) {
                
                available_thread->procesos = proceso;
                available_thread->available = 0;
                int id = available_thread->id;
                int run_time = (proceso->tiempo_restante < quantum) ? proceso->tiempo_restante : quantum;
                proceso->tiempo_restante -= run_time;
                
                printf("Despachando proceso PID=%d a un hilo  %d\n", proceso->pid, id);
                printf("Ejecutando proceso PID= %d durante Quantum= %d \n", proceso->pid, quantum);
                
                if (proceso->tiempo_restante <=0) //Si el tiempo de ejecución del proceso ha terminado
                {
                    proceso->state = TERMINATED;

                    printf("Proceso PID= %d terminado \n", proceso->pid);

                }else{ //Si queda tiempo por ejecutar del proceso

                    proceso->state = READY;
                    printf("Proceso PID=%d se reprograma con tiempo restante= %d \n", proceso->pid, proceso->tiempo_restante);
                }

                available_thread->procesos=NULL; //Liberamos el hilo despues de ejecución
                available_thread->available=1;
            
            }else{
                printf("No hay hilos disponibles para el proceso PID=%d \n", proceso->pid);
            }  
        }
    
    }
    pthread_mutex_unlock(&queue->lock);
}

void dispatch_process_priority(PCB *proceso, Machine *machine){ //Asignamos un hilo para cada proceso
    
    if (!machine)
    {
        printf("Error: maquina no inicializada. \n");
        return;
    }
    
    Thread *available_thread = find_available_thread(machine);

    if (available_thread) {

            available_thread->procesos = proceso;
            available_thread->available = 0;
            int id = available_thread->id;

            printf("Despachando proceso PID=%d a un hilo %d\n", proceso->pid, id);
            printf("Ejecutando proceso PID= %d hasta su finalización \n", proceso->pid);
            
            while (proceso->tiempo_restante > 0)
            {
                //sleep(1);
                proceso->tiempo_restante--;
                printf("Proceso PID=%d ejecutándose, tiempo restante=%d\n", proceso->pid, proceso->tiempo_restante);
            }
            
            proceso->state = TERMINATED; //El proceso ha terminado su ejecución
            
            printf("Proceso PID= %d terminado \n", proceso->pid);
            
            available_thread->procesos=NULL; //Liberamos el hilo despues de ejecución
            available_thread->available=1;
    }else{
            printf("No hay hilos disponibles para el proceso PID=%d \n", proceso->pid);
    }  
        
    
}

void schedule_priority(ProcessQueue *queue, Machine *machine) { //Ejecutamos aquellos procesos con mayor prioridad

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
        
        for (int j = highest_priority_index; j < queue->cont - 1; j++) { //Limpiamos la cola de procesos terminados
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
    int total_threads = machine->num_cpus * machine->num_cores * machine->num_threads;
    int start_index = last_thread_used;

    for (int offset = 0; offset < total_threads; offset++) {
        int index = (start_index + offset) % total_threads;
        int cpu_index = index / (machine->num_cores * machine->num_threads);
        int core_index = (index / machine->num_threads) % machine->num_cores;
        int thread_index = index % machine->num_threads;

        Thread *thread = &machine->cpus[cpu_index].cores[core_index].threads[thread_index];
        if (thread->available) {
            last_thread_used = index + 1;  // Avanzamos el índice circular
            return thread;
        }
    }
    return NULL;  // No hay hilos disponibles
}

/*Thread *find_available_thread(Machine *machine) { //Encontramos un hilo disponible en la máquina

    for (int i = 0; i < machine->num_cpus; i++) {
        for (int j = 0; j < machine->num_cores; j++) {
            for (int k = 0; k < machine->num_threads; k++) {
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if (thread->available) {  // Hilo libre
                    return thread;
                }
            }
        }
    }
    return NULL;
}
*/
//No es necesario, no lo usamos
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

void notify_scheduler(){ //Activamos el scheduler
    pthread_mutex_lock(&sched_mutex);
    pthread_cond_signal(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);
}

void stop_scheduler(){ //Detenemos el hilo del scheduler
    pthread_mutex_lock(&sched_mutex);
    run_scheduler=0;
    pthread_cond_signal(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);
}