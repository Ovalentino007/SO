#ifndef proc_generator_h
#define proc_generator_h

typedef struct {
    int frecuencia_gen;
    int run;
}Proc_generator_conf;

void *proc_generator_function(void *arg);
void stop_proc_generator();

#endif 