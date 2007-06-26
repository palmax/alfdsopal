/*
 *  usuario/lib/serv.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene las definiciones de las funciones de interfaz
 * a las llamadas al sistema. Usa la funcion de apoyo llamsis
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#include "llamsis.h"
#include "servicios.h"

/* Función del módulo "misc" que prepara el código de la llamada
   (en el registro 0), los parámetros (en registros 1, 2, ...), realiza la
   instruccion de llamada al sistema  y devuelve el resultado 
   (que obtiene del registro 0) */

int llamsis(int llamada, int nargs, ... /* args */);


/*
 *
 * Funciones interfaz a las llamadas al sistema
 *
 */


int crear_proceso(char *prog){
	return llamsis(CREAR_PROCESO, 1, (long)prog);
}
int terminar_proceso(){
	return llamsis(TERMINAR_PROCESO, 0);
}
int escribir(char *texto, unsigned int longi){
	return llamsis(ESCRIBIR, 2, (long)texto, (long)longi);
}

// YO
int obtener_id_pr(){ // funciona
  return llamsis(OBTENER_ID_PR, 0);
}
  
int dormir(unsigned int segundos){
  return llamsis(DORMIR, 1, segundos);
}
int tiempos_proceso(struct tiempos_ejec *t_ejec){
  return llamsis(TIEMPO_PROCESOS, 1, (struct tiempos_ejec *)t_ejec);
};
int crear_mutex(char *nombre, int tipo){
  return llamsis(CREAR_MUTEX, 2, (char *)nombre, (int)tipo);
}
int abrir_mutex(char *nombre){
  return llamsis(ABRIR_MUTEX, 1, (char *)nombre);
}
int lock(unsigned int mutexid){
  return llamsis(LOCK, 1, (unsigned int)mutexid);
}
int unlock(unsigned int mutexid){
  return llamsis(UNLOCK, 1, (unsigned int) mutexid);
}
int cerrar_mutex(unsigned int mutexid){
  return llamsis(CERRAR_MUTEX, 1, (unsigned int) mutexid);
}

int leer_caracter() {
  return llamsis(LEER_CARACTER, 0);
}

