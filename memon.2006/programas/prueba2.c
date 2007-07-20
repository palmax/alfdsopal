
/* 
   Programa de prueba de la práctica del monitor de memoria.
   Usa el heap para probar si el monitor gestiona correctamente el
   cambio de tamaño de una región

 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define TAM 2500

int main(int argc, char **argv)
{
	int i;
	int *zona1, *zona2;

	/* Reserva dos zonas del mismo tamaño */
	zona1= malloc(TAM*sizeof(int));
	zona2= malloc(TAM*sizeof(int));

	/* Hace unos cálculos tontos */
        for (i=0; i<TAM; i++) {
		zona1[i]=rand();
		zona2[i]=RAND_MAX-rand();
	}

	/* aumenta la segunda zona */
	zona2= realloc(zona2, 2*TAM*sizeof(int));

	/* Más cálculos tontos */
        for (i=0; i<TAM; i++) {
		zona2[i+TAM]=zona2[i]-zona1[i];
		zona1[i]=zona2[i+TAM]/2;
	}

	/* disminuye la segunda zona al tamaño inicial */
	zona2= realloc(zona2, TAM*sizeof(int));

	/* Más cálculos tontos */
        for (i=0; i<TAM; i++)
		zona2[i]+=zona1[i];

	/* Libera la segunda zona */
	free(zona2);
	
        return 0;
}

