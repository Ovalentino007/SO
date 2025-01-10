#ifndef structures_h
#define structures_h

#include <pthread.h>

typedef enum{
    READY,
    RUNNING,
    WAITING,
    TERMINATED
}ProcessState;

typedef struct{
    void *code;
    void *data;
    void *pgb;
}MemoryMap;

typedef struct {
    int pid;
    int tiempo_vida;
    int tiempo_restante;
    int prioridad;
    ProcessState state;
    MemoryMap mm;
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
    int id;
    int available;
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

typedef struct{
    char *memory;
    size_t size;
    size_t kernel_end;
}PhysicalMemory;

typedef struct{
    int virtual_page;
    int physical_frame;
    int valid;
}PageTableEntry;


void init_process_queue(ProcessQueue *queue, int capacidad_max);
void add_process(ProcessQueue *queue, PCB process);
void delete_process_queue(ProcessQueue *queue);
PCB *get_next_ready_process(ProcessQueue *queue);
void init_machine(Machine *machine, int num_cpus, int num_cores, int num_threads);
void delete_machine(Machine *machine);
void init_physical_memory(PhysicalMemory *pm, size_t total_size, size_t kernel_size);
void create_page_table(PhysicalMemory *pm, PCB *pcb, int num_pages);

#endif