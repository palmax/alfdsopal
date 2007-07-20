/*
 *  memon/apoyo.h
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene macros de utilidad para manejar la
 * máscara de protección y el prototipo de ejecutar_programa y tratar_SEGV
 *
 *      NO SE DEBE MODIFICAR
 *
 */

#ifndef _APOYO_H
#define _APOYO_H


/* Macros que facilitan el manejo de la máscara de protección */

/* PROT_ISSET:	Permite comprobar si en la proteccion está activo un 
		determinado permiso.
		Ejemplo: comprueba si en la máscara está activo
		el permiso de escritura
 
			if (PROT_ISSET(PROT_WRITE,prot))
*/

#define PROT_ISSET(prot_flag, proteccion) (prot_flag & proteccion)

/* PROT_SET:	Activa un determinado permiso en una máscara de
		protección devolviendo la máscara actualizada.
		Ejemplo: Añade a una máscara el permiso de escritura

			prot=PROT_SET(PROT_WRITE,prot);
*/
#define PROT_SET(prot_flag, proteccion) (proteccion | prot_flag )


/* PROT_CLR:	Desactiva un determinado permiso en una máscara de
		protección devolviendo la máscara actualizada.
		Ejemplo: Elimina en una máscara el permiso de escritura

			prot=PROT_CLR(PROT_WRITE,prot);
*/
#define PROT_CLR(prot_flag, proteccion) (proteccion & ~prot_flag)


/* Rutina que establece qué función realiza el tratamiento de SEGV */
void tratar_SEGV(void (*funcion)(void *));

/* Rutina que arranca la ejecucion del programa */
void ejecutar_programa(int argc, char **args);

#endif /* _APOYO_H */

