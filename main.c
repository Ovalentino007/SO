#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "clock.h"
#include "timer.h"
#include "proc_generator.h"
#include "scheduler.h"
#include "structures.h"

// Para compilar gcc -Wall -o simulador main.c clock.c timer.c proc_generator.c scheduler.c structures.c -lpthread

ProcessQueue proc_queue;
Machine machine;
Timerconf tim_config;
Proc_generator_conf generator_config;

int main(int argc, char *argv[]){

    int num_cpus, num_cores, num_threads, max_procesos;
    int frec_tick, frec_gen;
    int policy_choice, quantum;

    printf("Bienvenido al simulador del mejor Sistema Operativo \n");
    //Configuración de la máquina
    printf("Ingrese el número de CPUs: ");
    scanf("%d", &num_cpus);
    printf("Ingrese el número de núcleos por CPU: ");
    scanf("%d", &num_cores);
    printf("Ingrese el numero de hilos por núcleo: ");
    scanf("%d", &num_threads);
    //Configuración de la cola de procesos
    printf("Ingrese el numero máximo de procesos en la cola: ");
    scanf("%d", &max_procesos);
    //Configuración del timer
    printf("Ingrese la frecuencia de tick del Timer (ciclos del reloj): ");
    scanf("%d", &frec_tick);
    tim_config.frecuencia_tick=frec_tick;
    //Configuración del generador de procesos
    printf("Ingrese la frecuencia de generación de procesos (segundos): ");
    scanf("%d", &frec_gen);
    generator_config.frecuencia_gen=frec_gen;
    //Configuración del Scheduler
    printf("Seleccione la política de planificación:\n");
    printf("1. Round Robin \n");
    printf("2. Prioridad \n");
    printf("Opcion: ");
    scanf("%d", &policy_choice);
    ScheduleConfig scheduler_config;
    scheduler_config.queue = &proc_queue;
    scheduler_config.machine = &machine;
    switch (policy_choice)
    {
    case 1:
        printf("Indique un quantum para los procesos:\n ");
        scanf("%d", &quantum);
        scheduler_config.quantum = quantum;
        scheduler_config.policy = ROUND_ROBIN;
        break;
    case 2:
        scheduler_config.policy = PRIORITY;
        break;
    default:
        printf("Opción invalida. Usando Round Robin por defecto. \n");
        scheduler_config.policy = ROUND_ROBIN;
    }
    
    //Inicialización de estructuras
    init_clock();
    init_process_queue(&proc_queue,max_procesos);
    init_machine(&machine, num_cpus, num_cores, num_threads);

    //Comienzo
    printf("El sistema esta ejecutandose. Presione Ctrl+C para detener. \n");
    
    //Creacion de hilos
    pthread_t  timer_thread, generator_thread, scheduler_thread;

    pthread_create(&timer_thread, NULL, timer_function, &tim_config);
    pthread_create(&generator_thread, NULL, proc_generator_function, &generator_config);
    pthread_create(&scheduler_thread, NULL, scheduler_function, &scheduler_config);
    
    //Simulación durante x segundos
    sleep(25);

    //Detención de hilos y subsistemas
    stop_clock();
    stop_timer();
    stop_proc_generator();
    stop_scheduler();

    //Espera a que los hilos terminen
    pthread_join(timer_thread,NULL);
    pthread_join(generator_thread,NULL);
    pthread_join(scheduler_thread,NULL);

    //Limpieza
    delete_process_queue(&proc_queue);
    delete_machine(&machine);

    printf("Simulación finalizada. Recursos liberados \n");

    return 0;

}