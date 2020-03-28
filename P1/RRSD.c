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

/*Incializo las colas de hilos listos*/
struct queue *cola_listos_alta;
struct queue *cola_listos_baja;

/* Inicializo la cola de bloqueados */
struct queue *cola_bloqueados;

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

int flag = 0;

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
  /*Si no se habia iniciado la cola de listos lo hace*/
  if (!cola_iniciada){
   cola_listos_baja = queue_new();
   cola_listos_alta = queue_new();
   cola_bloqueados = queue_new();
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
  t_state[0].ticks = 0;

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
  TCB *hilo_a_encolar = &t_state[i];
  /*HIGH priority es 1 y LOW_PRIORITY es 0*/
  int prioridad = hilo_a_encolar->priority;
  /*Deshabilito las interrupciones*/
  disable_interrupt();
  disable_disk_interrupt();
  /*Encolo el hilo en la cola de listos*/
  switch (prioridad) {
        case LOW_PRIORITY:
             enqueue(cola_listos_baja,hilo_a_encolar);
             break;
        case HIGH_PRIORITY:
             sorted_enqueue(cola_listos_alta, hilo_a_encolar, hilo_a_encolar->remaining_ticks);
             flag = 1;
             break;
        default:
             perror("La prioridad no es ni baja ni alta");
             return -1;
  }
  /*Habilito las interrupciones*/
  enable_disk_interrupt();
  enable_interrupt();
  return i;
}
/****** Fin de la Creación del Hilo  ******/

/* Llamada a la lectura de disco desde la función */
int read_disk()
{
  if(data_in_page_cache() ){
      /* Creo un nuevo TCB el cual será el de espera*/
      TCB *waiting = running;
      printf("*** THREAD %i READ FROM DISK\n",waiting->tid);
      /*Le cambio el estado de listo a esperando*/
      waiting->state = WAITING;
      /* Dehsabilito las interrupciones*/
      disable_interrupt();
      disable_disk_interrupt();
      /* Encolo el hilo en la cola de esperando*/
      enqueue(cola_bloqueados, waiting);
      /* Habilito de nuevo las interrupciones*/
      enable_disk_interrupt();
      enable_interrupt();
      /* Llamo al planificador para conocer el siguiente hilo*/
      TCB *siguiente = scheduler();
      /*Llamo al activador para proceder a ejecutarlo */
      activator(siguiente);
   }
   return 1;
}

/* Interrupción de disco*/
void disk_interrupt(int sig)
{
   /* Si la cola de bloqueados no está vacía*/
   if(!queue_empty(cola_bloqueados)){
   /* Consigo el primer hilo de la cola de desbloqueados*/
   TCB *hilo_desblock = dequeue(cola_bloqueados);
   /* Cambio su estado de esperando a listo*/
   hilo_desblock->state = INIT;
   printf ("*** THREAD %i READY\n", hilo_desblock->tid);
   /*Genero una variable de prioridad para el switch*/
   int prioridad = hilo_desblock->priority;
   /* Inhabilito las interrupciones*/
      disable_interrupt();
      disable_disk_interrupt();
   /* Según su prioridad la meto en una cola de listos u otra*/
   if(hilo_desblock->state != IDLE){
      switch(prioridad){
            case LOW_PRIORITY:
                  enqueue(cola_listos_baja, hilo_desblock);
                  break;
            case HIGH_PRIORITY:
                  sorted_enqueue(cola_listos_alta, hilo_desblock, hilo_desblock->remaining_ticks);
                  flag = 1;
                  break;
            default:
                  perror("La prioridad del hilo no es correcta");
                  return;
      }
   }
     /* Habilito las interrupciones*/
     enable_disk_interrupt();
     enable_interrupt();
  }
}


/* Libera el hilo terminado y llama al planificador y al activador */
void mythread_exit()
{
  /*Obtengo el tid del proceso actual*/
  int tid = mythread_gettid();
  /*Y lo libero*/
  t_state[tid].state = FREE;
  free(t_state[tid].run_env.uc_stack.ss_sp);
  /*En el caso de que el hilo fuera el último finalizado*/
  if(queue_empty(cola_listos_baja) && queue_empty(cola_listos_alta)){
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
  if( (queue_empty(cola_listos_baja)) && (queue_empty(cola_listos_alta)) && (queue_empty(cola_bloqueados)) ){
    printf("mythread_free: No thread in the system\nExiting...\n");
    printf("*** FINISH\n");
    exit(1);
  }
  TCB* siguiente;
  /* Deshabilito las interrupciones */
  disable_interrupt();
  disable_disk_interrupt();
  /* Desencolo el hilo siguiente*/
  if( !queue_empty(cola_listos_alta)){
   siguiente = dequeue(cola_listos_alta);
  }
  else if (!queue_empty(cola_listos_baja)){
   siguiente = dequeue(cola_listos_baja);
  }else{
   siguiente = &idle;
  }
  /* Habilito las interrupciones */
  enable_disk_interrupt();
  enable_interrupt();
  /*Devuelvo el proceso obtenido*/
  return siguiente;
}

/* Timer interrupt */
void timer_interrupt(int sig)
{
   if(flag){
      /* Deshabilito la marca */
      flag = 0;
      /* deshabilito las interrupciones*/
      disable_interrupt();
      disable_disk_interrupt();
      /* Obtengo el nuevo hilo de cola alta que se ha introducido*/
      TCB *siguiente = dequeue(cola_listos_alta);
      int prioridad_running = running->priority;
      /* Si el hilo nuevo tiene menos tiempo que el actual o es de prioridad menor, se cambia */
      if( (siguiente->remaining_ticks < running->remaining_ticks) || running->priority == LOW_PRIORITY){
          /* Selecciono la prioridad del hilo ejecutándose*/
          if(hilo_desblock->state != IDLE){
              switch(prioridad_running){
                case LOW_PRIORITY:
                  if(current != 0){
                      // Le devuelvo el QUANTUM
                      running->ticks = QUANTUM_TICKS;
                      enqueue(cola_listos_baja, running);
                  }
                  break;
                case HIGH_PRIORITY:
                  sorted_enqueue(cola_listos_alta, running, running->remaining_ticks);
                  break;
                default:
                  perror("No tiene una prioridad adecuada");
                  return;
              }
            } 
          /* Habilito las interrupciones*/
          enable_disk_interrupt();
          enable_interrupt();
          /* Llamo al activador*/
          activator(siguiente);
      /* Si no, se encola de nuevo el hilo nuevo */
      }else if(hilo_desblock->state != IDLE){
       sorted_enqueue(cola_listos_alta, siguiente, siguiente->remaining_ticks);
       /* Habilito las interrupciones*/
       enable_disk_interrupt();
       enable_interrupt();
      }
   }
   /* Para todos los hilos*/
   running->remaining_ticks = running->remaining_ticks - 1;
   /* Solo para hilos de alta prioridad*/
   if( (running->priority == HIGH_PRIORITY) ){
     return;
   }
   /*Reviso si se ha completado su rodaja de tiempo o en el
     caso de que la cola está vacia, pueda proseguir con
     su ejecucion*/
   if( (running->ticks >0) || (queue_empty(cola_listos_baja)) ) {
      running->ticks = running->ticks - 1;
      return;
   }
   /* Le devuelvo su QUANTUM */
   running->ticks = QUANTUM_TICKS;
   /* Dishabilito las interrupciones */
   if(hilo_desblock->state != IDLE){
      disable_interrupt();
      disable_disk_interrupt();
      /* Encolo el proceso de nuevo a la cola de listos */
      enqueue(cola_listos_baja,running);
      /*Habilito las interrupciones */
      enable_disk_interrupt();
      enable_interrupt();
   }
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
    /* Inicio el contexto siguiente sin guardar el anterior */
    printf ("*** THREAD %i TERMINATED: SETCONTEXT OF %i\n", oldRunning->tid, next->tid );
    setcontext (&(next->run_env));
    printf("mythread_free: After setcontext, should never get here!!...\n");
  }else{
    /*Si ha venido un hilo de alta prioridad cuando se estaba ejecutando uno de baja*/
    if( oldRunning->tid != 0 && oldRunning->priority == LOW_PRIORITY && next->priority == HIGH_PRIORITY){
         printf("*** THREAD %i PREEMTED : SETCONTEXT OF %i\n", oldRunning->tid, next->tid);
    /* En el caso de que el tid antiguo sea idle*/
    }else if (oldRunning->tid == -1){
         printf("*** THREAD READY : SET CONTEXT TO %i\n",next->tid);
    /* EN otros casos*/
    }else{
         printf("*** SWAPCONTEXT FROM %i TO %i\n", oldRunning->tid, next->tid);
    }
    /* Guardo el contexto anterior y inicio el siguiente*/
    swapcontext(&(oldRunning->run_env), &(next->run_env));
  }
}
