#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 3
#define MAX_PROCESSES 10
// Estructura para los subsistemas
typedef struct {
    int id;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
} Subsystem;

typedef struct PCB{
    int pid;
}PCB;

typedef struct ProcessQueue{
    int count;
    PCB* processes[MAX_PROCESSES];
}ProcessQueue;

typedef struct Machine{
    int num_cpus;
    int num_cores;
}Machine;

ProcessQueue queue;
Machine machine;

void init_process_queue(){
    queue.count = 0;
}

void add_process(int pid){

    if (queue.count < MAX_PROCESSES)
    {
        PCB* new_process = (PCB*) malloc(sizeof(PCB));
        new_process ->pid = pid;
        queue.processes[queue.count++] = new_process;
        printf("Proceso %d añadido \n", pid);
    }else{
        printf("Cola llena \n");
    }
    
}

void* subsystem_function(void* arg) {
    Subsystem *subsystem = (Subsystem*)arg;

    // Simulación del trabajo del subsistema
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(subsystem->mutex);
        // Trabajando en el subsistema
        printf("Subsistema %d está trabajando.\n", subsystem->id);
        pthread_cond_signal(subsystem->cond); // Señalar que el subsistema ha trabajado
        pthread_mutex_unlock(subsystem->mutex);
        sleep(1); // Simular tiempo de procesamiento
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t threads[NUM_THREADS];
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    // Inicializar mutex y condición
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // Crear e inicializar subsistemas
    Subsystem subsystems[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        subsystems[i].id = i + 1;
        subsystems[i].mutex = &mutex;
        subsystems[i].cond = &cond;
        pthread_create(&threads[i], NULL, subsystem_function, (void*)&subsystems[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Limpiar recursos
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    printf("Simulador finalizado.\n");
    return 0;
}
