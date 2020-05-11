
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	test.c
 * @brief 	Implementation of the client test routines.
 * @date	01/03/2017
 *
 */


#include <stdio.h>
#include <string.h>
#include "filesystem/filesystem.h"
#include <stdlib.h>

// Color definitions for asserts
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE "\x1b[34m"

#define N_BLOCKS 290 			  // Number of blocks in the device
#define DEV_SIZE N_BLOCKS * BLOCK_SIZE // Device size, in bytes

int main()
{
	int ret;

	///////

	ret = mountFS();
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mountFS ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mountFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);


	////////
/*
	ret = createFile("/test3.txt");
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
*/
	///////

	ret = openFile("/test3.txt");
	if (ret == -1 || ret == -2)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFile ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFile ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	///////
	char* hola = "openFile: el objetivo principal de esta función es el de crear un nuevo fichero dentros del sistema, para ello, tras realizar diferentes comprobaciones de los parámetros de entrada a la función, se  comprueba que el fichero exista dentro del sistema de ficheros, si se da el caso de que ya existe, se devolver el descriptor donde este está abierto. En caso de que el fichero no exista, se busca un descriptor de fichero vacío y tras abrir el fichero mediante el uso de la función memcpy y situar el puntero de lectura-escritura al comienzo de este, se devuelve el descriptor de fichero vacío previamente encontrado.removeFile: esta función se implementa con el objetivo de que elimine un archivo del sistema de ficheros. En primer lugar comprueba la longitud del nombre que recibe la funcion para evitar que sea superior a 32 bytes, en segundo lugar comprueba mediante existeFichero si alguno de los inodos tiene como campo de nombre el nombre que recibe la función. En caso de que estas comprobaciones sean satisfactorias cierra el fichero en caso de que se encuentre abierto, vacía los campos del inodo asociado al archivo, actualiza los mapas de bits de los inodos y los datos para indicar que están libres y por último reduce una unidad al valor del superbloque que indica la cantidad de ficheros total que hay en el disco. unmountFS: función encargada de desmontar el sistema de fichero. Su funcionalidad consiste en escribir en disco los datos que el sistema de ficheros tiene en memoria para ello usa la función syncDisk() que escribe en el disco el contenido del superbloque, los mapas de bits y los Inodos. Tras realizar estas escrituras en el disco libera mediante free la memoria de los datos escritos en el disco.createFile: función que crea un nuevo fichero vacío en el el sistema de ficheros. Antes de realizar su función comprueba que no exista un fichero con el mismo nombre que el que se pretende crear, que no se haya alcanzado el número máximo de ficheros y que el nombre recibido por parámetros no ocupe más de 32  bytes. Una vez realizadas las comprobaciones busca un inodo vacío en el mapa de inodos mediante un for y la función bitmap_getbit(), si no se encuentra ninguno libre se devuelve -2 y se lanza un perror(). En caso de encontrar un Inodo vacio lo inicia escribiendo el campo de nombre del fichero con el nombre recibido por parámetro además de actualizar el mapa de inodos mediante bitmap_setbit() escribiendo un 1 en la posición del mapa correspondiente al inodo utilizado y por ultimo actualiza en el superbloque el número de ficheros totales. ;";
	
	ret = writeFile(0, hola, strlen(hola));
	if (ret == -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST write ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
			
	}
	
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST write ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	char buffer[4096];

	lseekFile(0,0,1);

	ret = readFile(0, buffer, 4096);
	printf("\n Leido: %i,   %s \n\n ", ret, buffer);
	if (ret == -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST read ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST read ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	////////

	ret = closeFile(0);
	if (ret == -1 || ret == -2)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFile ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFile ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	ret = unmountFS();
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST unmountFS ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST unmountFS ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	
	///////

	return 0;
}
