
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

#define NUM_TIMERS 1
//process generation frequency
//int PROC_GEN_FREQ;
//int PROC_GEN_FREQ =5;
//int SCHED_FREQ = 1;
int PROC_GEN_FREQ = 1;
int NEXT_FREE_PID = 1;
int MAX_NUM_INSTR = 10;
int SCHED_FREQ = 10;
int count_PG = 0;
int count_sched = 0;


int done=NUM_TIMERS;
int wakeup_timer=0;
int wakeup_sched=0;
int FREQ, NUM_CPU, NUM_CORES, NUM_THR;
//int i,j,k;

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

struct ProccessLinkedList *running_proc;
struct ProccessLinkedList *waiting_proc;
struct ProccessLinkedList *bloqued_proc;
struct ProccessLinkedList *finished_proc;

void my_append(struct ProccessLinkedList *list,struct elem* e);
void delete_last(struct ProccessLinkedList *list);

void my_append(struct ProccessLinkedList *list,struct elem* e){
    list->len++;
    e->prev=list->last;
    list->last->next=e;
    list->last=e;
}

void delete_last(struct ProccessLinkedList *list){
    list->len--;
    list->last->prev->next=NULL;
}

void print_linkedlist(struct ProccessLinkedList *list){
    printf("Linked List (len: %i)\n",list->len);
    printf("%i -> ",list->first->proc->pid);
    struct elem* e=list->first;
    for(int i=0;i<(list->len)-1;i++){
        e=e->next;
        printf("%i -> ",e->proc->pid);
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
    while(1){
        sleep(1);
        pthread_mutex_lock(&mutex);
        //done=1;
        printf("Clock: produced\n");
        while(done<NUM_TIMERS) pthread_cond_wait(&cond1,&mutex);

        for(int i=0;i<NUM_CPU;i++){
            for(int j=0;j<NUM_CORES;j++){
                for(int k=0;k<NUM_THR;k++){
                    if(sizeof(*((((machine->cpus+i)->cores+j)->hilos+k)))!=sizeof(struct PCB*)){
                        printf("Error\n");
                    }
                    if(sizeof(*((((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec))!=sizeof(struct PCB)){
                        printf("Error\n");
                    }
                    if((((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec!=NULL){
                        (((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec->PC++;
                        printf("PC actualizado (pid: %i)\n",(((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec->pid);
                        if((((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec->PC == (((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec->num_instr){
                            pthread_mutex_lock(&mutex_sched);
                            wakeup_sched=1;
                            pthread_cond_signal(&cond_sched);
                            printf("CLK AVISANDO A SCHED\n");
                            pthread_mutex_unlock(&mutex_sched);
                            //printf("flag\n");
                            (((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec->State='F';
                            //printf("flag\n");
                            //struct elem* p=(struct elem*)malloc(sizeof(struct elem));
                            //p->proc=machine->cpus->cores->hilos->proc_en_ejec;
                            //my_append(finished_proc,p);
                            //printf("flag\n");
                            //my_append(finished_proc,pcb_to_elem((((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec));
                            //printf("flag\n");
                            //delete_last(running_proc);
                            //printf("flag\n");

                            //(((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec=(struct PCB*)malloc(sizeof(struct PCB));
                            (((machine->cpus+i)->cores+j)->hilos+k)->proc_en_ejec=NULL;
                            
                            
                            //printf("flag\n");
                        }
                    }
                }
            }
            }


        done=0;
        pthread_cond_broadcast(&cond2);
        pthread_mutex_unlock(&mutex);
        
    }
}

void* timer(){
    while(1){
        pthread_mutex_lock(&mutex);
        done++;
        printf("timer: consumed\n");
        pthread_cond_signal(&cond1);
        pthread_cond_wait(&cond2,&mutex);
        pthread_mutex_unlock(&mutex);
        count_PG++;
        count_sched++;
        
        //printf("TIMER INTERRUPTION !\n");
        //avisar al procesos
        if(count_PG==PROC_GEN_FREQ){
            //count_sched=0;
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
            printf("TIMER AVISANDO A SCHED\n");
            pthread_cond_signal(&cond_sched);
            pthread_mutex_unlock(&mutex_sched);
        }
        
    }
}


void* processGenerator(){

    //esperar al timer
    int pi,pj,pk;
    int continuar;

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
    
    pi,pj,pk = 0;
    continuar = 1;
    while(continuar && pi<NUM_CPU){
        while(continuar && pj<NUM_CORES){
            while(continuar && pk<NUM_THR){
                if((((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec==NULL){
                    (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec=(struct PCB*)malloc(sizeof(struct PCB));
                    (((machine->cpus+pi)->cores+pj)->hilos+pk)->proc_en_ejec=newproc;
                    printf("PG: process added to thread (%i,%i,%i)\n",pi,pj,pk);
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
    printf("New process generated (pid: %i, length: %i)\n",newproc->pid,newproc->num_instr);
    //pthread_mutex_unlock(&mutex_timer);

    pthread_mutex_lock(&mutex_sched);
    wakeup_sched=1;
    pthread_cond_signal(&cond_sched);
    pthread_mutex_unlock(&mutex_sched);
    }
}


void* scheduler(){
    //esperar al timer

    while(1){
        pthread_mutex_lock(&mutex_sched);
        while(wakeup_sched==0)  pthread_cond_wait(&cond_sched,&mutex_sched);
        printf("PRIORIDADES ACTUALIZADAS\n");
        wakeup_sched=0;
        pthread_mutex_unlock(&mutex_sched);
    }
    



    /*
    while(1){
    pthread_mutex_lock(&mutex_timer);
    wakeup_timer=1;
    pthread_cond_signal(&cond_timer);
    pthread_cond_wait(&cond_timer2,&mutex_timer);
    

    //recalcular prioridades
    

    pthread_mutex_unlock(&mutex_timer);
    }
    */
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

    /*
    struct ProccessLinkedList *lista=(struct ProccessLinkedList*)malloc(sizeof(struct ProccessLinkedList));
    lista->len=0;
    struct PCB a = {79,0,0,0,0,'W',NULL,NULL};
    struct elem *elema = pcb_to_elem(&a);
    struct PCB b = {80,0,0,0,0,'W',NULL,NULL};
    struct elem *elemb = pcb_to_elem(&b);
    struct PCB c = {81,0,0,0,0,'W',NULL,NULL};
    struct elem *elemc = pcb_to_elem(&c);
    struct PCB d = {82,0,0,0,0,'W',NULL,NULL};
    struct elem *elemd = pcb_to_elem(&d);
    my_append(lista,elema);
    my_append(lista,elemb);
    my_append(lista,elemc);
    my_append(lista,elemd);
    print_linkedlist(lista);
    delete_last(lista);
    delete_last(lista);
    print_linkedlist(lista);
    */

   
    pthread_t t1,t_timer,t3,t4;
    pthread_mutex_init(&mutex,NULL);

    //lanzar hilos

    //lanzar hilo clock
    pthread_create(&t1,NULL,clk,NULL);
    //lanzar hilo Process Generator
    pthread_create(&t3,NULL,processGenerator,NULL);
    //lanzar hilo scheduler
    pthread_create(&t4,NULL,scheduler,NULL);
    //lanzar hilo timer del scheduler
    //pthread_create(&t_timer,NULL,timer_timer,NULL);
    //lanzar hilo timer del ProcessGenerator
    //pthread_create(&t_procgen,NULL,timer_timer,NULL);
    pthread_create(&t_timer,NULL,timer,NULL);
    
    //esperar a que terminen los hilos (NUNCA)
    pthread_join(t1,NULL);
    pthread_join(t_timer,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
}