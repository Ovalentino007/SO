#ifndef scheduler_h
#define scheduler_h

#include "structures.h"

typedef enum {
    ROUND_ROBIN,
    PRIORITY
} SchedulePolicy;

typedef struct{
    ProcessQueue *queue;
    Machine *machine;
    SchedulePolicy policy;
}ScheduleConfig;

void *scheduler_function(void *arg);
void schedule_round_robin(ProcessQueue *queue, Machine *machine);
void schedule_priority(ProcessQueue *queue);
Thread *find_available_thread(Machine *machine);
void notify_scheduler();
void dispatch_process(ProcessQueue *queue, Machine *machine);
void stop_scheduler();
//void dispatch_process(ProcessQueue *queue);
#endif