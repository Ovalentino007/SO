#ifndef structures_h
#define structures_h

#include <pthread.h>

typedef enum{
    READY,
    RUNNING,
    WAITING,
    TERMINATED
}ProcessState;

typedef struct {
    int pid;
    int tiempo_vida;
    int tiempo_restante;
    int prioridad;
    ProcessState state;
}PCB;

typedef struct {

    PCB *procesos;
    int cont;
    int capacidad_max;
    pthread_mutex_t lock;

}ProcessQueue;

typedef struct 
{
    PCB *procesos;
}Thread;

typedef struct 
{
    Thread *threads;
}Core;

typedef struct 
{
    Core *cores;
}CPU;

typedef struct{
    CPU *cpus;
    int num_cores;
    int num_cpus;
    int num_threads;

}Machine;

void init_process_queue(ProcessQueue *queue, int capacidad_max);
void add_process(ProcessQueue *queue, PCB process);
void delete_process_queue(ProcessQueue *queue);
PCB *get_next_ready_process(ProcessQueue *queue);
void init_machine(Machine *machine, int num_cpus, int num_cores, int num_threads);
void delete_machine(Machine *machine);

#endif