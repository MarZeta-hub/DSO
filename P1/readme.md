# Práctica 1
Se basa en la administracción de hilos mediante tres formas:
* Round-Robin: el planificador de hilos se basa en Round-Robin, haciendo que cada hilo sea ejecutado en rodajas de tiempo definidas mediante la variable QUANTUM_TICKS. La ejecución del hilo solo se detendrá cuando el proceso acabe su ejecución o cuando finalice su rodaja de tiempo.
* Round-Robin/SJF con prioridades: en este caso se intercalan dos tipos de planificación, haciéndose uso de una planificación Round-Robin para hilos de baja prioridad y SJF (primero el trabajo más corto) para los hilos de prioridad alta. En este caso se añade el hecho de que la ejecución de un hilo con prioridad baja pueda ser expulsado por un hilo de prioridad alta.
*	Round-Robin/SJF con posibles cambios de contexto voluntarios: este apartado es una ampliación del segundo apartado, añadiendo cambios de contexto debido a la función read_disk, lo que provocará el bloqueo del proceso, añadiéndose este a una cola de bloqueados.
### Para compilar modificar el contenido del makefile al codigo deseado, y luego realizar make.
### Desarrolladores:
* Gonzalo Fernández García
* Daniel Romero Ureña
* Marcelino Tena Blanco
### Nota: 8,9
