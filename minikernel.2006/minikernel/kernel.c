/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */
//  Buscar lista_listos (entre otras cosas) y poner NIVEL_3 cuando se toque


/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */

#include "string.h"
/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Función que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Función que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	//	printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al mínimo el nivel de interrupción mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	lista_listos.primero->estado = EJECUCION;
	fijar_nivel_int(lista_listos.primero->nivel);
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;
	int nivel_anterior;

	nivel_anterior = fijar_nivel_int(NIVEL_3);
	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	fijar_nivel_int(nivel_anterior);

        return; /* no debería llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

  //  Yo he cambiado cosas
  if (!viene_de_modo_usuario())
      if (!accediendo_parametro)
      {
	panico("excepcion de memoria cuando estaba dentro del kernel");
      }
  
  printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
  liberar_proceso();

  return; /* no debería llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char caracter;
	BCP *p_proc_anterior;
	int nivel_anterior;


	caracter = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", caracter);

	if (buffer_chars.siguiente_libre +1 != buffer_chars.indice) {
	  buffer_chars.chars[buffer_chars.siguiente_libre] = caracter;
	  buffer_chars.siguiente_libre++;
	  printk("buffer_chars.siguiente_libre = %d\n", buffer_chars.siguiente_libre);
	  if (buffer_chars.siguiente_libre == TAM_BUF_TERM)
	    buffer_chars.siguiente_libre = 0;
	  /*
	  if (buffer_chars.siguiente_libre == buffer_chars.indice)
	    buffer_chars.siguiente_libre = -1;
	  */
	}

	//  despierto a los que esten dormidos esperando un caracter
	if (lista_bloqueados_terminal.primero) {
	  nivel_anterior                     = fijar_nivel_int(NIVEL_3);
	  p_proc_anterior                    = p_proc_actual;
	  p_proc_actual                      = lista_bloqueados_terminal.primero;
	  p_proc_actual->nivel               = nivel_anterior;
	  p_proc_actual->cuanto_queda_rodaja = TICKS_POR_RODAJA;
	  p_proc_actual->estado              = LISTO;
	  eliminar_elem(&lista_bloqueados_terminal, p_proc_actual);  //  quitamos de lista_listos
	  insertar_ultimo(&lista_listos, p_proc_actual);  //  metemos
	  p_proc_actual                      = planificador();
	  cambio_contexto(&(p_proc_anterior->contexto_regs),
			  &(p_proc_actual->contexto_regs));  //  cambio a listo
	  printk("-> %d ha sido despertado de esperar un char gracias a %c\n",p_proc_actual->id, caracter);
	  fijar_nivel_int(nivel_anterior);
	}  //  sino no hacemos nada

        return;
}

/*
 * Tratamiento de interrupciones software
 */
static void int_sw(){  //  Falta lo de las llamadas al sistema
  BCP * p_proc_anterior = p_proc_actual;
  int nivel_anterior;  
	printk("-> TRATANDO INT. SW\n");
	//  Yo
	//  Suponemos que solo hay un tipo de int_sw
	//  Tenemos que coger el p_proc_actual y hacer un change
	//  con el siguiente y luego ponerlo en la cola de listos
	nivel_anterior = fijar_nivel_int(NIVEL_3);  // tocamos la lista_listos
	/// if (nivel_anterior == 2) hacer algo (encolar?)
	/// if (nivel_anterior == 1) hacer algo (encolar?)
	/// if (nivel_anterior == 0) normal
	if (p_proc_actual && p_proc_actual->siguiente && p_proc_actual->estado == EJECUCION) {
	  printk("-> usamos round robin\n");
	  p_proc_actual                  = p_proc_actual->siguiente;
	  lista_listos.ultimo->siguiente = p_proc_anterior;
	  lista_listos.ultimo            = p_proc_anterior;
	  lista_listos.primero           = p_proc_actual;
	  p_proc_anterior->siguiente     = NULL;
	  cambio_contexto(&(p_proc_anterior->contexto_regs),
			  &(p_proc_actual->contexto_regs));  //  cambio a listo
	  p_proc_anterior->estado = BLOQUEADO;
	  printk("-> hecho RR\n");
	  //  quitamos de lista_listos
	  //	  eliminar_elem(&lista_listos, p_proc_anterior);
	  //	  cambio_contexto(&(p_proc_anterior->contexto_regs),
	  //			  &(p_proc_actual->contexto_regs));  //  cambio a listo
	  //	  insertar_ultimo(&lista_listos, p_proc_anterior);  //  metemos al final
	}
	//  Tanto si hay mas como si no le doy ticks
	p_proc_anterior->cuanto_queda_rodaja = TICKS_POR_RODAJA;
	fijar_nivel_int(nivel_anterior);	
	return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){
  BCP *paux_ant = NULL;
  BCP *paux     = lista_dormidos.primero;
  BCP *paux_sig;

  //  printk("-> TRATANDO INT. DE RELOJ\n");

  if (p_proc_actual && p_proc_actual->estado == EJECUCION) {
    printk("-> p_proc_actual->estado es %d\n", p_proc_actual->estado);
      if (viene_de_modo_usuario())
	p_proc_actual->tiempos.usuario++;
      else
	p_proc_actual->tiempos.sistema++;
  }

  tiempo_sistema++;  //  Viven

  if (paux)
    paux_sig = paux->siguiente;

  while (paux) {
    paux->jiffies = paux->jiffies - 1;
    if (paux->jiffies == 0) {
      if (paux_ant) {
	printk("-> %d siguiendo a %d\n",paux->id, paux->siguiente);
	paux_ant->siguiente = paux_sig;
      }
      paux->estado= LISTO;
      printk("-> %d listo\n",paux->id);
      eliminar_elem(&lista_dormidos, paux);
      insertar_ultimo(&lista_listos, paux);
      //  hay que planificar?
      paux = paux_sig;
      if (paux_sig)
	paux_sig = paux_sig->siguiente;
    } else {
      paux_ant = paux;
      paux = paux_sig;
      if (paux_sig)
	paux_sig = paux_sig->siguiente;
    }
  }

  if (p_proc_actual) {
    p_proc_actual->cuanto_queda_rodaja = p_proc_actual->cuanto_queda_rodaja - 1;
    if (p_proc_actual->cuanto_queda_rodaja == 0) {
      activar_int_SW();
    }
  }


  return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	short int i;
	BCP *p_proc;
	//  Yo
	int nivel_anterior;


	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;
		//  Yo
		p_proc->siguiente       = NULL;
		p_proc->jiffies         = 0;
		p_proc->tiempos.usuario = 0;
		p_proc->tiempos.sistema = 0;
		p_proc->nivel           = 0;  //  innecesario?
		for (i=0;i<NUM_MUT_PROC;i++) {
		    p_proc->mutex_consultados[i] = -1;
//		    p_proc->mutex_abiertos[i] = -1;
		}
//		p_proc->indice_mutex_consultados = 0;  //  usar %
//		p_proc->indice_mutex_abiertos = 0;  //  usar %
		p_proc->cuanto_queda_rodaja   = TICKS_POR_RODAJA;
		/* lo inserta al final de cola de listos */
		nivel_anterior = fijar_nivel_int(NIVEL_3);
		insertar_ultimo(&lista_listos, p_proc);
		fijar_nivel_int(nivel_anterior);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;


	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);

	return res;


}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no debería llegar aqui */
}

//  Yo
/*
 * Tratamiento de llamada al sistema obtener_id_pr
 */
int sis_obtener_id_pr(){

    int res = p_proc_actual->id;

    printk("-> EL PID DEL PROCESO ES %d\n", res);

    return res;
}

/*
 * Tratamiento de llamada al sistema dormir
 */
int sis_dormir(){

  unsigned int segundos;
  int jiffies;
  BCP * p_proc_anterior;
  int nivel_anterior;

  segundos=(unsigned int)leer_registro(1);
  jiffies = TICK * segundos;  //  nº jiffies que quiere dormir
  if (jiffies) {
    printk("-> EL PROCESO QUIERE DORMIR %d SEGUNDOS <=> %d JIFFIES\n",
	   segundos, jiffies);
    nivel_anterior = fijar_nivel_int(NIVEL_3);
    p_proc_actual->nivel = nivel_anterior;
    p_proc_actual->jiffies = jiffies;
    p_proc_actual->estado=BLOQUEADO;
    p_proc_anterior = p_proc_actual;
    eliminar_elem(&lista_listos, p_proc_actual);  //  quitamos de lista_listos
    insertar_ultimo(&lista_dormidos, p_proc_actual);  //  metemos en dormidos
    p_proc_actual = planificador();  //  elijo el siguiente
    cambio_contexto(&(p_proc_anterior->contexto_regs),
		    &(p_proc_actual->contexto_regs));  //  cambio a listo
    fijar_nivel_int(nivel_anterior);
  }
  return segundos;  // Suponemos segundo
}

/*
 * Tratamiento de llamada al sistema tiempos_proceso
 */
int sis_tiempos_proceso(){
  struct tiempos_ejec *t_ejec;
  int    nivel_anterior = fijar_nivel_int(NIVEL_3);

  t_ejec = (struct tiempos_ejec *)leer_registro(1);
  printk("-> EL PROCESO QUIERE CONSULTAR LOS TIEMPOS DE PROCESO\n");

  if (!t_ejec) {
    printk("-> el tiempo desde el big bang: %d\n", tiempo_sistema);
  } else {
    accediendo_parametro = 1;
    t_ejec->usuario = p_proc_actual->tiempos.usuario;
    t_ejec->sistema = p_proc_actual->tiempos.sistema;
    accediendo_parametro = 0;
  }
  fijar_nivel_int(nivel_anterior);

  return (tiempo_sistema);
}


/*
 * Tratamiento de llamada al crear_mutex
 */
int sis_crear_mutex(){
  char *nombre         = (char *)leer_registro(1);
  int tipo             = (int)leer_registro(2);
  short int i          = 0;
  short int j          = 0;
  short int hay_hueco  = 0;
  BCP *p_proc_anterior = p_proc_actual;

  printk("-> EL PROCESO QUIERE CREAR UN MUTEX");
  while ((i != NUM_MUT) && (!hay_hueco)) {
    if (lista_mutex[i].tipo == -1)
      hay_hueco = 1;
    else
      i++;
  }
  
  if (!hay_hueco) {
    // a reencolar y a esperar a que un majete nos pase un sitio bloquear
    p_proc_actual->estado = BLOQUEADO;
    eliminar_elem(&lista_listos, p_proc_actual);      //  quitar de lista_listos
    p_proc_actual = planificador();                    //  elijo el siguiente
    cambio_contexto(&(p_proc_anterior->contexto_regs),
		    &(p_proc_actual->contexto_regs));  //  cambio a uno listo
    insertar_ultimo(&lista_bloqueados_mutex,
		    p_proc_anterior);                  //  metemos al final
  }
  
  hay_hueco = 0;
  j = 0;  //  p_proc_actual->indice_mutex_consultados;
  while ((j != NUM_MUT_PROC) && (!hay_hueco)) {
    if (p_proc_actual->mutex_consultados[j] == -1)
      hay_hueco = 1;
    else
      j++;
  }
  
  if (hay_hueco) {
    p_proc_actual->mutex_consultados[j] = i;
    lista_mutex[i].tipo = tipo;
    //  innecesario?
    memset(lista_mutex[i].nombre, 0, MAX_NOM_MUT);
    strncpy(lista_mutex[i].nombre,
	    nombre, MAX_NOM_MUT);
    lista_mutex[i].proceso_poseedor            = NULL;
    lista_mutex[i].procesos_bloqueados.primero = NULL;
    lista_mutex[i].procesos_bloqueados.ultimo  = NULL;
    lista_mutex[i].cerrojos_bloqueados         = 0;
    lista_mutex[i].cerrojos_poseedor           = 0;
    lista_mutex[i].n_veces_abierto             = 0;
    return i;  //  <- retornamos i o j?
  } else {
    return -1;
  }
  return -2;
}

/*
 * Tratamiento de llamada al abrir_mutex
 */
int sis_abrir_mutex(){
  char *nombre         = (char *)leer_registro(1);
  short int i          = 0;
  short int j          = 0;
  short int encontrado = 0;
  
  //  Hacemos cosas
  printk("-> EL PROCESO QUIERE ABRIR UN MUTEX");
  
  if (i >= NUM_MUT_PROC)
    return -1; // te estas pasando

  while ((i != NUM_MUT) && (!encontrado)) {
    j = p_proc_actual->mutex_consultados[i];
    if ((j != -1) && (j < NUM_MUT) &&
	(!strncmp (lista_mutex[j].nombre, nombre,
		   MAX_NOM_MUT)))
      encontrado = 1;
    else
      i++;
  }
  
  if (encontrado) {
    lista_mutex[j].n_veces_abierto++;  //  Abierto otra
    return j;
  }
  else
    return -2;
}


/*
 * Tratamiento de llamada al lock
 */
int sis_lock(){
  unsigned int mutexid = (unsigned int)leer_registro(1);
  short int i          = 0;  //  p_proc_actual->indice_mutex_consultados;
  short int encontrado = 0;
  BCP *p_proc_anterior = p_proc_actual;

  printk("-> EL PROCESO QUIERE LOCKEAR UN MUTEX\n");

  if ((mutexid < 0) || (mutexid >= NUM_MUT))
    return -1;

  while ((i != NUM_MUT) && (!encontrado)) {
    if ((i != -1) && (i < NUM_MUT) &&
	(p_proc_actual->mutex_consultados[i] == mutexid))
      encontrado = 1;
    else
      i++;
  }
  if (!encontrado)
    return -2;
  else {
    if (lista_mutex[mutexid].proceso_poseedor == NULL) {
      lista_mutex[mutexid].proceso_poseedor = p_proc_actual;
      lista_mutex[mutexid].cerrojos_poseedor = 1;
    } else if ((lista_mutex[mutexid].proceso_poseedor ==
		p_proc_actual) && 
	       (lista_mutex[mutexid].tipo == RECURSIVO)) {
      lista_mutex[mutexid].cerrojos_poseedor ++;
    } else {
      lista_mutex[mutexid].cerrojos_bloqueados ++;
      //bloquear
      p_proc_actual->estado = BLOQUEADO;
      eliminar_elem(&lista_listos, p_proc_actual);  //  quitamos de lista_listos
      p_proc_actual = planificador();                    //  elijo el siguiente
      cambio_contexto(&(p_proc_anterior->contexto_regs),
		      &(p_proc_actual->contexto_regs));  //  cambio a uno listo
      insertar_ultimo(&lista_mutex[mutexid].procesos_bloqueados,
		      p_proc_anterior);                  //  metemos al final
    }
    return 0;
  }
}

/*
 * Tratamiento de llamada al unlock
 */
int sis_unlock() {
  unsigned int mutexid  = (unsigned int)leer_registro(1);
  short int i           = 0;  //  p_proc_actual->indice_mutex_consultados;
  short int encontrado  = 0;
  BCP *p_proc_anterior  = p_proc_actual;
  BCP *paux             = NULL;
  
  printk("-> EL PROCESO QUIERE UNLOCKEAR UN MUTEX\n");
  
  if ((mutexid < 0) || (mutexid >= NUM_MUT))
    return -1;
  
  while (((i % NUM_MUT_PROC) != NUM_MUT)
	 && (!encontrado)) {
    if ((i != -1) && (i < NUM_MUT) &&
	(p_proc_actual->mutex_consultados[i] == mutexid))
      encontrado = 1;
    else
      i++;
  }
  if (!encontrado)
    return -2;
  else {
    if (lista_mutex[mutexid].proceso_poseedor == NULL) {
      return -3;
    } else if (lista_mutex[mutexid].proceso_poseedor !=
	       p_proc_actual)
      return -4;
    else {
      if (((lista_mutex[mutexid].tipo == RECURSIVO) &&
	   (lista_mutex[mutexid].cerrojos_poseedor >= 1)) ||
	  ((lista_mutex[mutexid].tipo == NO_RECURSIVO) &&
	   (lista_mutex[mutexid].cerrojos_poseedor == 1))) {
	lista_mutex[mutexid].cerrojos_poseedor --;
	if (lista_mutex[mutexid].cerrojos_poseedor == 0) {
	  //  el poseedor hay que desbloquearlo y que lo pille otro si existe
	  p_proc_actual->estado = LISTO;
	  eliminar_elem(&lista_mutex[mutexid].procesos_bloqueados,
			p_proc_actual);  //  quitamos de lista mutex bloqueados
	  insertar_ultimo(&lista_listos,
			  p_proc_anterior);  //  metemos al final
	  p_proc_actual = planificador();  //  elijo el siguiente
	  cambio_contexto(&(p_proc_anterior->contexto_regs),
			  &(p_proc_actual->contexto_regs));  //  cambio a uno listo
	  
	  if (lista_mutex[mutexid].cerrojos_bloqueados == 0)
	    lista_mutex[mutexid].proceso_poseedor = NULL;
	  else if (lista_mutex[mutexid].cerrojos_bloqueados > 0) {
	    paux = 
	      lista_mutex[mutexid].procesos_bloqueados.primero;
	    eliminar_elem(&lista_mutex[mutexid].procesos_bloqueados,
			  paux);  //  quitamos de lista mutex bloqueados
	    lista_mutex[mutexid].cerrojos_bloqueados --;
	  } else { // < 0
	    printk ("Los cerrojos_bloqueados son <0 y deberian ser >=0/n");
	    return -5;
	  }
	}
      } else {
	printk ("Los cerrojos_poseedor son <0 y deberian ser >=0/n");
	return -6;
      }
      return 0;
    }
  }
}


/*
 * Tratamiento de llamada al cerrar_mutex
 */
int sis_cerrar_mutex(){
  unsigned int mutexid;
  mutexid = (unsigned int)leer_registro(1);
  short int i           = 0;  //  p_proc_actual->indice_mutex_consultados;
  short int encontrado  = 0;
  BCP *p_proc_anterior  = p_proc_actual;

  printk("-> EL PROCESO QUIERE CERRAR UN MUTEX  <- sin implementar\n");

  if ((mutexid < 0) || (mutexid >= NUM_MUT))
    return -1;


  for (i=0; (i < NUM_MUT_PROC) && (!encontrado); i++) {
    if (p_proc_actual->mutex_consultados[i] == mutexid)
      encontrado = 1;
    else
      i++;
  }
  if (!encontrado)
    return -2;
  
  p_proc_actual->mutex_consultados[i] = -1;

  if (lista_mutex[mutexid].n_veces_abierto == 1) {
    lista_mutex[mutexid].tipo                        = -1;
    memset(lista_mutex[mutexid].nombre, 0, MAX_NOM_MUT);
    lista_mutex[mutexid].proceso_poseedor            = NULL;
    lista_mutex[mutexid].procesos_bloqueados.primero = NULL;
    lista_mutex[mutexid].procesos_bloqueados.ultimo  = NULL;
    lista_mutex[mutexid].cerrojos_bloqueados         = 0;
    lista_mutex[mutexid].cerrojos_poseedor           = 0;
    lista_mutex[mutexid].n_veces_abierto             = 0;

    if (lista_bloqueados_mutex.primero) {
      p_proc_actual = lista_bloqueados_mutex.primero; 
      p_proc_actual->estado = LISTO;
      eliminar_elem(&lista_bloqueados_mutex,
		    p_proc_actual);  //  quitamos de lista bloqueados mutex
      insertar_ultimo(&lista_listos,
		      p_proc_anterior);  //  metemos al final
      p_proc_actual = planificador();  //  elijo el siguiente
      cambio_contexto(&(p_proc_anterior->contexto_regs),
		      &(p_proc_actual->contexto_regs));  //  cambio a uno listo
    }
  }

  return (0);  //  retornamos lo que toque
}


/*
 * Tratamiento de llamada a leer_caracter
 */
int sis_leer_caracter(){
  char caracter_a_devolver;
  BCP * p_proc_anterior;
  int nivel_anterior;

  printk("-> EL PROCESO QUIERE LEER UN CARACTER\n");

  /*
    leer_caracter() {
      Si (buffer vacío)
           bloquear el proceso
      extraer el carácter del buffer y devoverlo como resultado de la función
  */

  if (buffer_chars.indice == buffer_chars.siguiente_libre) { // no hay chars
    // bloqueo
    nivel_anterior                     = fijar_nivel_int(NIVEL_3);
    p_proc_anterior                    = p_proc_actual;
    p_proc_actual->nivel               = nivel_anterior;
    p_proc_actual->cuanto_queda_rodaja = TICKS_POR_RODAJA;
    p_proc_actual->estado              = BLOQUEADO;
    p_proc_anterior                    = p_proc_actual;
    eliminar_elem(&lista_listos, p_proc_actual);  //  quitamos de lista_listos
    insertar_ultimo(&lista_bloqueados_terminal, p_proc_actual);  //  metemos
    p_proc_actual = planificador();  //  elijo el siguiente
    cambio_contexto(&(p_proc_anterior->contexto_regs),
		    &(p_proc_actual->contexto_regs));  //  cambio a listo
    fijar_nivel_int(nivel_anterior);
  }
  caracter_a_devolver = buffer_chars.chars[buffer_chars.indice];
  /*
  if (buffer_chars.siguiente_libre == buffer_chars.indice) {
    buffer_chars.siguiente_libre = buffer_chars.indice;
  }
  */
  buffer_chars.indice = buffer_chars.indice + 1;
  if (buffer_chars.indice == TAM_BUF_TERM)
    buffer_chars.indice = 0;

  //  printk("buffer_chars.indice = %d\n", buffer_chars.indice);

  return ((int) caracter_a_devolver);  //  sera un casting
}


/*
 *
 * Rutina de inicialización invocada en arranque
 *
 */
int main(){
    //Yo
    short int i = 0;
	/* se llega con las interrupciones prohibidas */
	iniciar_tabla_proc();

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */


	// Yo
	for (i=0;i<NUM_MUT;i++)
	    lista_mutex[i].tipo = -1;

	buffer_chars.indice = 0;

	memset (buffer_chars.chars, 0, TAM_BUF_TERM*sizeof(char));
	/*
	for (i=0;i<TAM_BUF_TERM;i++)
	    buffer_chars.chars[i] = (char) -1;
	*/
	buffer_chars.siguiente_libre = 0;


	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
