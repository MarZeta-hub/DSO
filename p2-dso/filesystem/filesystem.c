
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

TipoSuperbloque super; //Defino el tipo para agregar contenido al superbloque

void createSuperBloque();
/*
 * Genera la estructura del sistema de ficheros disenada.
 * Entrada:Tamano del disco a dar formato, en bytes.
 * Salida: Devuelve 0 si es correcto y -1 si es un error.
 */
int mkFS(long deviceSize)
{
 	//Superbloque (SB): devuelve el char con 2048B con los datos del superbloque
	char contenidoSB[BLOCK_SIZE]; //El contenido del superbloque
	createSuperBloque(deviceSize, contenidoSB); //Añadir todos el contenido al superbloque
	//Escribo el contenido del superbloque en el primer bloque
	if( bwrite(DEVICE_IMAGE, 0, contenidoSB) == -1) {
		return -1; //En caso de que de fallo la escritura
	}

	//Mapa de Inodos: pertenece al bloque 1 y mediante él sabemos que bloques de inodos están vacios
	char contenidoMaps[super.numBloquesInodos] ; //Creamos el bloque de Inodos
	for( int i; i<super.numBloquesInodos; i++){
		bitmap_setbit(contenidoMaps, i, 0);
	}
	//En nuestro sistema de ficheros vamos a agregar un inodo por bloque, por lo que el número de bloques
	//Y de inodos es el mismo, por lo que los mapas son iguales.
	//para el mapa de inodos
	if( bwrite(DEVICE_IMAGE, 1, contenidoMaps) == -1) {
		return -1; //En caso de que de fallo la escritura
	}
	//Para el mapa de bloques
	if( bwrite(DEVICE_IMAGE, 2, contenidoMaps) == -1) {
		return -1; //En caso de que de fallo la escritura
	}

	//crear ultimo bloque de EOF
	return 0;
}

/**
 * Monta la particion en la memoria
 * Entrada: Nada
 * Salida: 0 si es correcto y salida -1 si es incorrecto
*/
int mountFS(void)
{
	//Montamos en memoria el superbloque
	char contenidoSB[BLOCK_SIZE];
	if( bread(DEVICE_IMAGE, 0, contenidoSB ) == -1) {return -1;}
	//Creo espacio para guardar el SUPERBLOQUE
	sbloque = malloc(sizeof sbloque);
	//Pasamos los datos a sbloque, es decir, a la memoria
	tipoChartoSB(contenidoSB);

	//Montamos en memoria los MAPAS DE BITS
	char mapAux[BLOCK_SIZE];
	//Mapa de INODOS
	//Leo de disco en el bloque 1, el del 
	if( bread(DEVICE_IMAGE, 1, mapAux) == -1){return -1;}
	//Igualamos los punteros
	i_map = malloc(sizeof sbloque[0].numBloquesInodos);
	memcpy(i_map, mapAux, sizeof sbloque[0].numBloquesInodos);
	//Mapa de BLOQUES DE DATOS
	if( bread(DEVICE_IMAGE, 2, mapAux) == -1){return -1;}	
	//Igualamos los punteros
	b_map = malloc(sizeof sbloque[0].numBloquesDatos);
	memcpy(b_map, mapAux, sizeof sbloque[0].numBloquesInodos);

	//Montamos en memoria los Inodos
	char inodosContent[sbloque[0].numBloquesInodos][BLOCK_SIZE];
	inodos  = malloc(sizeof inodos * sbloque[0].primerInodo);
	for(int i=0; i < (sbloque[0].numBloquesInodos); i++) {
		if( bread(DEVICE_IMAGE, sbloque[0].primerInodo + i, inodosContent[i]) == -1) {return -1;} 
		tipoChartoInodo(inodosContent[i], i);	
	}	
	
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
		return -1;
	}
	//Desmontamos los datos del disco en memoria
	free (sbloque);
	free (i_map);
    free (b_map);
    free (inodos);
	return 0;
}

int syncDisk(){
	return 0;
}

/*
 * Crea un nuevo archivo 
 * Entrada: nombre del archivo
 * Salida: 0 Si es correcto, -1 si ya existe el nombre y -2 otros errores
 */
int createFile(char *fileName)
{
	//no más de 48 archivos
	//Buscar un inodo vacío
	//Modificas el Inodo vacia mediante inodo[Indice]
	//Buscar bloque libre de datos
	//Actulizar los mapas de bits con set 1
	return 0;
}

/*
 * Elimina un archivo del disco
 * Entrada: nombre del archivo
 * Salida: o Si es correcto, -1 si no existe el archivo y -2 otros errores
 */
int removeFile(char *fileName)
{	
	//Buscar el inodo con el nombre if si no existe return -1
	//Actualizar el mapa de bits de datos y de inodos
	//Borrar el inodo con el nombre
	
	return -2;
}

/*
 * Pasa los datos de un archivo a la memoria
 * Entrada: nombre del archivo
 * Salida: descriptor de entrada -1 no existe en el sistema y  -2 otros errores
 */
int openFile(char *fileName)
{
	//Buscar entre los inodos el archivo
	//traer de los bloques todo el contenido mediante un malloc con size de tamaño
	//devolver el lugar del char generado
	return -2;
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
TipoSuperbloque super;
/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */
int removeLn(char *linkName)
{
    return -2;
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
	super.primerInodo = 4;	//El primer bloque de inodos
	super.numBloquesMapaInodos = BLOCKS_MAP_INODO; //el mapa de nodos para conocer si está libre o no el nodo
	super.numBloquesInodos =  (numBloques - 3)/2; //El número de bloques de inodos
	super.primerBloqueDatos = super.numBloquesInodos + 4; //El primer bloque de datos
	super.numBloquesMapaDatos = BLOCKS_MAPS_DATA;  //el mapa de nodos para conocer si está libre o no el bloque de datos
	super.numBloquesDatos =  super.numBloquesInodos;// quitar el superbloques y los dos bloques de mapas al total
	super.tamDispositivo = tamanoDisco;	// EL tamano de la partición
	super.numeroFicheros = 0;
	tipoSBtoChar(contenidoSB); //para convertir de Struct a Char el tipo SuperBLoques
	
}

/**
 * Pasa los datos de una estructura superbloque a un array de caracteres
 * Entrada: el array de caracteres
 * Salida: nada
*/
void tipoSBtoChar(char* contenidoSB){
	short auxiliar = 0;
	//Primer bloque donde se encuentra los bloques de Inodos
	memcpy(contenidoSB, &super.primerInodo, sizeof super.primerInodo);
	auxiliar = sizeof super.primerInodo; //Actualizo el valor de auxiliar
	//El número de bloques del que cuenta el mapa de Inodos
	memcpy(contenidoSB + auxiliar, &super.numBloquesMapaInodos, sizeof super.numBloquesMapaInodos);
	auxiliar = sizeof super.numBloquesMapaInodos + auxiliar; //Actualizo el valor de auxiliar
	//El número total de bloques de inodos
	memcpy(contenidoSB + auxiliar, &super.numBloquesInodos, sizeof super.numBloquesInodos);
	auxiliar = sizeof super.numBloquesInodos + auxiliar; //Actualizo el valor de auxiliar
	//El primer bloque de datos
	memcpy(contenidoSB + auxiliar, &super.primerBloqueDatos, sizeof super.primerBloqueDatos);
	auxiliar = sizeof super.primerBloqueDatos + auxiliar; //Actualizo el valor de auxiliar
	//El nnumero de bloques del mapa de datos
	memcpy(contenidoSB + auxiliar, &super.numBloquesMapaDatos, sizeof super.numBloquesMapaDatos);
	auxiliar = sizeof super.numBloquesMapaDatos + auxiliar; //Actualizo el valor de auxiliar
	//El total de bloques de datos
	memcpy(contenidoSB + auxiliar, &super.numBloquesDatos, sizeof super.numBloquesDatos);
	auxiliar = sizeof super.numBloquesDatos + auxiliar; //Actualizo el valor de auxiliar
	//El numero de fichero actualmente
	memcpy(contenidoSB + auxiliar, &super.numeroFicheros, sizeof super.numeroFicheros);
	//El tamaño total de la particion
	memcpy(contenidoSB + auxiliar, &super.tamDispositivo, sizeof super.tamDispositivo);
}

/**
 * Pasa de un array de caracteres a una estructura de superbloques
 * Entrada: el array de caracteres
 * Salida: nada
*/
void tipoChartoSB(char* contenidoSB){
	short auxiliar = 0;
	//Primer bloque donde se encuentra los bloques de Inodos
	memcpy(&sbloque[0].primerInodo, contenidoSB, sizeof sbloque[0].primerInodo);
	auxiliar = sizeof sbloque[0].primerInodo; //Actualizo el valor de auxiliar
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
	memcpy(&sbloque[0].numeroFicheros, contenidoSB + auxiliar, sizeof sbloque[0].numeroFicheros);
	//El tamaño total de la particion
	memcpy(&sbloque[0].tamDispositivo, contenidoSB + auxiliar, sizeof sbloque[0].tamDispositivo);
}

/**
 * Pasa de un array de caracteres a una estructura de inodos
 * Entrada: el array de contenidos y el indice
 * Salida: nada
*/
void tipoChartoInodo(char* contenidoInodo, int indice){
	int auxiliar = 0;
	//El nombre del fichero
	memcpy( inodos[indice].nomFichero, contenidoInodo, sizeof inodos[indice].nomFichero);
	auxiliar = sizeof inodos[indice].nomFichero; //Actualizo el valor de auxiliar
	//La dirección del bloque de datos
	memcpy( &inodos[indice].referencia, contenidoInodo + auxiliar, sizeof inodos[indice].referencia);
	auxiliar= sizeof inodos[indice].referencia+ auxiliar; //Actualizo el valor de auxiliar
	//La dirección del siguiente bloque de datos
	memcpy( &inodos[indice].referenciaSig, contenidoInodo+ auxiliar, sizeof inodos[indice].referenciaSig);
	auxiliar= sizeof inodos[indice].referenciaSig+ auxiliar; //Actualizo el valor de auxiliar
	//El tamaño del fichero
	memcpy( &inodos[indice].tamano, contenidoInodo+ auxiliar, sizeof inodos[indice].tamano);
	auxiliar= sizeof inodos[indice].tamano+ auxiliar; //Actualizo el valor de auxiliar
	//El puntero de lectura y escritura del fichero
	memcpy( &inodos[indice].punteroRW, contenidoInodo + auxiliar, sizeof inodos[indice].punteroRW);
	auxiliar= sizeof inodos[indice].punteroRW+ auxiliar; //Actualizo el valor de auxiliar
	//El valor de integridad hash
	memcpy( &inodos[indice].integridad, contenidoInodo + auxiliar, sizeof inodos[indice].integridad);

}

/**
 * Imprime en pantalla los atributos de una estructura de superbloques
 * Entrada: nada
 * Salida: nada
*/
void printfSB(){
	printf("Tipo SB: %hu %hu %hu %hu %hu %hu %i\n", sbloque[0].primerInodo, sbloque[0].numBloquesMapaInodos, sbloque[0].numBloquesInodos, 
					 sbloque[0].primerBloqueDatos,sbloque[0].numBloquesMapaDatos, sbloque[0].numBloquesDatos, sbloque[0].numeroFicheros, 
					 sbloque[0].tamDispositivo );
}

/**
 * Imprime en pantalla los atributos de una estructura de inodos
 * Entrada: el bloque de se desea imprimir
 * Salida: nada
*/
void printfInodo(int indice){
	printf("Inodo %i: %s %hu %hu %hu %hu %hu\n", indice, inodos[indice].nomFichero, inodos[indice].referencia, inodos[indice].referenciaSig,
													inodos[indice].tamano, inodos[indice].punteroRW, inodos[indice].integridad);
}