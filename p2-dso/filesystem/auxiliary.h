
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

//función para crear el char para agregar los datos al super bloque
void createSuperBloque(int tamanoDisco, char* contenidoSB);

//Para pasar una estructura TipoSuperBloque a Char
void sBtoChar(char* contenidoSB);

//Para pasar una estructura Char a TipoSuperBloque
void chartoSB(char* contenidoSB);

//Para pasar de una estructura char a TipoInodo
void chartoInodo(char* contenidoInodo, int indice);

//Para pasar crear una estructura inodo formateada
void inodoVacio();

//Para añadir de una estructura inodo a char array
void inodotoChar(char* contenido, int indice);

//Para obtener los datos de Superbloque
void printfSB();

//Para obtener los datos de un inodo
void printInodo(int indice);

//El sistema de actualizar el superbloque
void SbloquetoChar(char* contenido);

//Se utiliza para sincronizar los cambios con el disco
int syncDisk();