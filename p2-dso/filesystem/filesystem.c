
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
#include "math.h"

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
	//MAPA DE INODOS
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
	//Empiezo a formatear todos los inodos
	for (int i = 0; i < sbloque[0].numBloquesInodos; i++){
		if( bwrite(DEVICE_IMAGE, sbloque[0].primerInodo + i, contenido) == -1) {
			perror("Fallo al formatear los INODOS mkFS\n");
			return -1; //En caso de que de fallo la escritura
		}
	}

	//Libero memoria principal
	free(i_map); //Vacío el mapa de inodos
	i_map = NULL;
	free(b_map); //Vacio el mapa de datos
	b_map = NULL;
	free(inodos); //Vacio los inodos
	inodos = NULL;
	free(sbloque); //Libero la estructura de superbloque
	sbloque = NULL;
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
	int fd = -1;
	for(int i = 0; i < (sizeof(fileDescriptor) / sizeof(fileDescriptor[0])); i++ ){
		if(strcmp(fileDescriptor[i].nombre, "") == 0){
			fd = i;
			break;
		}
	}

	if(fd == -1){
		perror("No se puede desmontar el sistema, hay archivos abiertos");
		return -1;
	}

	//Desmontamos los datos del disco en memoria
  	free(inodos);
	inodos = NULL;
	free (sbloque);
	sbloque = NULL;
	free (i_map);
	i_map = NULL;
    free (b_map);
	b_map = NULL;
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
	if (checkTamanoNombre( fileName) == -1){
		perror("El tamaño del nombre es incorrecto");
		return -2;
	} 
	//Compruebo si el nombre del archivo existe en el sistema

	//devuelve el nodo si existe el fichero, si no devuelve -1
	if (existeFichero(fileName) != -1){
		perror("El nombre de archivo ya existe");
		return -1;
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
	bitmap_setbit(i_map, inodo_libre, 1); //Ocupo en el mapa de bits el nuevo bloque
	inodos[inodo_libre].referencia[0] = sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1 ;
	sbloque[0].numFicheros = sbloque[0].numFicheros + 1; //Añado al superbloque un nuevo archivo

	return 0;
}



/*
 * Elimina un archivo del disco
 * Entrada: nombre del archivo
 * Salida: 0 si es correcto, -1 si no existe el archivo y -2 otros errores
 */
int removeFile(char *fileName)
{	
	//Comprobar si el tamaño del nombre es correcto
	if(checkTamanoNombre(fileName) != 0){
		perror("El tamaño del nombre no es correcto");
		return -2;
	} 
    //Busco donde está el fichero
	short indiceFichero = existeFichero(fileName);
	//Buscar el inodo con el nombre if si no existe return -1
	if (indiceFichero == -1) {
		perror("El fichero no existe");
		return -1;
	}

	//Elimino el fd si tiene uno
	int fd = searchFD(fileName);
	if(fd !=-1){
		if(fileDescriptor[fd].abiertoConIntegridad == 0) closeFile(fd); //cerrarlo sin integridad
		else closeFileIntegrity(fd); //Cerrado con integridad
	}
	
	int bloqueEOF = sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1 ;

	for(int i = 0; i< 5; i++){
		int bloqueDatos = inodos[indiceFichero].referencia[i];
		if(bloqueDatos == bloqueEOF) break;
		bitmap_setbit(b_map, bloqueDatos - sbloque[0].primerBloqueDatos , 0);
	}

	//Formateo el nodo
	inodoVacio(indiceFichero);
	//Actualizar el mapa de bits de datos y de inodos
	bitmap_setbit(i_map, indiceFichero,0);
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
	
	//Compruebo la existencia del fichero, y guardo su identificador de nodo
	short inodoFichero = existeFichero(fileName);
	if (inodoFichero == -1) {
		perror("El fichero no existe");
		return -1;
	}
	
	//Compruebo si ya existe en el sistema
	int fd = searchFD(fileName);
	//Si ya existe, devuelvo el descritor donde está abierto
	if(fd != -1){
		perror("El fichero ya está abierto");
		return -1;
	}
	
	//Busco un fd vacio
	for(int i = 0; i < (sizeof(fileDescriptor) / sizeof(fileDescriptor[0])); i++ ){
		if(strcmp(fileDescriptor[i].nombre, "") == 0){
			fd = i;
			break;
		}
	}
	if(fd == -1){
		perror("Máximo de archivos abiertos alcanzado");
		return -1;
	} 
	//Hago todo lo necesario para abrir el fichero en memoria
	memcpy(fileDescriptor[fd].nombre, fileName, strlen (fileName));
	fileDescriptor[fd].punteroRW = 0;	
	fileDescriptor[fd].punteroInodo = &inodos[inodoFichero];

	return fd;
}



/**
 * Cierra un fichero de datos en memoria
 * Entrada: descriptor de archivos
 * Salida: o si es correcto y -1 si es erronea
*/
int closeFile(int file_Descriptor) {

	//Compruebo si el valor que me han pasado es correcto
	if(file_Descriptor >48 || file_Descriptor<0){
		perror("Descriptor de fichero invalido");
		return -1;
	}

	//Compruebo que el fichero está abierto
	if(strcmp(fileDescriptor[file_Descriptor].nombre, "") == 0){
		perror("No está abierto ese fichero");
		return -1;
	}
	
	if(fileDescriptor[file_Descriptor].abiertoConIntegridad == 1){
		perror("El archivo ha sido abierto con comprobación de integridad");
		return -1;
	}

	//Formateo la estructura del descriptor de ficheros elegida.
	memcpy(fileDescriptor[file_Descriptor].nombre, "", sizeof "");
	fileDescriptor[file_Descriptor].punteroRW = 0;
	fileDescriptor[file_Descriptor].punteroInodo = NULL;
	return 0;
}


/**
 * Leo la cantidad de bytes que quiere el usuario
 * Entrada: descriptor de fichero, un buffer, y el tamaño de bytes que se quiere leer
 * Salida: -1 si es un error, o los bytes leidos
*/
int readFile(int file_Descriptor, void *buffer, int numBytes)
{
	
	int punteroR = fileDescriptor[file_Descriptor].punteroRW;
	if(numBytes + punteroR > LIMITE_TAMANO){
		perror("Error, el tamaño más la posición actual del puntero es mayor que el tamaño máximo de archivo");
		return -1;
	}

	//Creo un nuevo lector para grabar lo que obtengo de disco
	char* lecturaBloques = malloc ( LIMITE_TAMANO );
	//Variables:
	int numBytesLeidos = 0; //Los bytes que obtengo al leer de disco
	int indiceBloque = 0; //El indice del bloque que leo del archivo desde el inodo
	int bloqueLeido = -1; //El bloque que leo de disco
	//Último bloque del disco, significa que es el fin del fichero
	int bloqueEOF = sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1 ;
	//Mientras no supere los bytes leidos de disco de los bytes del usuario
	while(numBytesLeidos < numBytes){
		// Consigo el bloque de disco que quiero leer
		bloqueLeido = fileDescriptor[file_Descriptor].punteroInodo[0].referencia[indiceBloque];
		//Compruebo que no es el fin del fichero
		if( bloqueLeido == bloqueEOF )break;
		//Leo de disco para conseguir el bloque y lo añado a nuestro char
		if( bread(DEVICE_IMAGE, bloqueLeido, lecturaBloques + numBytesLeidos) == -1){
			perror("Error al leer de disco");
			return -1;
		}
		//Actualizo el número de bytes que he ledio de disco
		numBytesLeidos = strlen(lecturaBloques);
		//Actualizo el indice de bloques para obtener el siguiente bloque
		indiceBloque = indiceBloque + 1;
	}

	//En el caso de que el numero de bytes leidos sea menor que el que me piden
	numBytesLeidos = numBytesLeidos - punteroR;
	lecturaBloques = lecturaBloques + punteroR;	
	memcpy(buffer, lecturaBloques, strlen(lecturaBloques));
	lecturaBloques = NULL;
	free(lecturaBloques);
	//Actualizo el puntero
	fileDescriptor[file_Descriptor].punteroRW = punteroR;
	//Devuelvo los bloques leidos
	return numBytesLeidos;
}



/**
 * Escribe el contenido de un array de chars en disco
 * Entrada: descriptor de archivos, el buffer y el numBytes
 * Salida: el número de bytes escritos si es correcto, -1 si es incorrecto 
*/
int writeFile(int file_Descriptor, void *buffer, int numBytes)
{	
	int punteroW = fileDescriptor[file_Descriptor].punteroRW; // Obtengo el puntero donde está el archivo
	short bloqueEOF = sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1; // Obtengo la posición del bloque EOF
	
	if(numBytes + punteroW > LIMITE_TAMANO){
		numBytes = LIMITE_TAMANO; //En el caso de que el numBytes que se quiera leer más la posición del archivo sea mas que el limite máximo
	}

	//LECTURA DEL CONTENIDO DEL DISCO QUE HAY QUE MODIFICAR
	int obtenerReferencia = punteroW / BLOCK_SIZE ;  //Obtengo la referencia desde donde se quiere escribir
	int bloquesNecesarios = (numBytes / BLOCK_SIZE) + 1; // Obtengo los bloques necesarios que se quiere escribir
	char* contenidoDisco = malloc (bloquesNecesarios * BLOCK_SIZE); // Creo un nuevo array de chars para almacenar los datos
	if( readFile(file_Descriptor, contenidoDisco, bloquesNecesarios * BLOCK_SIZE) == -1) return -1; //leo el contenido si se quiere sobreescribir el archivo
	int punteroCD = punteroW - obtenerReferencia * BLOCK_SIZE; //Obtengo un nuevo puntero desde la refencia anterior
	memcpy(contenidoDisco + punteroCD, buffer, numBytes); //Copio lo nuevo que quiero escribir sobreescribiendo lo anterior mediante el puntero
	
	//ESCRITURA DEL CONTENIDO ACTUALIZADO AL DISCO
	short nuevoBloque = 0; //Es un indicador que me ofrece si sigue habiendo bloques del fichero o hay que buscar nuevos bloques de datos vacios
	short lugarBloque = -1; //Me proporciona el bloque que hay que modificar
	int punteroEscritura = 0; //Me otorga la posición actual del array de chars de contenidoDisco

	//Hasta que no se completen los bloques que quiero escribir
	for(int bloqueActual = 0; bloqueActual < bloquesNecesarios; bloqueActual++){
		obtenerReferencia = obtenerReferencia + bloqueActual; //El indice de refencias del archivo
		if(nuevoBloque == 0) lugarBloque = fileDescriptor[file_Descriptor].punteroInodo[0].referencia[obtenerReferencia]; // Si existe esa referencia en el inodo
		if(lugarBloque == bloqueEOF) nuevoBloque = 1; //En caso de que no, se activa el flag de que debemos buscar a partir de ahora bloques de datos nuevos
		if(nuevoBloque == 1){ //Para buscar un bloque de datos nuevo
			for(int i = 0; i < sbloque[0].numBloquesDatos; i++){ // Busco entre todos los bloques de datos del sistema
				if( bitmap_getbit(b_map, i) == 0){ //En el caso de que en el mapa de datos no esté usado el bloque
					lugarBloque = i + sbloque[0].primerBloqueDatos ; //Lo añado al Lugar bloque
					fileDescriptor[file_Descriptor].punteroInodo[0].referencia[obtenerReferencia] = lugarBloque; //Lo añado a las referencias del inodo
					bitmap_setbit(b_map, i, 1); //Selecciono como que ese bloque ha sido ya utilizado
					break; //Rompo el for
				}
			}
		}
		//Escribo en disco en el lugar de bloque proporcionado y el contenido que se quiere agregar
		if( bwrite(DEVICE_IMAGE, lugarBloque, contenidoDisco + punteroEscritura) != 0){
			perror("Error al escribir en el archivo");
			return -1;
		}
		punteroEscritura = punteroEscritura + BLOCK_SIZE; //Actualizo el puntero de escritura
	}
	punteroW = punteroW + numBytes; //Actualizo el puntero del descriptor de archivos
	fileDescriptor[file_Descriptor].punteroRW = punteroW; //Actualizao el puntero del descriptor de archivos
	int tamanoFichero = fileDescriptor[file_Descriptor].punteroInodo[0].tamano; //Obtengo el tamaño del fichero
	contenidoDisco = NULL;
	free(contenidoDisco); //Libero el malloc realizado anteriormente
	if(punteroW > tamanoFichero){ //Si el puntero otorgado es mayor que el tamaño dle fichero, significa que he añadido más contenido
		fileDescriptor[file_Descriptor].punteroInodo[0].tamano = punteroW; //Entonces cambio el tamaño
		fileDescriptor[file_Descriptor].punteroInodo[0].referencia[obtenerReferencia + 1] = bloqueEOF; //Y añado el bloque eof al final de los bloques de referencia
	}
	return numBytes; //Devuelvo el número de bytes escritos
}



/**
 * Establece el puntero de lectura a una posición dicha por el usuario
 * Entrada: el descriptor de fichero, la posición de desplazamiento y desde donde
 * Whence = 0: EL puntero se queda donde está. Solo funciona el offset en este caso
 * Whence = 1: EL documento se establece desde el inicio
 * Whence = 2: El docuemento se establece desde el final
 * 
*/
int lseekFile(int file_Descriptor, long offset, int whence)
{
	if(strcmp(fileDescriptor[file_Descriptor].nombre,"")==0 || file_Descriptor<0 || file_Descriptor>48){//COmprobamos el fd pasado
		perror("El descriptor de fichero no existe");
		return -1;
	}
	short nuevaPosicion;
	switch (whence)
	{
	case 0: //FS_SEEK_CUR  DESDE LA POSICION ACTUAL
		 nuevaPosicion = fileDescriptor[file_Descriptor].punteroRW + offset;
		if(nuevaPosicion<0 || nuevaPosicion > fileDescriptor[file_Descriptor].punteroInodo[0].tamano){//Comprobamos que la nueva posicion del puntero no exceda los limites del fichero
			perror("El offset introducido excede los limites del fichero");
			return -1;
		}
		fileDescriptor[file_Descriptor].punteroRW = nuevaPosicion;
		break;
	case 1: //FS_SEEK _BEGIN DESDE EL INICIO
		fileDescriptor[file_Descriptor].punteroRW=0;//actualizamos al principio del archivo el puntero
		break;
	case 2: //FS_SEEK_END DESDE EL FINAL
		fileDescriptor[file_Descriptor].punteroRW=fileDescriptor[file_Descriptor].punteroInodo[0].tamano;//actualizamos al final del archivo el puntero
		break;	
	default:
		perror("Opcion whence no valida");//Si whence no es valido
		return -1;
		break;
	}
	return 0;

}



/**
 * Verifica la integridad de un fichero mediante el valor que está
 * en el inodo y mediante una nueva obtención de integridad
 * Entrada: el nombre del archivo
 * Salida: 0 si es correcto, -1 si está corrupto y -2 otros errores
*/
int checkFile (char * fileName)
{
	//Compruebo el tamaño del nombre otorgado
	if(checkTamanoNombre(fileName) != 0){//comprobamos el nombre
		perror("El tamaño del nombre no es valido");
		return -2; //Otros errores
	}
	//Compruebo si el fichero existe y almaceno el indice del inodo
	int inodo = existeFichero(fileName);
	if(inodo == -1){//Comprobamos si existe el fichero
		perror("El fichero no existe");
		return -2; //Otros errores
	}
	//Busco si existe un descriptor de ficheros ya que no puede
	//Hacerse una revisión de integridad con un fichero siendo
	//Manipulado
	int fd = searchFD(fileName);
	if(fd!= -1){//Si el archivo ya estaba abierto error
		perror("El archivo estaba abierto");
		return -2; //Otros errores
	}
	//Obtengo el CRC actual que tiene el fichero, además
	//De que compruebo si lo tiene
	uint32_t crcActual = tieneIntegridad(inodo);
	if(crcActual == -1){//Comprobamos si tiene integridad
		perror("El fichero no tiene integridad");
		return -2; //Otros errores
	}
	//Obtengo las variables necesarias para hacer la integridad 
	//para comprobar si las dos integridades, tanto la que realmente
	//tiene el fichero y la que nos dice que es son correctas
	int tamano = inodos[inodo].tamano;
	unsigned short* referencia = inodos[inodo].referencia;
	uint32_t CRCnuevo = crearIntegridad(tamano, referencia);

	//Compruebo si las dos integridades son inguales
	if(CRCnuevo != crcActual){ //comparamos los CRC
		perror("El archivo esta corrupto");
		return -1; //Otros errores	
	}
    return 0;
}




/**
 * Le almacena una integridad a un fichero sin ella
 * Entrada: el nombre del fichero
 * Salida: o si es correcto, -1 si el fichero no existe y -2 por otros motivos
*/
int includeIntegrity (char * fileName)
{
	//Verifico el tamaño del fichero otorgado
    if(checkTamanoNombre(fileName) != 0){//comprobamos el nombre
		perror("El tamaño del nombre no es valido");
		return -2; //Otros errores
	}
	//Verifico la existencia del fichero en el sistema
	int inodo = existeFichero(fileName);
	if(inodo == -1){//Comprobamos si existe el fichero
		perror("El fichero no existe");
		return -1;	//El fichero no existe
	}

	/* Creemos que un fichero no puede modificar su integridad
	   cuando está abierto porque no es óptimo porqué si está 
	   abierto se puede modificar*/
	int fd = searchFD(fileName);
	if(fd!= -1){//Si el archivo ya estaba abierto error
		perror("El archivo estaba abierto");
		return -2; //Otros errores
	}

	//Busco si el fichero ya tenía integridad anteriormente
	uint32_t crcActual = tieneIntegridad(inodo);
	if(crcActual != -1){//Comprobamos si tiene integridad
		perror("El fichero tiene integridad");
		return -2; //Otros errores
	}
	
	//Los parámetros para poder crear una nueva integridad
	int tamano = inodos[inodo].tamano;
	unsigned short* referencia = inodos[inodo].referencia;
	//Obtengo la nueva integridad del fichero
	uint32_t CRCnuevo = crearIntegridad(tamano, referencia);
	//Se la añado
	inodos[inodo].integridad = CRCnuevo; //Establecemos el CRC creado
	return 0;
}



/**
 * Abre un archivo con verificación de integridad, si el archivo
 * no posee integridad o está corrupto no se abre
 * Entrada: el nombre del archivo
 * Salida: 0 si es correcto, -1 si no existe, -2 si está corrupto
 * -3 otros errores
*/
int openFileIntegrity(char *fileName)///MIRAR ERRORES
{	
	//Compruebo el tamaño del nombre de ficheor dado
	if(checkTamanoNombre(fileName) != 0){//comprobamos el nombre
		perror("El tamaño del nombre no es valido");
		return -3; //Otros problemas
	}
	//Compruebo si existe el nodo
	int inodo = existeFichero(fileName);
	if(inodo == -1){//Comprobamos si existe el fichero
		perror("El fichero no existe");
		return -1;	//No existe en el sistema	
	}

	int verificacionFichero = checkFile(fileName);
	if( verificacionFichero == -1){
		perror("Fichero corrupto");
		return -2; //El fichero está corrupto
	}
	if(verificacionFichero == -2){
		perror("No contiene integridad ");
		return -3;
	}

	//Compruebo si ya existe en el sistema de filedescriptor
	int fd = searchFD(fileName);
	//Si ya existe, devuelvo el descritor donde está abierto
	if(fd != -1) {
		perror("El fichero ya está abierto");
		return -3;
	}
	
	//Busco un fd vacio
	for(int i = 0; i < (sizeof(fileDescriptor) / sizeof(fileDescriptor[0])); i++ ){
		if(strcmp(fileDescriptor[i].nombre, "") == 0){
			fd = i;
			break;
		}
	}
	//En el caso de que no existan descriptores deficheros libres
	if(fd == -1){
		perror("Máximo de archivos abiertos alcanzado");
		return -3;
	} 


	//Hago todo lo necesario para abrir el fichero en memoria
	memcpy(fileDescriptor[fd].nombre, fileName, strlen (fileName));
	//Se activa el open con integridad
	fileDescriptor[fd].abiertoConIntegridad = 1;
	//se modifica el puntero de posición
	fileDescriptor[fd].punteroRW = 0;	
	fileDescriptor[fd].punteroInodo = &inodos[inodo];

	return fd;

}



/**
 * Cierra un descriptor de fichero con actualización de integridad
 * Entrada: el descriptor de fichero
 * Salida: 0 si es correcto, -1 en caso de error
*/
int closeFileIntegrity(int file_Descriptor)
{
	//Compruebo si el valor que me han pasado es correcto
	if(file_Descriptor >48 || file_Descriptor<0){
		perror("Descriptor de fichero invalido");
		return -1;
	}

	//Compruebo que el fichero está abierto
	if(strcmp(fileDescriptor[file_Descriptor].nombre, "") == 0){
		perror("No está abierto ese fichero");
		return -1;
	}
	
	//Compruebo si el fichero fue abierto con la opción de abrir 
	//fichero con integridad
	if(fileDescriptor[file_Descriptor].abiertoConIntegridad != 1){
		perror("El archivo NO ha sido abierto con comprobación de integridad");
		return -1;
	}

	//Obtengo el tamaño y las referencias de los bloques de datos 
	//Del descriptor de ficheros
	int tamano = fileDescriptor[file_Descriptor].punteroInodo[0].tamano;
	unsigned short* referencia = fileDescriptor[file_Descriptor].punteroInodo[0].referencia;

	//Obtengo el nuevo valor de integridad del fichero
	fileDescriptor[file_Descriptor].punteroInodo[0].integridad = crearIntegridad(tamano, referencia);
	//Formateo la estructura del descriptor de ficheros elegida.
	memcpy(fileDescriptor[file_Descriptor].nombre, "", sizeof "");
	//apunto su puntero a cero
	fileDescriptor[file_Descriptor].punteroRW = 0;
	//Apuntos u puntero del inodo a null
	fileDescriptor[file_Descriptor].punteroInodo = NULL;
	fileDescriptor[file_Descriptor].abiertoConIntegridad = 0;
	return 0;
}


/**
 * El método crea un enlace blando hacia un archivo 
 * específico. El enlace blando es un fichero que solo
 * contiene inodo, que referencia mediante puntero hacia los 
 * datos del archivo dado. De por si mismo, solo tiene el nombre
 * Entrada: el nombre del archivo del enlace blanco y el 
 * nombre del archivo al que se quiere enlazar
 * Salida: 0 si es correcto, -1 si el archivo al que se quiere
 * enlazar no existe y -2 para otros errores
*/
int createLn(char *fileName, char *linkName)
{
	//Compruebo si el nombre del enlace es correcto
	if( checkTamanoNombre(linkName) != 0 ){
		perror("El nombre del link es erroneo"); 
		return -2;
	}

	//Compruebo que el nombre del fichero es correcto
	if(checkTamanoNombre(fileName) != 0){
		perror("El nombre del link es erroneo"); 
		return -2;
	}

	//Busco si existe el fichero en el sistema
	int inodoFichero = existeFichero(fileName);
	if(inodoFichero == -1){
		perror("No existe el archivo");
		return -1;
	}
	
	//Busco un inodo que este libre para que sea el
	//del enlace blando
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

	//CREAR EL INODO nuevo para el enlace:
	bitmap_setbit(i_map, inodo_libre, 1); //Ocupo en el mapa de bits el nuevo bloque
	//El nodo libre va a ser un puntero del enlace duro
	inodos[inodo_libre] = inodos[inodoFichero];
	//Elimino el nombre del fichero
	memset(inodos[inodo_libre].nomFichero, '\0', sizeof inodos[inodo_libre].nomFichero);
	//Le añado el nuevo nombre
	memcpy(inodos[inodo_libre].nomFichero, linkName, strlen(linkName)); //le añado el nuevo nombre
	//Actualizo en el superbloque el nuevo fichero
	sbloque[0].numFicheros = sbloque[0].numFicheros + 1;
    return 0;
}



/**
 * Elimina un enlace blando sin eliminar el archivo original
 * En el caso de que se elimine un enlace blando por le método
 * de removeFile, se eliminará todas las refencias hacia los 
 * bloques de datos, por lo que se quedará el inodo del archivo 
 * inservible, ya que el enlace blando es contiene punteros hacia
 * los inodos de los archivos.
 * Entrada: el nombre del enlace blando
 * Salida: 0 si funciona correctamente, -1 si no se encuentra el archivo 
 * en el sistema y -2 otros errores
 */
int removeLn(char *linkName)
{
	//Comprobar si el tamaño del nombre es correcto
	if(checkTamanoNombre(linkName) != 0){
		perror("El tamaño del fichero no es valido");
		return -2;

	}
    //Busco donde está el fichero
	short indiceFichero = existeFichero(linkName);
	//Buscar el inodo con el nombre if si no existe return -1
	if (indiceFichero == -1) {
		perror("El fichero no existe");
		return -1;
	}

	//Elimino el fd si tiene uno para no dejar el archivo abierto
	int fd = searchFD(linkName);
	if(fd != -1){
		closeFile(fd);
	}

	//Formateo el nodo
	//Formateo el nombre con carácteres de final de array
	memset(inodos[indiceFichero].nomFichero, '\0', sizeof inodos[indiceFichero].nomFichero);
	//Recalculo el número de ficheros en el sistema, porque
	//un enlace blando se cuenta como un archivo
	sbloque[0].numFicheros = sbloque[0].numFicheros - 1;
	//Establezco en el mapa que el inodo está libre
	bitmap_setbit(i_map, indiceFichero,0);
    return 0;
}


////////////////////////NUESTROS METODOS NUEVOS/////////////////////////////////



/**
 * Comprueba el tamaño del nombre del fichero
 * Entrada: el nombre del fichero
 * Salida: 0 si es correcto, -1 si es incorrecta
*/
int checkTamanoNombre(char* fileName){
	//Compruebo si el tamaño del nombre otorgado es correcto
	int tamano = strlen(fileName) ;
	if(tamano > 32 || tamano == 0){ //Si cumple alguna de estas condiciones
		return -1; //El nombre del fichero es incorrecto
	}
	return 0; //Devuelvo 0 si es correcto
}



/**
 * Comprueba si existe el fichero
 * Entrada: Nombre del fichero
 * Salida: -1 si no se encuentra y el nodo si ya existe
*/
int existeFichero(char* fileName){
	//Busco entre todos los inodos la existencia del fichero
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
	//Busco en la estructura de descriptores de ficheros si el nombre dado está abierto
	for(int i = 0; i < (sizeof(fileDescriptor) / sizeof(fileDescriptor[0])); i++ ){
		if(strcmp(fileDescriptor[i].nombre, fileName)== 0){ //Si son iguales
			return i; //Devuelvo el FD 
		}
	}
	return -1; // EN el caso de que no exista
}

/**
 * Comprueba si el fichero tiene integridad
 * Entrada: el CRC del fichero
 * Salida: 0 si tiene integridad o -1 si no 
*/
uint32_t tieneIntegridad(short indice){
	//Busco la integridad en el inodo
	uint32_t integridad = inodos [indice].integridad;
		if( integridad == 0){
			return -1; //En el caso de que no existe la integridad
		}
	return integridad; //Devuelvo la integridad si existe
}


/**
 * El método crea una integridad para el inodo elegido
 * Entrada: el índice del inodo en el mapa de inodos
 * Salida: el CRC obtenido por el archivo
*/
uint32_t crearIntegridad(int tamano, unsigned short* referencia){
	int bloquesLectura = ( (tamano) / BLOCK_SIZE) + 1; // Obtengo los bloques totales del archivo
	char* buffer = malloc (bloquesLectura*BLOCK_SIZE); //Obtengo el buffer donde voy a leer el archivo
	int bloqueLeido ; //El bloque donde hay que leer
	int puntero = 0; //El puntero para agregar datos a disco
	for(int i = 0; i<bloquesLectura; i++){
		bloqueLeido = referencia[i]; //obtengo el bloque dondo hay que leer
		if (bread(DEVICE_IMAGE, bloqueLeido, buffer + puntero) != 0) return -1;
		puntero = puntero + BLOCK_SIZE; //Actualizo el puntero
	}
	unsigned int tamanoFichero = strlen(buffer); //Obtengo el tamaño del archivo
	unsigned char* buffer2 = malloc (tamanoFichero); //para que el CRC2 pueda realizar sus acciones
	memcpy(buffer2, buffer, tamanoFichero); //copio el contenido del buffer otorgado al buffer2
	uint32_t CRCnuevo = CRC32 ( buffer2, tamanoFichero);//hacemos el CRC de los datos leidos 

	//Libero la memoria utilizada
	buffer2 = NULL;
	buffer = NULL;
	free(buffer2);
	free(buffer);
	return CRCnuevo; //Devuelvo el nuevo CRC
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
	sbloque[0].numMagico = 100383266;  //Identificador del disco
	sbloque[0].primerInodo = 3;	//El primer bloque de inodos SuperBloque 0, Mapa 1 y mapa 2
	sbloque[0].numBloquesMapaInodos = BLOCKS_MAP_INODO; //el mapa de nodos para conocer si está libre o no el nodo
	sbloque[0].numBloquesInodos =  48; //El número de bloques de inodos solo puede haber hasta 48 inodos
	sbloque[0].primerBloqueDatos = sbloque[0].numBloquesInodos + sbloque[0].primerInodo; //El primer bloque de datos
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
	memset(inodos[indice].nomFichero, '\0', sizeof inodos[indice].nomFichero); //Lleno de carácteres nulos el nombre del fichero
	inodos[indice].referencia[0] =  sbloque[0].numBloquesInodos + sbloque[0].numBloquesDatos + sbloque[0].primerInodo -1;  //Añado EOF a la referencia 0
	inodos[indice].tamano = 0; //Cambio el tamaño
	inodos[indice].integridad= 0; //Cambio la integridad
}



/**
 * Para pasar de Inodos a Char
 * Entrada: array de caracteres y el indice
 * Salida: nada
*/
void inodotoChar(char* contenidoInodo, int indice){
	short auxiliar = 0;
	//Copio el nombre
	memcpy( contenidoInodo, inodos[indice].nomFichero, sizeof inodos[indice].nomFichero);
	auxiliar = sizeof inodos[indice].nomFichero; //Actualizo el valor de auxiliar
	//Copio las referencias
	memcpy( contenidoInodo + auxiliar, inodos[indice].referencia, sizeof inodos[indice].referencia);
	auxiliar= sizeof inodos[indice].referencia + auxiliar; //Actualizo el valor de auxiliar
	//Copio el tamaño
	memcpy( contenidoInodo + auxiliar, &inodos[indice].tamano, sizeof inodos[indice].tamano);
	auxiliar= sizeof inodos[indice].tamano+ auxiliar; //Actualizo el valor de auxiliar
	//Copio la integridad
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
 * Imprime en pantalla los atributos de todos los inodos
 * Entrada: el bloque de se desea imprimir
 * Salida: nada
*/
void printfInodo(){
	for(int indice = 0; indice<sbloque[0].numBloquesInodos; indice++){
		printf("Inodo %i: %s %hu %hu %i\n", indice, inodos[indice].nomFichero, inodos[indice].referencia[0],
													inodos[indice].tamano, inodos[indice].integridad);
	}
}



/**
 * Lee el total del descriptor de ficheros y lo imprime
 * Entrada: nada
 * Salida: nada
*/
void printfFD(){
	for(int i = 0; i< sizeof fileDescriptor/ sizeof fileDescriptor[0]; i++){
		printf("FD num %i: %s, %i\n", i, fileDescriptor[i].nombre, fileDescriptor[i].punteroRW);
	}
}



/**
 * Escribe en panalla un mapa otorgado
 * Entrada: el mapa a leer, 1 mapa de inodos, 2 mapa de datos
 * Salida: nada
*/
void printfMapa(int mapa){
	char * mapaElegido;
	if(mapa == 1) mapaElegido = i_map;
	if(mapa == 2) mapaElegido = b_map;
	for(int i = 0; i<sizeof mapaElegido; i++){
		printf("%hu", bitmap_getbit(mapaElegido,i));
	}printf("\n");

	}