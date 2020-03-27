#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include "my_io.h"

//#include "mythread.h"
#include "interrupt.h"

#include "queue.h"

TCB* scheduler();
void activator();
void timer_interrupt(int sig);
void disk_interrupt(int sig);

/* Array of state thread control blocks: the process allows a maximum of N threads */
static TCB t_state[N];

/*Incializo la cola de prioridad baja de hilos listos*/
struct queue *cola_baja_listos;

/*Incializo la cola de prioridad alta de hilos listos*/
struct queue *cola_alta_listos;

/* Current running thread */
static TCB* running;

/*Tid del hilo actual*/
static int current = 0;

/* Variable indicating if the library is initialized (init == 1) or not (init == 0) */
static int init=0;

/*Para conocer si la cola de listos está iniciada*/
static int colas_listos_iniciadas = 0;

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
  /*Si no se habia iniciado la cola de listos de baja prioridad lo hace*/

  if (!colas_listos_iniciadas){
   cola_baja_listos = queue_new();
   cola_alta_listos = queue_new();
   colas_listos_iniciadas = 1;
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

  if(t_state[i].priority != LOW_PRIORITY && t_state[i].priority != HIGH_PRIORITY){
    perror("*** ERROR: prioridad no valida");
    exit(-1);
  }
  /*Si el nuevo hilo es de baja prioridad lo encolo en la lista de listos de baja prioridad */
  if(t_state[i].priority == LOW_PRIORITY){
    disable_interrupt();
    disable_disk_interrupt();
    enqueue(cola_baja_listos,&t_state[i]);
    enable_disk_interrupt();
    enable_interrupt();
  }
  else if(t_state[i].priority == HIGH_PRIORITY){
    disable_interrupt();
    disable_disk_interrupt();
    /*Encolamos ordenadamente el nuevo hilo en la cola de alta prioridad*/
    sorted_enqueue (cola_alta_listos,&t_state[i], t_state[i].remaining_ticks);
    enable_disk_interrupt();
    enable_interrupt();
    /*Si se ejecutata un hilo de baja prioridad se cambia por el nuevo de alta*/
    if(running->priority == LOW_PRIORITY) {
        TCB* next = scheduler();
        int anterior_tid = running->tid;
        printf("*** THREAD <%i> PREEMTED : SETCONTEXT OF <%i>\n", anterior_tid, next->tid);
        /*llamamos al activador para que realize el cambio de contexto*/
        activator(next);
    }
    /*Si se ejecuta un hilo de alta prioridad pero mas largo que el nuevo se sustituye por este*/
    else if(running->remaining_ticks > t_state[i].remaining_ticks){
        TCB* next = scheduler();   
        int anterior_tid = running->tid;
        printf("*** SWAPCONTEXT FROM <%i> TO <%i>", anterior_tid, next->tid);
        /*llamamos al activador para que realize el cambio de contexto*/
        activator(next);
    }
  }
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
void mythread_exit() {
  int tid = mythread_gettid();
  t_state[tid].state = FREE;
  free(t_state[tid].run_env.uc_stack.ss_sp);
  if(queue_empty(cola_baja_listos) && queue_empty(cola_alta_listos)){
    printf("*** THREAD %d FINISHED\n", tid);
  }
  TCB* next = scheduler();
  printf ("*** THREAD  <%i> TERMINATED: SETCONTEXT  OF <%i>\n",tid, next->tid );
  activator(next);
}


void mythread_timeout(int tid) {
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
int mythread_gettid(){
  if (!init) { init_mythreadlib(); init=1;}
  return current;
}


/* SJF para alta prioridad, RR para baja*/
TCB* scheduler()
{
  if(queue_empty(cola_baja_listos)&&queue_empty(cola_alta_listos)){
    printf("mythread_free: No thread in the system\nExiting...\n");
    exit(1);
  }
  /*Desencolo el hilo actual porque va a ser el siguiente en ejecutarse*/
  disable_interrupt();
  TCB* siguiente;
  if(!queue_empty(cola_alta_listos)){
   siguiente = dequeue(cola_alta_listos);
  }
  else{
    siguiente = dequeue(cola_baja_listos);
  }
  enable_interrupt();
  return siguiente;
}


/* Timer interrupt */
void timer_interrupt(int sig)
{
   /*Si el hilo es de alta prioridad se sigue ejecutando*/
   if(running->priority==HIGH_PRIORITY){
     running->remaining_ticks = running->remaining_ticks - 1;
     return;
   }
    /*reviso si se ha completado su rodaja de tiempo o en el caso de que la cola está 
     vacia, pueda proseguir con su ejecucion*/
   else if((running->priority==LOW_PRIORITY && (running->ticks < QUANTUM_TICKS || queue_empty(cola_alta_listos)))){
      running->ticks = running->ticks + 1;
      running->remaining_ticks = running->remaining_ticks - 1;
      return;
   }
   running->ticks = 0;
   /*Encolo el hilo acutal porque no ha acabado*/
   int anterior_tid = running->tid;
   int prioridad_anterior = running->priority;
   /*Desabilito las interrupciones para poder agregar el hilo a la cola*/
   disable_interrupt();
   /*Devuelvo el hilo a la cola de listos de alta o baja prioridad segun corresponda*/
   if(running->priority==LOW_PRIORITY){
    enqueue(cola_baja_listos,running);
   }
   else if(running->priority==HIGH_PRIORITY){
    sorted_enqueue ( cola_alta_listos, running, running->remaining_ticks );
   }
   /*Habilito las interrupciones de nuevo*/
   enable_interrupt();
   /*Llamo al planificador para conocer el siguiente hilo*/
   TCB* next = scheduler();
   if(prioridad_anterior==LOW_PRIORITY && next->priority==HIGH_PRIORITY && anterior_tid != 0){
    printf("*** THREAD <%i> PREEMTED : SETCONTEXT OF <%i>\n", anterior_tid, next->tid);
   }
   else if(anterior_tid != 0){
    printf("*** THREAD READY : SET CONTEXT TO <%i>", next->tid);
   }
   else{
    printf("*** SWAPCONTEXT FROM <%i> TO <%i>\n", anterior_tid, next->tid);
   }
   /*llamamos al activador para que realize el cambio de contexto*/
   activator(next);
 }


/* Activator */
void activator(TCB* next)
{
  running = next;
  current = running->tid;
  /*Cambio de contexto por le contexto seleccionado en el planificador*/
  setcontext (&(next->run_env));
  printf("mythread_free: After setcontext, should never get here!!...\n");
}


