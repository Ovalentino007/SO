#ifndef scheduler_h
#define scheduler_h

#include "structures.h"

typedef enum {
    ROUND_ROBIN,
    PRIORITY
} SchedulePolicy;

typedef struct{
    ProcessQueue *queue;
    int quantum;
    Machine *machine;
    SchedulePolicy policy;
}ScheduleConfig;

void *scheduler_function(void *arg);
void schedule_round_robin(ProcessQueue *queue, Machine *machine, int quantum);
void schedule_priority(ProcessQueue *queue);
Thread *find_available_thread(Machine *machine);
void update_process_state(PCB *proceso, ProcessState new_state);
void notify_scheduler();
void dispatch_process(ProcessQueue *queue, Machine *machine);
void stop_scheduler();
//void dispatch_process(ProcessQueue *queue);
#endif