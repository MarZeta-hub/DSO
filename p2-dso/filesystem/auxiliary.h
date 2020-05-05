
/*
 *
 * Operating System Design / Diseno de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	Last revision 01/04/2020
 *
 */

short BLOCKS_MAP_INODO = 1;
short INODO_SIZE = 12;
short BLOCKS_MAPS_DATA = 1;

 /*Hemos realizado unsigned short debido a que no necesitamos números
   negativos por lo que el short llega a un límite más alto. Por otra 
   parte, como no son números muy altos, no es necesario ocupar más 
   espacio de lo debido.*/
typedef struct {
unsigned short numBloquesMapaInodos; //Número de bloques del mapa inodos 
unsigned short numBloquesMapaDatos; //Número de bloques del mapa datos 
unsigned short numInodos; //Número de inodos en el dispositivo
unsigned short primerInodo; //Número bloque del 1o inodo del disp. (inodo raíz) 
unsigned short numBloquesDatos; // Número de bloques de datos en el dispositivo
unsigned short primerBloqueDatos; // Número de bloque del 1o bloque de datos 
unsigned int tamDispositivo; //Tamano total del disp. (en bytes)
} TipoSuperbloque;

typedef struct {
char nomFichero[32]; //nombre del fichero
unsigned short referencia; //dirección del primer bloque de datos
unsigned short referenciaSig; //dirección del segundo bloque de datos
unsigned short tamano; //tamano del fichero
unsigned short punteroRW; //localización del puntero de lectura y escritura
} TipoInodo;