#ifndef timer_h
#define timer_h

typedef struct{
    int frecuencia_tick;
}Timerconf;

void *timer_function(void *arg);
void stop_timer();

#endif 