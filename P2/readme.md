# Práctica 2
Se basa en la creación de un sistema de archivos totalmente nuevo, añadiendo llamadas del sistema totalmente nuevas creadas por nosotros:
### Creación de un sistema de fichero
El sistema de archivos es una simplificación de UFS (Unix File System) enlazada, donde existe:
* SuperBloque: primer bloque del sistema de ficheros y almacena datos importantes del mismo.
* Bloque de mapa de i-nodos: es un bloque donde existe un mapa de bits, donde un uno significa que el bloque está ocupado y un cero implica que no lo está.
* Bloque de mapa de datos: similar al bloque de i-nodos pero para datos.
* Bloques de i-nodos: contiene los metadatos de un archivo.
* Bloques de datos: esta sección incluye los bloques encargados de almacenar la información de los diferentes ficheros.
### Funciones Básicas
* mKFS: función encargada de generar la estructura del sistema de ficheros diseñado.
* mountFS: se encarga de montar la partición del sistema de ficheros en memoria.
* unmountFS: método encargado de desmontar el sistema de fichero.
* createFile: función que crea un nuevo fichero vacío en el el sistema de ficheros.
* removeFile: esta función se implementa con el objetivo de que elimine un archivo del sistema de ficheros.
* openFile: el objetivo principal de esta función es el de crear un descriptor de fichero.
* closeFile: la funcionalidad que realiza este método es la de cerrar un archivo abierto en el sistema de ficheros.
* readFile: tiene como finalidad el leer el contenido de un fichero y devolver el número de bytes leídos.
* writeFile: esta función tiene la finalidad de escribir en un cierto fichero, un determinado número de bytes contenidos en un buffer como parámetro de entrada.
* lseekFile: el objetivo de esta función es el de modificar el valor del puntero de posición de un fichero.
### Funciones de integridad de fichero
* checkFile: es el encargado de comprobar la integridad de un fichero.
* includeIntegrity: función que se encarga de incluir la integridad en un fichero que no la tiene.
* openFileIntegrity: su principal funcionalidad es la de abrir un fichero con integridad realizando un check durante este proceso.
* closeFileIntegrity: su cometido es el de cerrar un archivo con integridad.
### Funciones de creación de enlaces simbólicos
* createLn: el objetivo de la función es el de crear un enlace simbólico a un archivo ya existen en el sistema de ficheros.
* removeLn: tiene como finalidad eliminar el enlace simbólico de un determinado fichero.
### Para compilar usar make.
### Desarrolladores:
* Gonzalo Fernández García
* Daniel Romero Ureña
* Marcelino Tena Blanco
### Nota: 8,4
