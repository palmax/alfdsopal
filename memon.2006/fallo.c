/*
 *  memon/fallo.c
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene la rutina que trata el fallo de página.
 * Esta rutina se encarga de llevar las estadísticas de la monitorización
 *
 *      SE DEBE MODIFICAR
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>

#include "mapa.h"
#include "marcos.h"
#include "apoyo.h"

/* Variables que almacenan las estadísticas de la monitorización */
int fallos_total=0;

/* ¿El fallo de página se debe a la falta de memoria física? 
*/
int fallos_no_forzados=0;  /* Sí */
int fallos_forzados=0; /* No, ocurre debido a la carga por demanda */

/* ¿El fallo de página ha causado un reemplazo? */
int fallos_sin_reemplazo=0;
int fallos_con_reemplazo=0;

/* ¿El fallo de página implica una operación de lectura de fichero, de
   swap o no implica lectura? */
int fallos_sin_lectura=0; /* sin lectura (o sea, rellenar con 0) */
int fallos_con_lectura_fichero=0; /* con lectura de fichero */
int fallos_con_lectura_swap=0; /* con lectura de swap */

/* Número de operaciones de escritura (pageouts) */
int escrituras_en_fichero=0;	/* se ha escrito en fichero */
int escrituras_en_swap=0;	/* se ha escrito en swap */

/* Rutina de reemplazo que se va a usar (iniciado por memon.c) */
int (*reemplazo)();

/* Rutina que trata el fallo de página */

void fallo_pagina(void *dir_fallo) {
	printf("-------- fallo %p -------------\n", dir_fallo);
	return;
}
