void TipoChartoInodo(char* contenidoInodo, int indice){
	short auxiliar = 0;
	//Primer bloque donde se encuentra los bloques de Inodos
	memcpy( &inodoAux[indice].nomFichero, contenidoInodo[indice], sizeof inodoAux[indice].nomFichero);
	auxiliar = sizeof inodoAux[indice].nomFichero; //Actualizo el valor de auxiliar
	memcpy( &inodoAux[indice].referencia, contenidoInodo[indice][auxiliar], sizeof inodoAux[indice].referencia);
	auxiliar= sizeof inodoAux[indice].referencia+ auxiliar; //Actualizo el valor de auxiliar
	memcpy( &inodoAux[indice].referenciaSig, contenidoInodo[indice][auxiliar], sizeof inodoAux[indice].referenciaSig);
	auxiliar= sizeof inodoAux[indice].referenciaSig+ auxiliar; //Actualizo el valor de auxiliar
	memcpy( &inodoAux[indice].tamano, contenidoInodo[indice][auxiliar], sizeof inodoAux[indice].tamano);
	auxiliar= sizeof inodoAux[indice].tamano+ auxiliar; //Actualizo el valor de auxiliar
	memcpy( &inodoAux[indice].punteroRW, contenidoInodo[indice][auxiliar], sizeof inodoAux[indice].punteroRW);
	auxiliar= sizeof inodoAux[indice].punteroRW+ auxiliar; //Actualizo el valor de auxiliar
	memcpy( &inodoAux[indice].integridad, contenidoInodo[indice][auxiliar], sizeof inodoAux[indice].integridad);

}


typedef struct {
char nomFichero[32]; //nombre del fichero
unsigned short referencia; //dirección del primer bloque de datos
unsigned short referenciaSig; //dirección del segundo bloque de datos
unsigned short tamano; //tamano del fichero
unsigned short punteroRW; //localización del puntero de lectura y escritura
} TipoInodo;