
/* 
   Programa de prueba de la práctica del monitor de memoria.
   Realiza una serie de cálculos absurdos usando vectores.
   Recibe como parámetros dos ficheros:
	- el primero representa un vector que será tanto operando
	como resultado de la operación (proyección MAP_SHARED).
	- el segundo representa un vector que será sólo operando
	de la operación (proyección MAP_PRIVATE).

   En la evaluación de la práctica se usarán como argumentos los ficheros
   "vector_resultado" y "vector_operando", respectivamente.

	prueba1 vector_resultado vector_operando

   Dado que el programa modifica el primer vector, se proporciona una copia
   del mismo (fichero "vector_resultado.orig") para poder restaurar
   el fichero "vector_resultado" cuando quiera realizarse la misma
   prueba usada en la evaluación.

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#define MAX 3500

/* longitud de vectores proyectados */
int tam;

/* vectores proyectados */
unsigned long *vector_resultado;
unsigned long *vector_operando;

/* Usamos otro vector auxiliar */
unsigned long vector_auxiliar[MAX];

int i; /* como no monitorizamos la pila, definimos el índice como global */

unsigned long minimo=ULONG_MAX; /* para calcular el mínimo de los vectores */
unsigned long maximo=0; /* para calcular el máximo de los vectores */

int main(int argc, char **argv)
{
	struct stat stat1;
	struct stat stat2;
	int df1, df2;

	if (argc!=3) {
		fprintf (stderr, "Uso: %s vector_resultado vector_operando\n",
					argv[0]);
		return(1);
	}

	/* Abre el primer archivo para lectura/escritura */
        if ((df1=open(argv[1], O_RDWR))<0) {
                perror("No puede abrirse el primer archivo");
                return(1);
        }

	/* Abre el segundo archivo sólo para lectura */
        if ((df2=open(argv[2], O_RDONLY))<0) {
                perror("No puede abrirse el segundo archivo");
		close(df1);
                return(1);
        }

	/* Averigua la longitud de archivos */
        if ((fstat(df1, &stat1)<0) || (fstat(df2, &stat2)<0)) {
                perror("Error en fstat de archivo");
                close(df1); close(df2);
                return(1);
        }

	/* Comprueba que tienen la misma longitud y ésta en menor que MAX */
	if ((stat1.st_size>MAX*sizeof(unsigned long)) ||
			(stat2.st_size!=stat1.st_size)) {
		fprintf (stderr, "longitud de ficheros incorrecta\n");
                close(df1); close(df2);
		return(1);
	}

	tam=stat1.st_size/sizeof(unsigned long);

	/* Se proyecta el primer archivo como compartido */
        if ((vector_resultado=mmap((caddr_t) 0, stat1.st_size,
			PROT_READ|PROT_WRITE, MAP_SHARED, df1, 0)) == 
				(void *)MAP_FAILED) {
                perror("Error en la proyeccion de primer archivo");
                close(df1); close(df2);
                return(1);
        }

	/* Se proyecta el segundo archivo como privado. Se proyecta
	   con permiso de escritura ya que se va a modificar,
	   aunque los cambio sobre el mismo no vayan a persistir. */
        if ((vector_operando=mmap((caddr_t) 0, stat2.st_size,
			PROT_READ|PROT_WRITE, MAP_PRIVATE, df2, 0)) ==
				(void *)MAP_FAILED) {
                perror("Error en la proyeccion de primer archivo");
                close(df1); close(df2);
                return(1);
        }

	/* Primer bucle: compara vector_operando[i] y vector_resultado[i]
	   y almacena el mayor en vector_auxiliar y el menor en el propio
	   vector_operando. Además, calcula el máximo y mínimo */

        for (i=0; i<tam; i++) {
		if (vector_resultado[i]>=vector_operando[i])
			vector_auxiliar[i]=vector_resultado[i];
		else {
			vector_auxiliar[i]=vector_operando[i];
			vector_operando[i]=vector_resultado[i];
		}
		if (vector_auxiliar[i] > maximo)
			maximo=vector_auxiliar[i];
		if (vector_operando[i] < minimo)
			minimo=vector_operando[i];
	}

	/* Segundo bucle: actualiza vector_resultado usando los valores
	   almacenados en vector_operando. */
	for (i=0; i<tam; i++)
		vector_resultado[i]=vector_operando[i]-minimo;

	/* Ya no se necesita vector_operando: Se elimina la proyeccion */
        munmap(vector_operando, stat2.st_size);

	/* Tercer bucle: actualiza vector_resultado usando los valores
	   almacenados en vector_auxiliar. */
	for (i=0; i<tam; i++)
		vector_resultado[i]+=maximo-vector_auxiliar[i];

        return 0;
}

