#!/usr/bin/env python

import threading
import time

GREEN = '\033[32m'
BRIGHT_BLUE = '\033[94m'
RESET = '\033[0m' # called to return to standard terminal text color

def ping():
	for x in range(5):
		sem_pong.acquire()
		print("Ping")
		time.sleep(1)
		sem_ping.release()

'''def cliente:
	preparar_peticion
	signal(sem_servidor)
	wait(sem_respuesta)
	recoger_respuesta

def servidor:
	wait(sem_servidor)
	recoger_peticion + procesar --> genera respuesta
	escribir_respuesta
	signal(sem_respuesta)
'''
def pong():
	for x in range(5):
		sem_ping.acquire()
		print("Pong")
		time.sleep(1)
		sem_pong.release()

sem_ping = threading.Semaphore(value=0) #pong debe esperar hasta que ping le avise
sem_pong = threading.Semaphore(value=1) #ping debe esperar hasta que pong le avise
t_hilo_ping = threading.Thread(target = ping).start()
t_hilo_pong = threading.Thread(target = pong).start()

exit(0)
