/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */

//  Yo
struct tiempos_ejec {
  int usuario;
  int sistema;
};

typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;				/* ident. del proceso */
        int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	/* copia de regs. de UCP */
        void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */

  // Nuestro {
  int                 jiffies;
  struct tiempos_ejec tiempos;
  short int           nivel;
  short int           cuanto_queda_rodaja;
  short int           mutex_consultados[NUM_MUT_PROC];
  // }
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en semáforo, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};

/*
 *
 * Definición del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;


/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
// Yo
int sis_obtener_id_pr();
int sis_dormir();
int sis_tiempos_proceso();
// Yo <- Mutex
#define NO_RECURSIVO 0
#define RECURSIVO 1
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock();
int sis_unlock();
int sis_cerrar_mutex();
int sis_leer_caracter();
/*
 * Variable global que representa la cola de procesos que esperan por estar
 dormidos
 */
lista_BCPs lista_dormidos = {NULL, NULL};
/*
 * Variable global que representa si accede el parametro
 */
short int accediendo_parametro = 0;
int       tiempo_sistema = 0;  //  En el ppio dios dijo hagase la luz
/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{sis_obtener_id_pr},
					{sis_dormir},
					{sis_tiempos_proceso},
					{sis_crear_mutex},
					{sis_abrir_mutex},
					{sis_lock},
					{sis_unlock},
					{sis_cerrar_mutex},
					{sis_leer_caracter} };

struct descriptor {
  short int  tipo;
  char       nombre[MAX_NOM_MUT];
  BCP*       proceso_poseedor;
  lista_BCPs procesos_bloqueados;
  short int  cerrojos_bloqueados;
  short int  cerrojos_poseedor;
  short int  n_veces_abierto;  //  abrir_mutex++ y cerrar_mutex--
};

struct descriptor lista_mutex[NUM_MUT];
/*
struct lista_mutex_t{
    struct descriptor lista_descriptores[NUM_MUT];
} lista_mutex;
*/

lista_BCPs lista_bloqueados_mutex [NUM_MUT];

lista_BCPs lista_esperando_mutex= {NULL, NULL};

struct buffer_chars_t{
  char      chars[TAM_BUF_TERM];
  short int indice;
} buffer_chars;

/*
 * Variable global que representa la cola de procesos bloqueados por terminal
 */
lista_BCPs lista_bloqueados_terminal= {NULL, NULL};

#endif /* _KERNEL_H */
