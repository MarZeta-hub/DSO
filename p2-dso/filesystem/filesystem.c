
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
		contenidoMaps[i] = '1';
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
	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	return -1;
}

/*.numBloquesInodoe file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	return -2;

}

int closeFile(int fileDescriptor) {
	return -1;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
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
	sprintf(contenidoSB,"%hu, %hu, %hu, %hu, %hu, %hu, %i", super.primerInodo, super.numBloquesMapaInodos, super.numBloquesInodos, 
						 super.primerBloqueDatos,super.numBloquesMapaDatos, super.numBloquesDatos, super.tamDispositivo );
}