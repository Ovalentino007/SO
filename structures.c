#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "structures.h"
#include "scheduler.h"

void init_process_queue(ProcessQueue *queue, int max_capacidad){ //Inicializar una cola de procesos

    queue->procesos = malloc(max_capacidad * sizeof(PCB)); //Asignamos memoria para la cola
    queue->cont = 0;
    queue->capacidad_max = max_capacidad;
    pthread_mutex_init(&queue->lock,NULL); //Protegemos de acceso concurrente

}

void add_process(ProcessQueue *queue, PCB process){ //Añadimos un proceso a la cola

    pthread_mutex_lock(&queue->lock);

    if(queue->cont < queue->capacidad_max)
    {
        queue->procesos[queue->cont++] = process;
        printf("Proceso añadido: PID=%d \n", process.pid);
    }else
    {
        printf("Error: Cola de procesoss llena \n");
        printf("No se pudo añadir el proceso PID=%d \n", process.pid);
    }

    pthread_mutex_unlock(&queue->lock);

}

void delete_process_queue(ProcessQueue *queue){ //Liberamos la mememoria y los recursos asignados a la cola de procesos

    free(queue->procesos);
    pthread_mutex_destroy(&queue->lock);

}

//No es necesario, no lo utilizamos
PCB *get_next_ready_process(ProcessQueue *queue){ //Obtenemos el siguiente procesos en estado READY en la cola

    pthread_mutex_lock(&queue->lock);
    for (int i = 0; i < queue->cont; i++)
    {
        if (queue->procesos[i].state == READY)
        {
            pthread_mutex_unlock(&queue->lock);
            return &queue->procesos[i];
        }
        
    }
    pthread_mutex_unlock(&queue->lock);
    return NULL;

}

void init_machine(Machine *machine, int cpus, int cores, int threads){ //Inicializamos una máquina con un numero de CPUs, núcleos e hilos en concreto

    machine->num_cpus = cpus;
    machine-> num_cores = cores;
    machine-> num_threads = threads;
    machine->cpus = malloc(cpus*sizeof(CPU));

    if (!machine->cpus)
    {
        printf("Error: no se pudo asignar memoria para las CPUS \n");
        return;
    }

    for (int i = 0; i < cpus; i++)
    {
        machine->cpus[i].cores = malloc(cores * sizeof(Core));

        if (!machine->cpus[i].cores)
        {
        printf("Error: no se pudo asignar memoria para los Cores \n");
        delete_machine(machine);
        return;
        }

        for (int j = 0; j < cores; j++)
        {
            machine->cpus[i].cores[j].threads = malloc(threads*sizeof(Thread));

            if (!machine->cpus[i].cores[j].threads)
            {
                printf("Error: no se pudo asignar memoria para los Threads \n");
                delete_machine(machine);
                return;
            }
        }
        
    }
    

}

void delete_machine(Machine *machine){ //Liberamos la memoria asignada para la máquina

    for (int i = 0; i < machine->num_cpus; i++)
    {
        for (int j = 0; j < machine->num_cores; j++)
        {
            free(machine->cpus[i].cores[j].threads);
        }

        free(machine->cpus[i].cores);
    }

    free(machine->cpus);
}