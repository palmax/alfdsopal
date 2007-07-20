/*
 *  memon/mapa.h
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene prototipos exportados por mapa.c
 * y la definición de la entrada de la tabla de regiones y de páginas.
 *
 *
 *      SE DEBE MODIFICAR PARA COMPLETAR LA DEFINICIÓN DE LA ENTRADA DE
 *	LA TABLA DE PÁGINAS. ADEMÁS, SE PUEDE MODIFICAR PARA INCLUIR
 *      FUNCIONES ADICIONALES (como, por ejemplo, para activar el
 *	de modificado o dada una dirección encontrar la página)
 *
 */

#ifndef _MAPA_H
#define _MAPA_H


/* Prototipos de las funciones usadas por el entorno de apoyo
   para notificar los cambios en el mapa del proceso.
	NO SE DEBEN MODIFICAR
*/

/* nos informan de que se ha creado una región
        dir: dirección de comienzo de la región
        inodo: fichero al que está vinculada (0 si es anónima)
        prot: permisos de acceso a la región
        tamano: tamaño de la región
	compartida: ¿es una región de tipo compartida?
*/
void creacion_region(void *dir, int inodo, int prot,
			int tamano, int compartida);
/* nos informan de que ha cambiado el tamaño de una región
        dir: dirección de comienzo de la región
        tamano: nuevo tamaño de la región
*/
void cambio_tam_region(void *dir, int tamano);
/* nos informan de que se ha eliminado una región
        dir: dirección de comienzo de la región
*/
void eliminacion_region(const void *dir);

/* Definiciones de las entradas de la tabla de páginas y de regiones */

struct entrada_tabla_regiones_t;

/* Tipo que define una entrada en la tabla de páginas */
typedef struct {
        void *dir_inicial;	/* dirección inicial de la página */
	struct entrada_tabla_regiones_t *region; /* región a la que pertenece */
} entrada_tabla_paginas;


/* Tipo que define una entrada en la tabla de regiones */
typedef struct entrada_tabla_regiones_t {
        int usada;
        void *dir_inicial;	/* dirección inicial de la región */
        int ino;		/* inodo del fichero o 0 si anónima */
        int prot;		/* protección de la región */
        int npags;		/* tamaño en páginas */
        int compartida;		/* ¿región compartida? */
        entrada_tabla_paginas *tabla_paginas;/* Tabla de páginas de la región */
} entrada_tabla_regiones;

/* Funciones auxiliares que a partir de la dirección de una entrada
   obtienen su posición en la tabla correspondiente */
int regnum(entrada_tabla_regiones *reg);

int pagnum(entrada_tabla_paginas *pag);

/* Función auxiliar que a partir de un número de página de una
región devuelve la dirección de su entrada en la tabla */
entrada_tabla_paginas *entrada_pagina(int regnum, int pagnum);

#endif /* _MAPA_H */
