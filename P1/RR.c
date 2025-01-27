#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include "my_io.h"
#include "mythread.h"
#include "interrupt.h"
#include "queue.h"

/*Planificador que tiene como salida el siguiente hilo
  a ejecutar*/
TCB* scheduler();

/*El activador que cambia de contexto entre hilos*/
void activator();

/*El interruptor de tiempo, verifica la rodaja de tiempo*/
void timer_interrupt(int sig);

/*Interruptor de disco: simula una interrupción de hw*/
void disk_interrupt(int sig);

/*Array de estados de TCB: coloca cada hilo en una posición
  del array, el cual será su tid*/
static TCB t_state[N];

/*Incializo la cola de hilos listos*/
struct queue *cola_listos;

/* Es el hilo actual ejecutándose */
static TCB* running;

/*Tid del hilo actual*/
static int current = 0;

/* Indica si la libreria está inciada (init=1) o no (init=0) */
static int init=0;

/*Para conocer si la cola de listos está iniciada*/
static int cola_iniciada = 0;

/* TCB controlador del hilo ocioso*/
static TCB idle;

/*Función del IDLE*/
static void idle_function()
{
  while(1);
}

/*Función de los hilos*/
void function_thread(int sec){

  while(running->remaining_ticks){
   /*do something*/
  }
  /*Finaliza el hilo*/
  mythread_exit();
}


/* Initialize the thread library */
void init_mythreadlib()
{
  int i;
  /*Si no se habia iniciado la cola de listos lo hace*/
  if (!cola_iniciada){
   cola_listos = queue_new();
   cola_iniciada = 1;
  }

  /* Crea el contexto para le hilo IDLE */
  if(getcontext(&idle.run_env) == -1)
  {
    perror("*** ERROR: getcontext in init_thread_lib");
    exit(-1);
  }
  /*Crea los parámetros para el hilo IDLE*/
  idle.state = IDLE;
  idle.priority = SYSTEM;
  idle.function = idle_function;
  idle.run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
  idle.tid = -1;
  if(idle.run_env.uc_stack.ss_sp == NULL)
  {
    printf("*** ERROR: thread failed to get stack space\n");
    exit(-1);
  }
  idle.run_env.uc_stack.ss_size = STACKSIZE;
  idle.run_env.uc_stack.ss_flags = 0;
  idle.ticks = QUANTUM_TICKS;
  makecontext(&idle.run_env, idle_function, 1);

  t_state[0].state = INIT;
  t_state[0].priority = LOW_PRIORITY;
  t_state[0].ticks = QUANTUM_TICKS;

  if(getcontext(&t_state[0].run_env) == -1)
  {
    perror("*** ERROR: getcontext in init_thread_lib");
    exit(5);
  }

  for(i=1; i<N; i++)
  {
    t_state[i].state = FREE;
  }

  t_state[0].tid = 0;
  running = &t_state[0];
  //Inicializa las interrupciones
  init_interrupt();
  init_disk_interrupt();
}

/*Crea y inicializa un nuevo hilo con una función determinada y un tid específico*/
int mythread_create (void (*fun_addr)(),int priority,int seconds)
{
  int i;
  if (!init) { init_mythreadlib(); init=1;}
  /*Busca un hueco que este libre para que sea el tid*/
  for (i=0; i<N; i++)
    if (t_state[i].state == FREE) break;
  if (i == N) return(-1);
  if(getcontext(&t_state[i].run_env) == -1)
  {
    perror("*** ERROR: getcontext in my_thread_create");
    exit(-1);
  }
  t_state[i].state = INIT;        //Estado listo para ejecutar
  t_state[i].priority = priority; //Prioridad otorgada por parámetro
  t_state[i].function = fun_addr; //Función otorgada por parámetro
  t_state[i].execution_total_ticks = seconds_to_ticks(seconds);
  //convierto los segundos dados por ticks
  t_state[i].remaining_ticks = t_state[i].execution_total_ticks;
  //Los ticks que le faltan para acabar serás los ticks totales

  /*Le damos su rodaja de tiempo para cuando se ejecute*/
  t_state[i].ticks = QUANTUM_TICKS;
  t_state[i].run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
  if(t_state[i].run_env.uc_stack.ss_sp == NULL)
  {
    printf("*** ERROR: thread failed to get stack space\n");
    exit(-1);
  }
  t_state[i].tid = i; //Su tid será el encontrado anteriormente
  t_state[i].run_env.uc_stack.ss_size = STACKSIZE;
  t_state[i].run_env.uc_stack.ss_flags = 0;

  /*Creamos el contexto del hilo*/
  makecontext(&t_state[i].run_env, fun_addr,2,seconds);

  /*Deshabilito las interrupciones*/
  disable_interrupt();
  disable_disk_interrupt();
  /*Encolo el hilo en la cola de listos*/
  enqueue(cola_listos,&t_state[i]);
  /*Habilito las interrupciones*/
  enable_disk_interrupt();
  enable_interrupt();
  return i;
}
/****** Fin de la Creación del Hilo  ******/

/* Llamada a la lectura de disco*/
int read_disk()
{
   return 1;
}

/* Interrupción de disco*/
void disk_interrupt(int sig){}


/* Libera el hilo terminado y llama al planificador y al activador */
void mythread_exit()
{
  /*Obtengo el tid del proceso actual*/
  int tid = mythread_gettid();
  /*Y lo libero*/
  t_state[tid].state = FREE;
  free(t_state[tid].run_env.uc_stack.ss_sp);
  /*En el caso de que el hilo fuera el último finalizado*/
  if(queue_empty(cola_listos)){
    printf("*** THREAD %d FINISHED\n", tid);
  }
  /*Llamo al planificador para conocer el siguiente hilo*/
  TCB* next = scheduler();
  /*Llamo al activador para cambiar de contexto*/
  activator(next);
}

/*Elimina el hilo si tiene problemas*/
void mythread_timeout(int tid)
{
    printf("*** THREAD %d EJECTED\n", tid);
    t_state[tid].state = FREE;
    free(t_state[tid].run_env.uc_stack.ss_sp);
    TCB* next = scheduler();
    activator(next);
}


/* Selecciona la prioridad del hilo */
void mythread_setpriority(int priority)
{
  int tid = mythread_gettid();
  t_state[tid].priority = priority;
  if(priority ==  HIGH_PRIORITY){
    t_state[tid].remaining_ticks = 195;
  }
}


/* Devuelve la prioridad de la hilo */
int mythread_getpriority(int priority)
{
  int tid = mythread_gettid();
  return t_state[tid].priority;
}


/* Obtiene el id del hilo actual */
int mythread_gettid()
{
  if (!init) { init_mythreadlib(); init=1;}
  return current;
}


/* SJF para alta prioridad, RR para baja*/
TCB* scheduler()
{
  /* Finalizo el programa si no quedan hilos en la cola */
  if(queue_empty(cola_listos)){
    printf("mythread_free: No thread in the system\nExiting...\n");
    printf("*** FINISH\n");
    exit(1);
  }
  /* Deshabilito las interrupciones */
  disable_interrupt();
  disable_disk_interrupt();
  /* Desencolo el hilo siguiente */
  TCB* siguiente = dequeue(cola_listos);
  /* Habilito las interrpciones */
  enable_disk_interrupt();
  enable_interrupt();
  /* Devuelvo el proceso obtenido */
  return siguiente;
}

/* Timer interrupt */
void timer_interrupt(int sig)
{
   /*Reviso si se ha completado su rodaja de tiempo o en el
     caso de que la cola está vacia, pueda proseguir con
     su ejecucion*/
   if( (running->ticks > 0) || (queue_empty(cola_listos)) )
   {
      running->ticks = running->ticks - 1;
      running->remaining_ticks = running->remaining_ticks - 1;
      return;
   }
   /* Le devuelvo su QUANTUM */
   running->ticks = QUANTUM_TICKS;
   /* Dishabilito las interrupciones */
   disable_interrupt();
   disable_disk_interrupt();
   /* Encolo el proceso de nuevo a la cola de listos */
   enqueue(cola_listos,running);
   /*Habilito las interrupciones */
   enable_disk_interrupt();
   enable_interrupt();
   /*Llamo al planificador para conocer el siguiente hilo*/
   TCB* next = scheduler();
   /*Llamamos al activador*/
   activator(next);
 }

/* Activator */
void activator(TCB* next)
{
  /*Creo un nuevo TCB para guardar el hilo anterior*/
  TCB* oldRunning = running;
  /*Cambio el hilo ejecutando al siguiente*/
  running = next;
  /*Cambio el tid de current*/
  current = running->tid;
  if(oldRunning->state == FREE){
    /*Inicio el contexto siguiente sin guardar el anterior*/
    printf ("*** THREAD %i TERMINATED: SETCONTEXT OF %i\n", oldRunning->tid, next->tid );
    setcontext (&(next->run_env));
    printf("mythread_free: After setcontext, should never get here!!...\n");
  }else{
    if (oldRunning->tid != -1){
       printf("*** SWAPCONTEXT FROM %i TO %i\n", oldRunning->tid, next->tid);
    }
    else{
       printf ("*** THREAD READY: SET CONTEXT TO %i\n",next->tid);
    }
    /*Guardo el contexto anterior y inicio el siguiente*/
    swapcontext(&(oldRunning->run_env), &(next->run_env));
  }
}
