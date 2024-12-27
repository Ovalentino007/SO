#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_timer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_timer = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_timer2 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_sched = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_sched = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_sched2 = PTHREAD_COND_INITIALIZER;

#define NUM_TIMERS 1
// process generation frequency
// int PROC_GEN_FREQ;
// int PROC_GEN_FREQ =5;
// int SCHED_FREQ = 1;
int PROC_GEN_FREQ = 1;
int NEXT_FREE_PID = 1;
int MAX_NUM_INSTR = 10;
int SCHED_FREQ = 10;
int count_PG = 0;
int count_sched = 0;
int MAX_PRIOR = 4;

int done = NUM_TIMERS;
int wakeup_timer = 0;
int wakeup_sched = 0;
int FREQ, NUM_CPU, NUM_CORES, NUM_THR;
// int i,j,k;

struct Machine *machine;

struct PCB
{
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
    void *stackPointer;
    void *generalRegisters;
};

struct PQ
{
    unsigned int tam;
    struct PCB *primero;
    struct PCB *ultimo;
};

struct Machine
{
    struct CPU *cpus;
};

struct CPU
{
    struct core *cores;
};

struct core
{
    struct hilo *hilos;
};

struct hilo
{
    struct PCB *proc_en_ejec;
};

struct elem
{
    struct PCB *proc;
    struct elem *prev;
    struct elem *next;
};

struct ProccessLinkedList
{
    // list length
    unsigned int len;
    // list first element
    struct elem *first;
    // list last element
    struct elem *last;
};

struct ProccessLinkedList *running_proc;
struct ProccessLinkedList *waiting_proc;
struct ProccessLinkedList *bloqued_proc;
struct ProccessLinkedList *finished_proc;

void my_append(struct ProccessLinkedList *list, struct elem *e);
void delete_last(struct ProccessLinkedList *list);

void my_append(struct ProccessLinkedList *list, struct elem *e)
{
    if (list == NULL)
    {
        printf("ERROR: la lista no apunta a nada !!!\n");
        return;
    }
    if (list->len == 0)
    {
        list->first = e;
        list->last = e;
    }
    else
    {
        e->prev = list->last;
        list->last->next = e;
        list->last = e;
    }

    list->len++;
}

void delete_last(struct ProccessLinkedList *list)
{
    list->len--;
    list->last->prev->next = NULL;
}

struct PCB *get_del_first(struct ProccessLinkedList *list)
{
    struct PCB *first;
    if (list->len < 1)
    {
        printf("Error: la lista no tiene elementos !!!\n");
        return NULL;
    }
    else if (list == NULL)
    {
        printf("Error: la lista no apunta a nada !!!\n");
        return NULL;
    }
    else
    {
        first = list->first->proc;
        list->first = list->first->next;
        list->first->prev = NULL;
        list->len--;
    }
    return first;
}

void print_linkedlist(struct ProccessLinkedList *list)
{
    if (list == NULL)
    {
        printf("ERROR: la lista no apunta a nada !!!\n");
        return;
    }
    printf("Linked List (len: %i)\n", list->len);
    if (list->len > 0)
    {
        printf("%i -> ", list->first->proc->pid);
        if (list->len > 1)
        {
            struct elem *e = list->first;
            for (int i = 0; i < (list->len) - 1; i++)
            {
                e = e->next;
                printf("%i -> ", e->proc->pid);
            }
        }
    }

    printf("\n");
}

struct elem *pcb_to_elem(struct PCB *pcb)
{
    struct elem *e = (struct elem *)malloc(sizeof(struct elem));
    e->proc = pcb;
    e->next = (struct elem *)malloc(sizeof(struct elem));
    e->prev = (struct elem *)malloc(sizeof(struct elem));

    return e;
}

struct PCB *get_del_next(struct ProccessLinkedList **list)
{

    struct PCB *biggest_prior;
    for (int p = MAX_PRIOR; p >= 0; p--)
    {
        if ((*(list + p))->len > 0)
        {
            biggest_prior = (*(list + p))->first->proc;
            (*(list + p))->first = (*(list + p))->first->next;
            (*(list + p))->first->prev = NULL;
            (*(list + p))->len--;

            return biggest_prior;
        }
    }
    printf("get_del_next: ningún proceso disponible\n");

    return NULL;
}

struct PriorityQueues{
    struct ProccessLinkedList** queues;
    unsigned int len;
};

int main()
{
    struct ProccessLinkedList **lista = (struct ProccessLinkedList **)malloc((MAX_PRIOR + 1) * sizeof(struct ProccessLinkedList *));
    for (int i = 0; i < MAX_PRIOR + 1; i++)
    {
        *(lista + i) = (struct ProccessLinkedList *)malloc((MAX_PRIOR + 1) * sizeof(struct ProccessLinkedList));
        (*(lista + i))->len = 0;
    }
    struct PCB a = {79, 0, 0, 0, 0, 'W', NULL, NULL};
    struct elem *elema = pcb_to_elem(&a);
    struct PCB b = {80, 0, 0, 0, 0, 'W', NULL, NULL};
    struct elem *elemb = pcb_to_elem(&b);
    struct PCB c = {81, 0, 0, 0, 0, 'W', NULL, NULL};
    struct elem *elemc = pcb_to_elem(&c);
    struct PCB d = {82, 0, 0, 0, 0, 'W', NULL, NULL};
    struct elem *elemd = pcb_to_elem(&d);
    struct PCB e = {83, 0, 0, 0, 0, 'W', NULL, NULL};
    struct elem *eleme = pcb_to_elem(&e);
    struct PCB f = {84, 0, 0, 0, 0, 'W', NULL, NULL};
    struct elem *elemf = pcb_to_elem(&f);
    // printf("flag\n");
    printf("-------------------------------\n");
    for (int i = MAX_PRIOR; i >= 0; i--)
        print_linkedlist(*(lista + i));
    printf("-------------------------------\n");
    my_append(*(lista + 0), elema);
    my_append(*(lista + 0), elemb);
    my_append(*(lista + 0), elemc);
    my_append(*(lista + 0), elemd);
    my_append(*(lista + 0), eleme);
    my_append(*(lista + 0), elemf);
    printf("-------------------------------\n");
    for (int i = MAX_PRIOR; i >= 0; i--)
        print_linkedlist(*(lista + i));
    printf("-------------------------------\n");

    struct PriorityQueues *priorqueues=(struct PriorityQueues*)malloc(sizeof(struct PriorityQueues));
    priorqueues->queues=lista;

    

    /*
    struct PCB *first1 = get_del_next(lista);
    printf("-------------------------------\n");
    for (int i = MAX_PRIOR; i >= 0; i--)
        print_linkedlist(*(lista + i));
    printf("-------------------------------\n");
    struct PCB *first2 = get_del_next(lista);
    printf("-------------------------------\n");
    for (int i = MAX_PRIOR; i >= 0; i--)
        print_linkedlist(*(lista + i));
    printf("-------------------------------\n");
    struct PCB *first3 = get_del_next(lista);
    printf("-------------------------------\n");
    for (int i = MAX_PRIOR; i >= 0; i--)
        print_linkedlist(*(lista + i));
    printf("-------------------------------\n");
    struct PCB *first4 = get_del_next(lista);
    printf("-------------------------------\n");
    for (int i = MAX_PRIOR; i >= 0; i--)
        print_linkedlist(*(lista + i));
    printf("-------------------------------\n");

    printf("first1 = %i\n", first1->pid);
    printf("first2 = %i\n", first2->pid);
    printf("first3 = %i\n", first3->pid);
    printf("first4 = %i\n", first4->pid);

    // delete_last(lista);
    // delete_last(lista);
    /*
    struct PCB* first1=get_del_first(lista);
    struct PCB* first2=get_del_first(lista);
    struct PCB* first3=get_del_first(lista);
    struct PCB* first4=get_del_first(lista);
    printf("first1 = %i\n",first1->pid);
    printf("first2 = %i\n",first2->pid);
    printf("first3 = %i\n",first3->pid);
    printf("first4 = %i\n",first4->pid);
    print_linkedlist(lista);
    struct PCB* first5=get_del_first(lista);
    struct PCB* aaa=first1;
    printf("&first1 = %p\n",&first1);
    printf("&aaa = %p\n",&aaa);
    printf("first1 = %p\n",first1);
    printf("aaa = %p\n",aaa);
    printf("%i\n",aaa->pid);
    */
}