
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