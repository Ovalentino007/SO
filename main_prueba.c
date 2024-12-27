
// PARA COMPILAR: gcc kernel.c -lpthread -o kernel

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>



pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2=PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_timer=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_timer=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_timer2=PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_sched=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_sched=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_sched2=PTHREAD_COND_INITIALIZER;

pthread_mutex_t sc_proc=PTHREAD_MUTEX_INITIALIZER;

#define NUM_TIMERS 1
//process generation frequency
int PROC_GEN_FREQ;
int NEXT_FREE_PID = 1;
//int MAX_NUM_INSTR = 20;
int MAX_NUM_INSTR = 200;
int SCHED_FREQ = 25;
int count_PG = 0;
int count_sched = 0;
int count_queueprint = 0;
int INITIAL_QUANTUM = 2;
const int PRIORITIES = 5;
int MAX_PRIOR = PRIORITIES-1;

int done=NUM_TIMERS;
int wakeup_timer=0;
int wakeup_sched=0;
int FREQ, NUM_CPU, NUM_CORES, NUM_THR;
int QUEUEPRINT_FREQ, RAND_PG;
int PRINT_CICLE, PRINT_PG, PRINT_SCHED, PRINT_CLK, PRINT_TMR;

struct Machine *machine;

struct PCB{
        unsigned int pid;
        unsigned int ppid;
        unsigned int uid;
        unsigned int PC;
        // número de instrucciones que tiene el proceso
        unsigned int num_instr;
        /* el carácter indica el estado del proceso:
         R: Running
         F: Finished
         W: Waiting
         B: Blocked
        */
        unsigned char State;
        void* stackPointer;
        void* generalRegisters;
        //prioridad entre 0 y 4
        int priority;
        //número de ticks antes de ser despachado.
        int quantum;
    };

struct PQ{
    unsigned int tam;
    struct PCB *primero;
    struct PCB *ultimo;
};

struct Machine{
    struct CPU *cpus;
};

struct CPU{
    struct core *cores;
};

struct core{
    struct hilo *hilos;
};

struct hilo{
    struct PCB *proc_en_ejec;
    //cuánto tiempo lleva el proceso ejecutándose
    //para saber si se ha consumido el quantum
    unsigned int time_in_cpu;
};

struct elem{
    struct PCB* proc;
    struct elem* prev;
    struct elem* next;
};

struct ProccessLinkedList{
    //list length
    unsigned int len;
    //list first element
    struct elem* first;
    //list last element
    struct elem* last;
};

struct PriorityQueues{
    struct ProccessLinkedList** queues;
    unsigned int len;
};


struct ProccessLinkedList *running_proc;
struct PriorityQueues* waiting_proc;
struct ProccessLinkedList *bloqued_proc;
struct ProccessLinkedList *finished_proc;

void my_append(struct ProccessLinkedList *list,struct elem* e);
void delete_last(struct ProccessLinkedList *list);

void my_append(struct ProccessLinkedList *list,struct elem* e){
    if(list==NULL){
        printf("ERROR: la lista no apunta a nada !!!\n");
        return;
    }
    if(list->len==0){
        list->first=e;
        list->last=e;
    }else{
        e->prev=list->last;
        list->last->next=e;
        list->last=e;
    }

    list->len++;
}

void delete_last(struct ProccessLinkedList *list){
    list->len--;
    list->last->prev->next=NULL;
}

struct PCB* get_del_next(struct ProccessLinkedList **list){
    
    struct PCB* biggest_prior;
    for(int p=MAX_PRIOR;p>=0;p--){
        if((*(list+p))->len>0){
            biggest_prior=(*(list+p))->first->proc;
            (*(list+p))->first=(*(list+p))->first->next;
            (*(list+p))->first->prev=NULL;
            (*(list+p))->len--;
    
            return biggest_prior;
        }
    }
    printf("get_del_next: ningún proceso disponible\n");

    return NULL;
}

void print_linkedlist(struct ProccessLinkedList *list){
    if(list==NULL){
        printf("ERROR: la lista no apunta a nada !!!\n");
        return;
    }
    printf("Linked List (len: %i)\n",list->len);
    if(list->len>0){
        printf("%i -> ",list->first->proc->pid);
        if(list->len>1){
            struct elem* e=list->first;
            for(int i=0;i<(list->len)-1;i++){
                e=e->next;
                printf("%i -> ",e->proc->pid);
            }
        }
        
    }
    
    printf("\n");
}

struct elem* pcb_to_elem(struct PCB* pcb){
    struct elem *e=(struct elem *)malloc(sizeof(struct elem));
    e->proc=pcb;
    e->next=(struct elem *)malloc(sizeof(struct elem));
    e->prev=(struct elem *)malloc(sizeof(struct elem));

    return e;
}

void* clk(){
        int ci,cj,ck;
        while(1){
        sleep(1);
        pthread_mutex_lock(&mutex);
        if(PRINT_CICLE) printf("CLK: produced\n");
        while(done<NUM_TIMERS) pthread_cond_wait(&cond1,&mutex);

        pthread_mutex_lock(&sc_proc);
        for(ci=0;ci<NUM_CPU;ci++){
            for(cj=0;cj<NUM_CORES;cj++){
                for(ck=0;ck<NUM_THR;ck++){
                    if((((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec!=NULL){
                        if(PRINT_CLK) printf("CLK: Actualizando PC (thread: (%i,%i,%i), pid: %i)\n",ci,cj,ck,(((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->pid);
                        (((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->PC++;
                        (((machine->cpus+ci)->cores+cj)->hilos+ck)->time_in_cpu++;
                        if((((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->quantum == (((machine->cpus+ci)->cores+cj)->hilos+ck)->time_in_cpu || (((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->PC == (((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->num_instr){
                            if((((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->quantum == (((machine->cpus+ci)->cores+cj)->hilos+ck)->time_in_cpu){
                                if(PRINT_CLK) printf("CLK: Quantum agotado (thread: (%i,%i,%i), pid: %i)\n",ci,cj,ck,(((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->pid);
                            }
                            if((((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->PC == (((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->num_instr){
                                if(PRINT_CLK) printf("CLK: Proceso terminado (thread: (%i,%i,%i), pid: %i)\n",ci,cj,ck,(((machine->cpus+ci)->cores+cj)->hilos+ck)->proc_en_ejec->pid);
                            }

                            pthread_mutex_lock(&mutex_sched);
                            wakeup_sched=1;
                            pthread_cond_signal(&cond_sched);
                            if(PRINT_CLK) printf("CLK: avisando a SCHED\n");
                            pthread_mutex_unlock(&mutex_sched);
                        }
                    }
                }
            }
            }

        pthread_mutex_unlock(&sc_proc);
        done=0;
        pthread_cond_broadcast(&cond2);
        pthread_mutex_unlock(&mutex);
        
    }
}

void* timer(){

    while(1){
        pthread_mutex_lock(&mutex);
        done++;
        if(PRINT_CICLE) printf("TIMER: consumed\n");
        pthread_cond_signal(&cond1);
        pthread_cond_wait(&cond2,&mutex);
        pthread_mutex_unlock(&mutex);
        count_PG++;
        count_sched++;
        count_queueprint++;
        
        if(count_PG==PROC_GEN_FREQ){
            if(RAND_PG) PROC_GEN_FREQ=(rand()+1)%10;
            count_sched=0;
            count_PG=0;
            pthread_mutex_lock(&mutex_timer);
            while(wakeup_timer<1) pthread_cond_wait(&cond_timer,&mutex_timer);
            wakeup_timer=0;
            pthread_cond_broadcast(&cond_timer2);
            pthread_mutex_unlock(&mutex_timer);
        }
        if(count_sched==SCHED_FREQ){
            count_sched=0;
            pthread_mutex_lock(&mutex_sched);
            wakeup_sched=1;
            if(PRINT_TMR) printf("TIMER: avisando a SCHED\n");
            pthread_cond_signal(&cond_sched);
            pthread_mutex_unlock(&mutex_sched);
        }
        if(count_queueprint==QUEUEPRINT_FREQ){
            count_queueprint=0;
            pthread_mutex_lock(&sc_proc);
            printf("------- WAITING PROCESS QUEUE -------\n");
            for(int m=MAX_PRIOR;m>=0;m--){
                printf("(priority: %i)",m);
                print_linkedlist(*(waiting_proc->queues+m));
            }
            printf("-------------------------------------\n");
            pthread_mutex_unlock(&sc_proc);
        }
        
    }
}


void* processGenerator(){

    //esperar al timer
    

    while(1){
    pthread_mutex_lock(&mutex_timer);
    wakeup_timer=1;
    pthread_cond_signal(&cond_timer);
    pthread_cond_wait(&cond_timer2,&mutex_timer);
    pthread_mutex_unlock(&mutex_timer);

    //generar proceso
    struct PCB *newproc = (struct PCB *)malloc(sizeof(struct PCB));
    newproc->pid=NEXT_FREE_PID;
    NEXT_FREE_PID++;
    newproc->PC=0;
    newproc->num_instr=rand()%MAX_NUM_INSTR + 1;
    newproc->priority=MAX_PRIOR;
    newproc->quantum=INITIAL_QUANTUM;

    pthread_mutex_lock(&sc_proc);
    my_append(*(waiting_proc->queues+MAX_PRIOR),pcb_to_elem(newproc));
    waiting_proc->len++;
    pthread_mutex_unlock(&sc_proc);

    if(PRINT_PG) printf("PG: New process generated (pid: %i, length: %i)\n",newproc->pid,newproc->num_instr);

    pthread_mutex_lock(&mutex_sched);
    wakeup_sched=1;
    pthread_cond_signal(&cond_sched);
    pthread_mutex_unlock(&mutex_sched);
    }
}

void* scheduler(){

    int pi,pj,pk;
    int continuar;
    struct PCB* next_proc;

    while(1){
        pthread_mutex_lock(&mutex_sched);
        while(wakeup_sched==0)  pthread_cond_wait(&cond_sched,&mutex_sched);
        if(PRINT_SCHED) printf("SCHED: prioridades actualizadas\n");
        pi,pj,pk = 0;
        continuar = 1;
        pthread_mutex_lock(&sc_proc);
        while(continuar && pi<NUM_CPU){
            while(continuar && pj<NUM_CORES){
                while(continuar && pk<NUM_THR){
                        if((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec!=NULL){
                            if((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->PC == (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->num_instr){
                                if(PRINT_SCHED) printf("SCHED: Despachado por haber terminado (thread: (%i,%i,%i), pid=%i)\n",pi,pj,pk,(((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->pid);
                                (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec=NULL;
                                (((machine->cpus+pi)->cores+pj)->hilos+pk)->time_in_cpu=0;
                            }
                            else{
                                if((((machine->cpus+pi)->cores+pj)->hilos+pk)->time_in_cpu==(((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->quantum){
                                    if(waiting_proc->len>0){
                                        //my_append(*(waiting_proc->queues+MAX_PRIOR),pcb_to_elem((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec));
                                        if((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->priority==0){
                                            (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->quantum=9999;
                                        }else{
                                            (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->quantum=(((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->quantum^2+1;
                                            (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->priority--;
                                        }
                                        my_append(*(waiting_proc->queues+((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->priority)),pcb_to_elem((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec));
                                        waiting_proc->len++;

                                        if(PRINT_SCHED) printf("SCHED: Proceso del thread (%i,%i,%i) cambiado a pid=%i)\n",pi,pj,pk,(((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->pid);
                                        (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec=NULL;
                                    }
                                    (((machine->cpus+pi)->cores+pj)->hilos+pk)->time_in_cpu=0;
                                }
                            }
                        }
                            if((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec==NULL && waiting_proc->len>0){
                                (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec=(struct PCB*)malloc(sizeof(struct PCB));
                                    next_proc=get_del_next(waiting_proc->queues);
                                    waiting_proc->len--;
                                    (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec=next_proc;
                                    (((machine->cpus+pi)->cores+pj)->hilos+pk)->time_in_cpu=0;
                                    my_append(running_proc,pcb_to_elem((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec));
                                    if(PRINT_SCHED) printf("SCHED: process (%i) added to thread (%i,%i,%i), (%i processes waiting to be runned)\n",(((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec->pid,pi,pj,pk,waiting_proc->len);
                                    continuar=0;
                                
                                
                            }
                            pk++;
                    }
                        
                        
                    
                
                pk=0;
                pj++;
            }
            pj=0;
            pi++;
        }
        pi=0;
        continuar=1;
        pthread_mutex_unlock(&sc_proc);
        wakeup_sched=0;
        pthread_mutex_unlock(&mutex_sched);
    }
}

int main(){

    bool seguir=0;

    

    //recibir paramtros de configuracion
    while(!seguir){
        printf("Introduzca la frecuencia del reloj (min 1GHz, max 1000GHz):");
        scanf("%d",&FREQ);
        if(FREQ<1 || FREQ>1000) printf("ERROR: Fuera de los limites.\n");
        else seguir=1;
    }
    seguir=0;
    
    while(!seguir){
        printf("Introduzca el numero de procesadores: (max 64)");
        scanf("%d",&NUM_CPU);
        if(NUM_CPU>64) printf("ERROR: Fuera de los limites.\n");
        else seguir=1;
    }
    seguir=0;
    
    while(!seguir){
        printf("Introduzca el numero de cores de cada procesador: (max 64)");
        scanf("%d",&NUM_CORES);
        if(NUM_CORES>64) printf("ERROR: Fuera de los limites.\n");
        else seguir=1;
    }
    seguir=0;

    while(!seguir){
        printf("Introduzca el numero de hilos de cada core: (max 64)");
        scanf("%d",&NUM_THR);
        if(NUM_THR>64) printf("ERROR: Fuera de los limites.\n");
        else seguir=1;
    }
    seguir=0;

    while(!seguir){
        printf("Introduzca 1 si quiere que el tiempo de generación de procesos sea aleatorio y 0 si no:");
        scanf("%d",&RAND_PG);
        if(RAND_PG!=1 && RAND_PG!=0) printf("ERROR: introduzca 1 o 0\n");
        else seguir=1;
    }
    seguir=0;

    if(!RAND_PG){
        while(!seguir){
            printf("Introduzca cada cuantos ciclos quieres que se genere un proceso nuevo: (min 1, max 20)");
            scanf("%d",&PROC_GEN_FREQ);
            if(PROC_GEN_FREQ<1 || PROC_GEN_FREQ>20) printf("ERROR: Fuera de los limites.\n");
            else seguir=1;
        }
    }else{
        srand(time(NULL));
        PROC_GEN_FREQ=(rand()+1)%10;
    }

    seguir=0;

    while(!seguir){
        printf("Introduzca el quantum inicial de los procesos nuevos (min 1, max 10):");
        scanf("%d",&INITIAL_QUANTUM);
        if(INITIAL_QUANTUM<1 || INITIAL_QUANTUM>10) printf("ERROR: Fuera de los limites.\n");
        else seguir=1;
    }
    seguir=0;
    

    while(!seguir){
        printf("Introduzca cada cuantos ciclos quiere imprimir la(s) cola(s) de procesos (0 signica no imprimirla nunca):");
        scanf("%d",&QUEUEPRINT_FREQ);
        if(QUEUEPRINT_FREQ<0 || QUEUEPRINT_FREQ>50) printf("ERROR: Fuera de los limites.\n");
        else seguir=1;
    }
    seguir=0;

    while(!seguir){
        printf("Introduzca 1 si quiere y 0 si no quiere imprimir cada ciclo de reloj:");
        scanf("%d",&PRINT_CICLE);
        if(PRINT_CICLE==1 || PRINT_CICLE==0) seguir=1;
        else printf("ERROR: introduzca 1 o 0\n");
    }
    seguir=0;

    while(!seguir){
        printf("Introduzca 1 si quiere y 0 si no quiere imprimir los mensajes de PROCESS GENERATOR:");
        scanf("%d",&PRINT_PG);
        if(PRINT_PG==1 || PRINT_PG==0) seguir=1;
        else printf("ERROR: introduzca 1 o 0\n");
    }
    seguir=0;

    while(!seguir){
        printf("Introduzca 1 si quiere y 0 si no quiere imprimir los mensajes de SCHEDULER:");
        scanf("%d",&PRINT_SCHED);
        if(PRINT_SCHED==1 || PRINT_SCHED==0) seguir=1;
        else printf("ERROR: introduzca 1 o 0\n");
    }
    seguir=0;

    while(!seguir){
        printf("Introduzca 1 si quiere y 0 si no quiere imprimir los mensajes de CLOCK:");
        scanf("%d",&PRINT_CLK);
        if(PRINT_CLK==1 || PRINT_CLK==0) seguir=1;
        else printf("ERROR: introduzca 1 o 0\n");
    }
    seguir=0;

    while(!seguir){
        printf("Introduzca 1 si quiere y 0 si no quiere imprimir los mensajes de TIMER:");
        scanf("%d",&PRINT_TMR);
        if(PRINT_TMR==1 || PRINT_TMR==0) seguir=1;
        else printf("ERROR: introduzca 1 o 0\n");
    }
    seguir=0;

    printf("-- Parámetros inicializados --\n");
   
    //inicializar estructuras de datos
    machine=(struct Machine*)malloc(sizeof(struct Machine));
    machine->cpus=(struct CPU*)malloc(NUM_CPU*sizeof(struct CPU*));

    printf("-- Machine inicializada --\n");
    
    for(int i=0;i<NUM_CPU;i++){
        (machine->cpus+i)->cores=(struct core*)malloc(NUM_CORES*sizeof(struct core*));
        for(int j=0;j<NUM_CORES;j++){
            ((machine->cpus+i)->cores+j)->hilos=(struct hilo*)malloc(NUM_THR*sizeof(struct hilo));
        }
    }

    printf("--Comprobando inicialización de la Machine--\n");

    for(int i=0;i<NUM_CPU;i++){
        for(int j=0;j<NUM_CORES;j++){
            for(int k=0;k<NUM_THR;k++){
                printf("THREAD %i (%i,%i,%i) IN: %p\n",
                i*NUM_CORES*NUM_THR+j*NUM_THR+k,
                i,
                j,
                k,
                (((machine->cpus+i)->cores+j)->hilos+k));
            }
        }
    }

    printf("Inicializando listas de procesos...\n");
    running_proc=(struct ProccessLinkedList*)malloc(sizeof(struct ProccessLinkedList));
    bloqued_proc=(struct ProccessLinkedList*)malloc(sizeof(struct ProccessLinkedList));
    finished_proc=(struct ProccessLinkedList*)malloc(sizeof(struct ProccessLinkedList));
    waiting_proc=(struct PriorityQueues*)malloc(sizeof(struct PriorityQueues));
    waiting_proc->queues=(struct ProccessLinkedList**)malloc(PRIORITIES*sizeof(struct ProccessLinkedList*));
    for(int i=0;i<PRIORITIES;i++){
        *(waiting_proc->queues+i)=(struct ProccessLinkedList*)malloc(sizeof(struct ProccessLinkedList));
        (*(waiting_proc->queues+i))->len=0;
    }
    waiting_proc->len=0;
    printf("Listas inicializadas\n");

    pthread_t t1,t2,t3,t4;

    //lanzar hilos

    //lanzar hilo clock
    pthread_create(&t1,NULL,clk,NULL);
    //lanzar hilo Process Generator
    pthread_create(&t3,NULL,processGenerator,NULL);
    //lanzar hilo scheduler
    pthread_create(&t4,NULL,scheduler,NULL);
    //lanzar hilo timer
    pthread_create(&t2,NULL,timer,NULL);
    
    //esperar a que terminen los hilos (NUNCA)
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);

    return 0;
}