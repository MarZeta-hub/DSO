
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	Last revision 01/04/2020
 *
 */
short BLOCKS_MAP_INODO = 1;
short INODO_SIZE = 12;
short BLOCKS_MAPS_DATA = 1;

#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}

 /*Hemos realizado unsigned short debido a que no necesitamos números
   negativos por lo que el short llega a un límite más alto. Por otra 
   parte, como no son números muy altos, no es necesario ocupar más 
   espacio de lo debido.*/

typedef struct {
unsigned short primerInodo; //Número bloque del 1o inodo del disp. (inodo raíz) 
unsigned short numBloquesMapaInodos; //Número de bloques del mapa inodos
unsigned short numBloquesInodos; //Número de inodos en el dispositivo 
unsigned short primerBloqueDatos; // Número de bloque del 1o bloque de datos 
unsigned short numBloquesMapaDatos; //Número de bloques del mapa datos 
unsigned short numBloquesDatos; // Número de bloques de datos en el dispositivo
unsigned int tamDispositivo; //Tamano total del disp. (en bytes)
} TipoSuperbloque;

typedef struct {
char nomFichero[32]; //nombre del fichero
unsigned short referencia; //dirección del primer bloque de datos
unsigned short referenciaSig; //dirección del segundo bloque de datos
unsigned short tamano; //tamano del fichero
unsigned short punteroRW; //localización del puntero de lectura y escritura
} TipoInodo;

TipoSuperbloque* sbloque;
char* i_map;
char* b_map;
TipoInodo* inodos;
char* prueba; //elikminar esto
