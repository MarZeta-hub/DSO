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

TCB* scheduler();
void activator();
void timer_interrupt(int sig);
void disk_interrupt(int sig);


/* Array of state thread control blocks: the process allows a maximum of N threads */
static TCB t_state[N];

/*Incializo la cola de hilos listos*/
struct queue *cola_listos;

/* Current running thread */
static TCB* running;

/*Tid del hilo actual*/
static int current = 0;

/* Variable indicating if the library is initialized (init == 1) or not (init == 0) */
static int init=0;

/*Para conocer si la cola de listos está iniciada*/
static int cola_iniciada = 0;

/* Thread control block for the idle thread */
static TCB idle;


static void idle_function()
{
  while(1);
}

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
  /* Create context for the idle thread */
  if(getcontext(&idle.run_env) == -1)
  {
    perror("*** ERROR: getcontext in init_thread_lib");
    exit(-1);
  }

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
  //Initialize disk and clock interrupts
  init_disk_interrupt();
  init_interrupt();
}


/* Create and intialize a new thread with body fun_addr and one integer argument */ 
int mythread_create (void (*fun_addr)(),int priority,int seconds)
{
  int i;
  if (!init) { init_mythreadlib(); init=1;}
  /*Si no se habia iniciado la cola de listos lo hace*/
  if (!cola_iniciada){
   cola_listos = queue_new();
   cola_iniciada = 1;
  }
  for (i=0; i<N; i++)
    if (t_state[i].state == FREE) break;

  if (i == N) return(-1);

  if(getcontext(&t_state[i].run_env) == -1)
  {
    perror("*** ERROR: getcontext in my_thread_create");
    exit(-1);
  }
  t_state[i].state = INIT;
  t_state[i].priority = priority;
  t_state[i].function = fun_addr;
  t_state[i].execution_total_ticks = seconds_to_ticks(seconds);
  t_state[i].remaining_ticks = t_state[i].execution_total_ticks;
  /*Le damos su rodaja de tiempo para cuando se ejecute*/
  t_state[i].ticks = QUANTUM_TICKS;
  t_state[i].run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
  if(t_state[i].run_env.uc_stack.ss_sp == NULL)
  {
    printf("*** ERROR: thread failed to get stack space\n");
    exit(-1);
  }
  t_state[i].tid = i;
  t_state[i].run_env.uc_stack.ss_size = STACKSIZE;
  t_state[i].run_env.uc_stack.ss_flags = 0;
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
/****** End my_thread_create() ******/

/* Read disk syscall */
int read_disk()
{
   return 1;
}

/* Disk interrupt  */
void disk_interrupt(int sig)
{

}


/* Free terminated thread and exits */
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


void mythread_timeout(int tid)
{
    printf("*** THREAD %d EJECTED\n", tid);
    t_state[tid].state = FREE;
    free(t_state[tid].run_env.uc_stack.ss_sp);
    TCB* next = scheduler();
    activator(next);
}


/* Sets the priority of the calling thread */
void mythread_setpriority(int priority)
{
  int tid = mythread_gettid();
  t_state[tid].priority = priority;
  if(priority ==  HIGH_PRIORITY){
    t_state[tid].remaining_ticks = 195;
  }
}

/* Returns the priority of the calling thread */
int mythread_getpriority(int priority)
{
  int tid = mythread_gettid();
  return t_state[tid].priority;
}


/* Get the current thread id.  */
int mythread_gettid()
{
  if (!init) { init_mythreadlib(); init=1;}
  return current;
}


/* SJF para alta prioridad, RR para baja*/
TCB* scheduler()
{
  /*Finalizo*/
  if(queue_empty(cola_listos)){
    printf("mythread_free: No thread in the system\nExiting...\n");
    printf("*** FINISH\n");
    exit(1);
  }
  /*Deshabilito la interrupción de tiempo */
  disable_interrupt();
  /*Deshabilito la interrupción de disco*/
  disable_disk_interrupt();
  /*Desencolo el hilo siguiente*/
  TCB* siguiente = dequeue(cola_listos);
  /*Habilito la interrupción de disco*/
  enable_disk_interrupt();
  /*Habilito la interrupción de tiempo*/
  enable_interrupt();
  return siguiente;
}


/* Timer interrupt */
void timer_interrupt(int sig)
{
   /*Reviso si se ha completado su rodaja de tiempo o en el caso de que la cola está 
     vacia, pueda proseguir con su ejecucion*/
   if(running->ticks || queue_empty(cola_listos)){
   /*Para caso de pruebas, si se salta el while de tiempo restante != 0 de la función*/
      if(running->remaining_ticks < 1){
         mythread_timeout(running->tid);
      }
      running->ticks = running->ticks - 1;
      running->remaining_ticks = running->remaining_ticks - 1;
      return;
   }
   running->ticks = QUANTUM_TICKS;
   /*Desabilito las interrupciones para poder agregar el hilo a la cola*/
   disable_interrupt();
   disable_disk_interrupt();
   /*Devuelvo el hilo a la cola de listos*/
   enqueue(cola_listos,running);
   /*Habilito las interrupciones de nuevo*/
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
    printf ("*** THREAD <%i> TERMINATED: SETCONTEXT OF <%i>\n", oldRunning->tid, next->tid );
    setcontext (&(next->run_env));
    printf("mythread_free: After setcontext, should never get here!!...\n");
  }else{
    /*Guardo el contexto anterior y inicio el siguiente*/
    printf("*** SWAPCONTEXT FROM <%i> TO <%i>\n", oldRunning->tid, next->tid);
    swapcontext(&(oldRunning->run_env), &(next->run_env));
  }
}
