#!/usr/bin/env python

import threading
import time

GREEN = '\033[32m'
BRIGHT_BLUE = '\033[94m'
RESET = '\033[0m' # called to return to standard terminal text color

def func_hilo(num):
    print("\t Thread ", num, " starting")

    sem_1.acquire()
    print(GREEN,"\t\t Thread ", num, " enters into critical section",RESET)
    time.sleep(5)
    print(BRIGHT_BLUE,"\t\t Thread ", num, " leaves critical section",RESET)
    sem_1.release()

    print("\t Thread ", num, " finishing")

sem_1 = threading.Semaphore(value=1)
for num_hilo in range(5):
    t_hilo = threading.Thread(target = func_hilo, args=(num_hilo,))
    t_hilo.start()

exit(0)
