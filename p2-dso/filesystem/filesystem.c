
/*
 * Operating System Design / Diseno de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	Last revision 01/04/2020
 */

#include "filesystem/filesystem.h" // Headers for the core functionality
#include "filesystem/auxiliary.h"  // Headers for auxiliary functions
#include "filesystem/metadata.h"   // Type and structure declaration of the file system
#include <string.h>
#include <stdlib.h>

/*
 * Genera la estructura del sistema de ficheros disenada.
 * Entrada:Tamano del disco a dar formato, en bytes.
 * Salida: Devuelve 0 si es correcto y -1 si es un error.
 */
int mkFS(long deviceSize)
{
 	//Superbloque (SB): devuelve el char con 2048B con los datos del superbloque
	char contenido[BLOCK_SIZE]; //El contenido del superbloque
	sbloque = malloc(sizeof sbloque[0]); //añado espacio
	createSuperBloque(deviceSize, contenido); //Añadir todos el contenido al superbloque
	//Escribo el contenido del superbloque en el primer bloque
	if( bwrite(DEVICE_IMAGE, 0, contenido) == -1) {
		perror("Fallo al escribir el SuperBloque mkFS\n");
		return -1; //En caso de que de fallo la escritura
	}

	//MAPA DE BITS DE INODOS
	i_map = malloc(sbloque[0].numBloquesInodos) ; //Creamos el bloque de Inodos
	for( int i; i<sbloque[0].numBloquesInodos; i++){
		bitmap_setbit(i_map, i, 0);
	}
	//para agregar el MAPA DE INODOS
	if( bwrite(DEVICE_IMAGE, 1, i_map) == -1) {
		perror("Fallo al escribir el BitMap de inodos\n");
		return -1; //En caso de que de fallo la escritura
	}

	//Crear el MAPA DE BITS DE DATOS
	b_map = malloc(sbloque[0].numBloquesDatos);
	for( int i = 0; i<sbloque[0].numBloquesDatos; i++){
		bitmap_setbit(b_map, i, 0);
	}
	//Agregar el bloque EOF
	bitmap_setbit(b_map, sbloque[0].numBloquesDatos, 1); //El último bloque de datos es EOF
	//Para agregar  el MAPA DE BLOQUES
	if( bwrite(DEVICE_IMAGE, 2, b_map) == -1) {
		perror("Fallo al escribir el BitMap de Datos\n");
		return -1; //En caso de que de fallo la escritura
	}

	//EL BLOQUE DE FINAL DE FICHERO
	char* buffer = EOF_SYSTEM;  //Contenido del bloque EOF
	//Lo guardo en el disco. El -1 es debido a que el primerInodo se cuenta dos veces
	short bloquefinal = sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1 ;	
	if( bwrite(DEVICE_IMAGE, bloquefinal,buffer) == -1){
		perror("Error al grabar el bloque EOF");
		return -1;
	}

	//INODOS: FORMATEAR TODOS LOS INODOS
 	inodos = malloc(sizeof inodos[0]); //añado espacio
	inodoVacio(0); //creo un inodo vacio
	inodotoChar(contenido, 0); //Añadir todos el contenido al nodo
	//Escribo el contenido del superbloque en el primer bloque
	for (int i = 0; i < sbloque[0].numBloquesInodos; i++){
		if( bwrite(DEVICE_IMAGE, sbloque[0].primerInodo + i, contenido) == -1) {
			perror("Fallo al formatear los INODOS mkFS\n");
			return -1; //En caso de que de fallo la escritura
		}
	}

	//Libero memoria principal
	free(i_map); //Vacío el mapa de inodosç
	free(b_map); //Vacio el mapa de datos
	free(inodos); //Vacio los inodos
	free(sbloque); //Libero la estructura de superbloque
	return 0;
}



/**
 * Monta la particion en la memoria
 * Entrada: Nada
 * Salida: 0 si es correcto y salida -1 si es incorrecto
*/
int mountFS(void)
{
	//SUPERBLOQUE
	char contenidoSB[BLOCK_SIZE];
	if( bread(DEVICE_IMAGE, 0, contenidoSB ) == -1) {return -1;}
	//Creo espacio para guardar el SUPERBLOQUE
	sbloque = malloc(sizeof sbloque);
	//Pasamos los datos a sbloque, es decir, a la memoria
	chartoSB(contenidoSB);

	//MAPAS DE BITS INODOS
	char mapAux[BLOCK_SIZE];
	//Leo de disco del bloque 1
	if( bread(DEVICE_IMAGE, 1, mapAux) == -1){
		perror("Error de lectura de Mapa de bits de INODOS");
		return -1;
	}
	//Añado memoria principal al puntero de mapasInodos
	i_map = malloc(sbloque[0].numBloquesInodos);
	//Copio los datos obtenidos
	memcpy(i_map, mapAux, sbloque[0].numBloquesInodos);

	//Mapa de BLOQUES DE DATOS
	//Leo de disco del bloque 2
	if( bread(DEVICE_IMAGE, 2, mapAux) == -1){
		perror("Error de lectura de Mapa de bits de Datos");
		return -1;
	}	
	//Añado memoria principal al puntero de mapasDatos
	b_map = malloc(sbloque[0].numBloquesDatos);
	//Copio los datos obtenidos
	memcpy(b_map, mapAux, sbloque[0].numBloquesInodos);


	//INODOS
	char inodosContent[BLOCK_SIZE];
	inodos = malloc((sizeof inodos[0]) * sbloque[0].numBloquesInodos);
	for(int i=0; i < (sbloque[0].numBloquesInodos); i++) {
		if( bread(DEVICE_IMAGE, sbloque[0].primerInodo + i, inodosContent) == -1) {return -1;} 
		chartoInodo(inodosContent, i);	
	}
	printfSB();
	return 0;
}



/**
 * Desmonta el sistema de ficheros de memoria
 * Entrada: Nada
 * Salida: 0 si es correcto -1 si es incorrecto
*/
int unmountFS(void)
{
	if( syncDisk() == -1){
		perror("Error al sincronizar con el disco");
		return -1;
	} 

	//Desmontamos los datos del disco en memoria
  	free(inodos);
	free (sbloque);
	free (i_map);
    free (b_map);
	return 0;

}


/***
 * Sincroniza las estructuras cargadas en memoria con el disco
 * Entrada: nada
 * Salida; 0 si es correcto y -1 si es incorrecto
 *  */
int syncDisk(){
	//Escribir en disco el SuperBloque
	char contenido[2048];
	sBtoChar(contenido);
	if(bwrite(DEVICE_IMAGE, 0, contenido) == -1){
		perror("La escritura en disco no se ha llevado a cabo");
		return -1;
	}
	
	//Escribir el BitMap de inodos
	memset(contenido,'\0',2048);
	memcpy(contenido, i_map, strlen(i_map));
	if(bwrite(DEVICE_IMAGE,1, contenido) == -1 ){
		perror("La escritura en disco para el BitMap Inode no ha sido satisfactoria");
		return -1;
	}

	//Escribir el BitMap de datos
	memset(contenido,'\0',2048);
	memcpy(contenido, b_map, strlen(i_map));
	if(bwrite(DEVICE_IMAGE,2,contenido) == -1 ){
		perror("La escritura en disco para el BitMap Datos no ha sido satisfactoria");
		return -1;
	}

	//Escribir los nodos modificados
	for(int i=0; i < (sbloque[0].numBloquesInodos); i++) {
		memset(contenido,'\0',2048);
		inodotoChar(contenido, i);
		if( bwrite(DEVICE_IMAGE,i + sbloque[0].primerInodo, contenido) == -1) {
			perror("Error al escribir los nodos");
			return -1;
		} 	
	}	
	return 0;
}



/*
 * Crea un nuevo archivo sin datos
 * Entrada: nombre del archivo
 * Salida: 0 Si es correcto, -1 si ya existe el nombre y -2 otros errores
 */
int createFile(char *fileName)
{
	//El máximo de ficheros es 48
	if(sbloque[0].numFicheros==48){
		perror("Máximo de ficheros alcanzado");
		return -2;
	}
	//Compruebo si el tamaño del nombre es correcto
	if (checkTamanoNombre( fileName) == -1) return -2;
	//Compruebo si el nombre del archivo existe en el sistema
	//devuelve el nodo si existe el fichero, si no devuelve -1
	if (existeFichero(fileName) != -1){
		perror("El nombre de archivo ya existe");
		return -1;
	} 

	//Buscar un bloque vacio
	int dato_libre = -1;
	for(int i = 0; i< sbloque[0].numBloquesDatos; i++){
		if(bitmap_getbit(b_map, i) == 0){
			dato_libre = i;
			break;
		}
	}
	if(dato_libre == -1){
		perror("No hay bloque de datos libres");
		return -2;
	}
	//Busco un inodo que este libre
	int inodo_libre=-1;
	for(int i=0; i<sbloque[0].numBloquesInodos; i++){
		if(bitmap_getbit(i_map,i) ==0){
			inodo_libre = i;
			break;
		}
	}
	//En el caso de que no exista un inodo libre
	if(inodo_libre == -1){
		perror("No hay i-nodos libres");
		return -2;
	}
	//CREAR EL INODO
	memcpy(inodos[inodo_libre].nomFichero, fileName, strlen(fileName)); //le añado el nuevo nombre
	inodos[inodo_libre].referencia[0] = dato_libre; //le añado el nuevo bloque al inodo
	//El último bloque será el de EOF
	inodos[inodo_libre].referencia[1] = sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1 ;
	bitmap_setbit(i_map, inodo_libre, 1); //Ocupo en el mapa de bits el nuevo bloque
	bitmap_setbit(b_map, dato_libre, 1); //Ocupo en el mapa de bits el fichero
	sbloque[0].numFicheros = sbloque[0].numFicheros + 1; //Añado al superbloque un nuevo archivo

	printfSB();
	printfInodo();
	return 0;
}



/*
 * Elimina un archivo del disco
 * Entrada: nombre del archivo
 * Salida: 0 si es correcto, -1 si no existe el archivo y -2 otros errores
 */
int removeFile(char *fileName)
{	
	//LLAMAR AL DESCRIPTOR DE FICHEROS
	//Comprobar si el tamaño del nombre es correcto
	if(checkTamanoNombre(fileName) != 0) return -2;
    //Busco donde está el fichero
	short indiceFichero = existeFichero(fileName);
	//Buscar el inodo con el nombre if si no existe return -1
	if (indiceFichero == -1) {
		perror("El fichero no existe");
		return -1;
	}
	//Formateo el nodo
	inodoVacio(indiceFichero);
	//Actualizar el mapa de bits de datos y de inodos
	bitmap_setbit(i_map, indiceFichero,0);
	bitmap_setbit(b_map,indiceFichero,0);
	sbloque[0].numFicheros = sbloque[0].numFicheros - 1;

	return 0;
}



/*
 * Pasa los datos de un archivo a la memoria
 * Entrada: nombre del archivo
 * Salida: descriptor de entrada -1 no existe en el sistema y  -2 otros errores
 */
int openFile(char *fileName)
{
	//Compruebo el tamaño del nombre que me han pasado
	if (checkTamanoNombre(fileName) != 0){
		perror("El tamaño del nombre es incorrecto");
		return -2;
	} 
	short indiceFichero = existeFichero(fileName);
	if (indiceFichero == -1) {
		perror("El fichero no existe");
		return -1;
	}
	if( strcmp(fileDescriptor[indiceFichero].nombre, fileName) == 0){
		perror("el fichero ya está abierto");
		return -2;
	}

	memcpy(fileDescriptor[indiceFichero].nombre, fileName, strlen (fileName));
	fileDescriptor[indiceFichero].punteroRW = 0;
	fileDescriptor[indiceFichero].punteroArchivo = malloc(BLOCK_SIZE);
	if (bread(DEVICE_IMAGE, inodos[indiceFichero].referencia [0], fileDescriptor[indiceFichero].punteroArchivo)){
		perror("Error de la lectura");
		return -1;
	}
	
	//añadir una funcion para leer el op
	printf("%s, %s, %i", fileDescriptor[indiceFichero].nombre, fileDescriptor[indiceFichero].punteroArchivo, fileDescriptor[indiceFichero].punteroRW);

	return 0;
}



/**
 * Cierra un fichero de datos en memoria
 * Entrada: descriptor de archivos
 * Salida: o si es correcto y -1 si es erronea
*/
int closeFile(int fileDescriptor) {
	//puntero int* datos = fd, 
	//free datos


	return -1;
}



int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}



/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}



/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	return -1;
}



/*
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile (char * fileName)
{
    return -2;
}



/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */

int includeIntegrity (char * fileName)
{
    return -2;
}



/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName)
{

    return -2;
}



/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor)
{
    return -1;
}



/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName)
{
    return -1;
}



/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */
int removeLn(char *linkName)
{
    return -2;
}



/**
 * Comprueba el tamaño del nombre del fichero
 * Entrada: el nombre del fichero
 * Salida: 0 si es correcto, -1 si es incorrecta
*/
int checkTamanoNombre(char* fileName){
	int tamano = strlen(fileName) ;
	if(tamano > 32 || tamano == 0){
		perror("Tamaño del nombre de archivo incorrecto");
		return -1;
	}
	return 0;
}



/**
 * Comprueba si existe el fichero
 * Entrada: Nombre del fichero
 * Salida: -1 si no se encuentra y el nodo si ya existe
*/
int existeFichero(char* fileName){
	for(int i = 0; i< sbloque[0].numBloquesInodos; i++){
		if(strcmp(inodos[i].nomFichero, fileName) == 0 ){
			return i; //te devuelvo el nodo donde está el fichero
		}
	}
	//En el caso de que no exista en fichero
	return -1;
}



/**
 * Busca si el archivo tiene ya un descriptor de ficheros
 * Entrada: el nombre del fichero
 * Salida: el descriptor de fichero o -1 si no se encuentra
*/
int searchFD(char* fileName){
	for(int i = 0; i < (sizeof(fileDescriptor) / sizeof(fileDescriptor[0])); i++ ){
		if(strcmp(fileDescriptor[i].nombre, fileName)){
			return i;
		}
	}
	return -1;
}



/**
 * Crea un nuevo superbloque a partir de los parámetros dados y
 * almacena la información en un array de caracteres
 * Entrada: El tamaño del disco y un array de contenido vacío
 * Salida: nada
*/
void createSuperBloque(int tamanoDisco, char* contenidoSB){ 
	//Obtener el número de bloques TipoSuperbloque super
	int numBloques = tamanoDisco/BLOCK_SIZE; //para obtener el número de bloques
	sbloque[0].numMagico = 100383266; 
	sbloque[0].primerInodo = 3;	//El primer bloque de inodos SuperBloque 0, Mapa 1 y mapa 2
	sbloque[0].numBloquesMapaInodos = BLOCKS_MAP_INODO; //el mapa de nodos para conocer si está libre o no el nodo
	sbloque[0].numBloquesInodos =  48; //El número de bloques de inodos solo puede haber hasta 48 inodos
	sbloque[0].primerBloqueDatos = sbloque[0].numBloquesInodos + sbloque[0].primerInodo -1; //El primer bloque de datos
	sbloque[0].numBloquesMapaDatos = BLOCKS_MAPS_DATA;  //el mapa de nodos para conocer si está libre o no el bloque de datos
	sbloque[0].numBloquesDatos =  numBloques - sbloque[0].numBloquesInodos - 3;// quitar el superbloques y los dos bloques de mapas al total
	sbloque[0].tamDispositivo = tamanoDisco;	// EL tamano de la partición
	sbloque[0].numFicheros = 0;
	sBtoChar(contenidoSB); //para convertir de Struct a Char el tipo SuperBLoques
}



/**
 * Pasa los datos de una estructura superbloque a un array de caracteres
 * Entrada: el array de caracteres
 * Salida: nada
*/
void sBtoChar(char* contenidoSB){
	short auxiliar = 0;
	//el numero mágico
	memcpy(contenidoSB, &sbloque[0].numMagico, sizeof sbloque[0].numMagico);
	auxiliar = sizeof sbloque[0].numMagico; //Actualizo el valor de auxiliar
	//Primer bloque donde se encuentra los bloques de Inodos
	memcpy(contenidoSB + auxiliar, &sbloque[0].primerInodo, sizeof sbloque[0].primerInodo);
	auxiliar = sizeof sbloque[0].primerInodo + auxiliar; //Actualizo el valor de auxiliar
	//El número de bloques del que cuenta el mapa de Inodos
	memcpy(contenidoSB + auxiliar, &sbloque[0].numBloquesMapaInodos, sizeof sbloque[0].numBloquesMapaInodos);
	auxiliar = sizeof sbloque[0].numBloquesMapaInodos + auxiliar; //Actualizo el valor de auxiliar
	//El número total de bloques de inodos
	memcpy(contenidoSB + auxiliar, &sbloque[0].numBloquesInodos, sizeof sbloque[0].numBloquesInodos);
	auxiliar = sizeof sbloque[0].numBloquesInodos + auxiliar; //Actualizo el valor de auxiliar
	//El primer bloque de datos
	memcpy(contenidoSB + auxiliar, &sbloque[0].primerBloqueDatos, sizeof sbloque[0].primerBloqueDatos);
	auxiliar = sizeof sbloque[0].primerBloqueDatos + auxiliar; //Actualizo el valor de auxiliar
	//El nnumero de bloques del mapa de datos
	memcpy(contenidoSB + auxiliar, &sbloque[0].numBloquesMapaDatos, sizeof sbloque[0].numBloquesMapaDatos);
	auxiliar = sizeof sbloque[0].numBloquesMapaDatos + auxiliar; //Actualizo el valor de auxiliar
	//El total de bloques de datos
	memcpy(contenidoSB + auxiliar, &sbloque[0].numBloquesDatos, sizeof sbloque[0].numBloquesDatos);
	auxiliar = sizeof sbloque[0].numBloquesDatos + auxiliar; //Actualizo el valor de auxiliar
	//El numero de fichero actualmente
	memcpy(contenidoSB + auxiliar, &sbloque[0].numFicheros, sizeof sbloque[0].numFicheros);
	auxiliar = sizeof sbloque[0].numFicheros + auxiliar; //Actualizo el valor de auxiliar
	//El tamaño total de la particion
	memcpy(contenidoSB + auxiliar, &sbloque[0].tamDispositivo, sizeof sbloque[0].tamDispositivo);
}



/**
 * Pasa de un array de caracteres a una estructura de superbloques
 * Entrada: el array de caracteres
 * Salida: nada
*/
void chartoSB(char* contenidoSB){
	short auxiliar = 0;
	memcpy(&sbloque[0].numMagico, contenidoSB, sizeof sbloque[0].numMagico);
	auxiliar = sizeof sbloque[0].numMagico; //Actualizo el valor de auxiliar
	//Primer bloque donde se encuentra los bloques de Inodos
	memcpy(&sbloque[0].primerInodo, contenidoSB + auxiliar, sizeof sbloque[0].primerInodo);
	auxiliar = sizeof sbloque[0].primerInodo + auxiliar; //Actualizo el valor de auxiliar
	//El número de bloque[0]s del que cuenta el mapa de Inodos
	memcpy(&sbloque[0].numBloquesMapaInodos, contenidoSB + auxiliar, sizeof sbloque[0].numBloquesMapaInodos);
	auxiliar = sizeof sbloque[0].numBloquesMapaInodos + auxiliar; //Actualizo el valor de auxiliar
	//El número total de bloques de inodos
	memcpy(&sbloque[0].numBloquesInodos,contenidoSB + auxiliar, sizeof sbloque[0].numBloquesInodos);
	auxiliar = sizeof sbloque[0].numBloquesInodos + auxiliar; //Actualizo el valor de auxiliar
	//El primer bloque de datos
	memcpy(&sbloque[0].primerBloqueDatos,contenidoSB + auxiliar, sizeof sbloque[0].primerBloqueDatos);
	auxiliar = sizeof sbloque[0].primerBloqueDatos + auxiliar; //Actualizo el valor de auxiliar
	//El numero de bloques del mapa de datos
	memcpy(&sbloque[0].numBloquesMapaDatos,contenidoSB + auxiliar, sizeof sbloque[0].numBloquesMapaDatos);
	auxiliar = sizeof sbloque[0].numBloquesMapaDatos + auxiliar; //Actualizo el valor de auxiliar
	//El total de bloques de datos
	memcpy(&sbloque[0].numBloquesDatos, contenidoSB + auxiliar, sizeof sbloque[0].numBloquesDatos);
	auxiliar = sizeof sbloque[0].numBloquesDatos + auxiliar; //Actualizo el valor de auxiliar
	//El número actual de ficheros
	memcpy(&sbloque[0].numFicheros, contenidoSB + auxiliar, sizeof sbloque[0].numFicheros);
	auxiliar = sizeof sbloque[0].numFicheros + auxiliar; //Actualizo el valor de auxiliar
	//El tamaño total de la particion
	memcpy(&sbloque[0].tamDispositivo, contenidoSB + auxiliar, sizeof sbloque[0].tamDispositivo);
}



/**
 * Realiza el formateo de un inodo
 * Entrada: el indice del inodo que se desea formatear
 * Salida: nada
*/
void inodoVacio(short indice){
	memset(inodos[indice].nomFichero, '\0', sizeof inodos[indice].nomFichero);
	inodos[indice].referencia[0] =  sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1;
	inodos[indice].tamano = 0;
	inodos[indice].integridad= 0;
}



/**
 * Para pasar de Inodos a Char
 * Entrada: array de caracteres y el indice
 * Salida: nada
*/
void inodotoChar(char* contenidoInodo, int indice){
	short auxiliar = 0;
	//Primer bloque donde se encuentra los bloques de Inodos
	memcpy( contenidoInodo, inodos[indice].nomFichero, sizeof inodos[indice].nomFichero);
	auxiliar = sizeof inodos[indice].nomFichero; //Actualizo el valor de auxiliar
	memcpy( contenidoInodo + auxiliar, inodos[indice].referencia, sizeof inodos[indice].referencia);
	auxiliar= sizeof inodos[indice].referencia + auxiliar; //Actualizo el valor de auxiliar
	memcpy( contenidoInodo + auxiliar, &inodos[indice].tamano, sizeof inodos[indice].tamano);
	auxiliar= sizeof inodos[indice].tamano+ auxiliar; //Actualizo el valor de auxiliar
	memcpy( contenidoInodo + auxiliar, &inodos[indice].integridad, sizeof inodos[indice].integridad);
}



/**
 * Pasa de un array de caracteres a una estructura de inodos
 * Entrada: el array de contenidos y el indice
 * Salida: nada
*/
void chartoInodo(char* contenidoInodo, int indice){
	int auxiliar = 0;
	//El nombre del fichero
	memcpy( inodos[indice].nomFichero, contenidoInodo, sizeof inodos[indice].nomFichero);
	auxiliar = sizeof inodos[indice].nomFichero; //Actualizo el valor de auxiliar
	//La dirección del bloque de datos
	memcpy( inodos[indice].referencia, contenidoInodo + auxiliar, sizeof inodos[indice].referencia);
	auxiliar= sizeof inodos[indice].referencia+ auxiliar; //Actualizo el valor de auxiliar
	//El tamaño del fichero
	memcpy( &inodos[indice].tamano, contenidoInodo+ auxiliar, sizeof inodos[indice].tamano);
	auxiliar= sizeof inodos[indice].tamano+ auxiliar; //Actualizo el valor de auxiliar
	//El valor de integridad hash
	memcpy( &inodos[indice].integridad, contenidoInodo + auxiliar, sizeof inodos[indice].integridad);
}



/**
 * Imprime en pantalla los atributos de una estructura de superbloques
 * Entrada: nada
 * Salida: nada
*/
void printfSB(){
	printf("Tipo SB: %i %hu %hu %hu %hu %hu %hu %hu %i\n", sbloque[0].numMagico,sbloque[0].primerInodo, sbloque[0].numBloquesMapaInodos, sbloque[0].numBloquesInodos, 
					 sbloque[0].primerBloqueDatos,sbloque[0].numBloquesMapaDatos, sbloque[0].numBloquesDatos, sbloque[0].numFicheros, 
					 sbloque[0].tamDispositivo );
}



/**
 * Imprime en pantalla los atributos de una estructura de inodos
 * Entrada: el bloque de se desea imprimir
 * Salida: nada
*/
void printfInodo(){
	for(int indice = 0; indice<sbloque[0].numBloquesInodos; indice++){
		printf("Inodo %i: %s %hu %hu %i\n", indice, inodos[indice].nomFichero, inodos[indice].referencia[0],
													inodos[indice].tamano, inodos[indice].integridad);
	}
}
