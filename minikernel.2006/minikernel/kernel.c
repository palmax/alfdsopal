/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

//  Algunas cosas:
//  Buscar lista_listos (entre otras cosas) y poner NIVEL_3 cuando se toque
//  He puesto lo de los ticks a tope despues del RR. Hay que poner el nivel de
//  la cosa que estas tratando. Cuando termine de merger, entregar y avisar a
//  Costoya (si puede nos echa un cable).
//  Hace falta revisar los indices de los bucles se pongan a 0 cuando vuelven
//  a girar


/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"     /* Contiene defs. usadas por este modulo */

#include "string.h"
/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *      iniciar_tabla_proc buscar_BCP_libre
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
 *      insertar_ultimo eliminar_primero eliminar_elem
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
 *      espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
  int nivel;
    
  //      printk("-> NO HAY LISTOS. ESPERA INT\n");
    
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
    espera_int();           /* No hay nada que hacer */
  lista_listos.primero->estado = EJECUCION;
  lista_listos.primero->cuanto_queda_rodaja = TICKS_POR_RODAJA;
  //fijar_nivel_int(lista_listos.primero->nivel);
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
  short int nivel_anterior;
  short int i = 0;

  for (i=0;i<NUM_MUT_PROC;i++){
    if ((p_proc_actual->mutex_consultados[i] >= 0) &&
	(p_proc_actual->mutex_consultados[i] < NUM_MUT)) {
      printk ("p_proc_actual->mutex_consultados[%d]   == %d\n",
	      i, p_proc_actual->mutex_consultados[i]);
      escribir_registro(1,(int) i);
      sis_cerrar_mutex();
    }
  }

  printk("Apunto de liberar la imagen\n");

  liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

  printk("He liberado la imagen\n");

  p_proc_actual->estado=TERMINADO;

  nivel_anterior = fijar_nivel_int(NIVEL_3);
  eliminar_primero(&lista_listos); /* proc. fuera de listos */
    

  /* Realizar cambio de contexto */
  p_proc_anterior=p_proc_actual;
  p_proc_actual=planificador();

  printk("2\n");
  printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
	 p_proc_anterior->id, p_proc_actual->id);
    
  liberar_pila(p_proc_anterior->pila);
  printk("a punto de hacer el cambio de contexto\n");
  printk("p_proc_actual == %d\n", p_proc_actual);
  printk("p_proc_actual->contexto_regs == %d\n", p_proc_actual->contexto_regs);
  printk("&(p_proc_actual->contexto_regs) == %d\n",
	 &(p_proc_actual->contexto_regs));
    
  cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
  fijar_nivel_int(nivel_anterior);
  printk("por aki no!\n");

  return; /* no debería llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *      excepciones: exc_arit exc_mem
 *      interrupciones de reloj: int_reloj
 *      interrupciones del terminal: int_terminal
 *      llamadas al sistemas: llam_sis
 *      interrupciones SW: int_sw
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
    if (!accediendo_parametro) {
      printk ("p_proc_actual                      == %d\n",
	      p_proc_actual);
      printk ("p_proc_actual->id                  == %d\n",
	      p_proc_actual->id);
      printk ("p_proc_actual->estado              == %d\n",
	      p_proc_actual->estado);
      printk ("p_proc_actual->contexto_regs       == %d\n",
	      p_proc_actual->contexto_regs);
      printk ("p_proc_actual->pila                == %d\n",
	      p_proc_actual->pila);
      printk ("p_proc_actual->siguiente           == %d\n",
	      p_proc_actual->siguiente);
      printk ("p_proc_actual->info_mem            == %d\n",
	      p_proc_actual->info_mem);
      printk ("p_proc_actual->jiffies             == %d\n",
	      p_proc_actual->jiffies);
      printk ("p_proc_actual->tiempos             == %d\n",
	      p_proc_actual->tiempos);
      printk ("p_proc_actual->nivel               == %d\n",
	      p_proc_actual->nivel);
      printk ("p_proc_actual->cuanto_queda_rodaja == %d\n",
	      p_proc_actual->cuanto_queda_rodaja);
      printk ("p_proc_actual->siguiente           == %d\n",
	      p_proc_actual->siguiente);
      printk ("p_proc_actual->mutex_consultados   == %d\n",
	      p_proc_actual->mutex_consultados);
            
      printk ("lista_listos.primero               == %d\n",
	      lista_listos.primero);
      if (lista_listos.primero)
	printk ("lista_listos.primero->id           == %d\n",
		lista_listos.primero->id);
      printk ("lista_listos.ultimo                == %d\n",
	      lista_listos.ultimo);
      if (lista_listos.ultimo)
	printk ("lista_listos.ultimo->id            == %d\n",
		lista_listos.ultimo->id);
              
            
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
  char      caracter;
  BCP      *p_proc_anterior;
  int       nivel_anterior;
  short int i = buffer_chars.indice;
  short int hay_hueco = 0;
    
  caracter = leer_puerto(DIR_TERMINAL);
  printk("-> TRATANDO INT. DE TERMINAL %c\n", caracter);
  
  nivel_anterior = fijar_nivel_int(NIVEL_2);

  while ((i < TAM_BUF_TERM) && (!hay_hueco)) {
    if (buffer_chars.chars[i] == 0)
      hay_hueco = 1;
    else {
      i++;
      //  printk("El nuevo i es == %d\n",i);
    }
  }
      
  if (!hay_hueco) {
    i = 0;
    while ((i < buffer_chars.indice) && (!hay_hueco)) {
      if (buffer_chars.chars[i] == 0)
	hay_hueco = 1;
      else {
	i++;
	//  printk("El nuevo i es == %d\n",i);
      }
    }
  }
      
  //  printk("Salimos con i == %d\n",i);
      
  if (!hay_hueco) {
    fijar_nivel_int(nivel_anterior);
    return;  //  Si no hay hueco se descarta el char como dice el enunciado
  } else {
    buffer_chars.chars[i] = caracter;
      
    //  printk("metiendo buffer_chars.chars[%d]=%c\n", i, caracter);    

    //  despierto a los que esten dormidos esperando un caracter
    if (lista_bloqueados_terminal.primero) {
      fijar_nivel_int(NIVEL_3);  // CASO ESPECIAL!!
      p_proc_anterior                    = p_proc_actual;
      p_proc_actual                      = lista_bloqueados_terminal.primero;
      p_proc_actual->nivel               = nivel_anterior;
      //p_proc_actual->cuanto_queda_rodaja = TICKS_POR_RODAJA;
      p_proc_actual->estado              = LISTO;
      eliminar_elem(&lista_bloqueados_terminal, p_proc_actual);
      insertar_ultimo(&lista_listos, p_proc_actual);  //  metemos
      p_proc_actual                      = planificador();
      cambio_contexto(&(p_proc_anterior->contexto_regs),
		      &(p_proc_actual->contexto_regs));  //  cambio a listo
      printk("-> %d ha sido despertado de esperar un char gracias a %c\n",
	     p_proc_actual->id, caracter);
    }
    fijar_nivel_int(nivel_anterior);
    return;
  }
}

/*
 * Tratamiento de interrupciones software
 */
static void int_sw(){  //  Falta lo de las llamadas al sistema
  BCP * p_proc_anterior = p_proc_actual;
  int nivel_anterior;  
  printk("-> TRATANDO INT. SW\n");
  printk("p_proc_actual->cuanto_queda_rodaja == %d\n",
	 p_proc_actual->cuanto_queda_rodaja);
  printk("p_proc_actual->siguiente == %d\n",
	 p_proc_actual->siguiente);
  //  Yo
  //  Suponemos que solo hay un tipo de int_sw
  //  Tenemos que coger el p_proc_actual y hacer un change
  //  con el siguiente y luego ponerlo en la cola de listos
  if (p_proc_actual && p_proc_actual->siguiente &&
      (p_proc_actual->estado == EJECUCION) &&
      (p_proc_actual->cuanto_queda_rodaja <= 0)) {
    printk("-> usamos round robin\n");
    nivel_anterior = fijar_nivel_int(NIVEL_3);  // tocamos la lista_listos
    p_proc_anterior->estado = LISTO;
    p_proc_actual->estado   = EJECUCION;
        
    eliminar_elem(&lista_listos, p_proc_anterior);
    insertar_ultimo(&lista_listos, p_proc_anterior);  //  metemos al final
        
    printk("p_proc_anterior      == %d\n", p_proc_anterior);
    printk("p_proc_actual        == %d\n", p_proc_actual);
    printk("lista_listos.primero == %d\n", lista_listos.primero);
    printk("lista_listos.ultimo  == %d\n", lista_listos.ultimo);
        
    p_proc_actual=planificador();
    cambio_contexto(&(p_proc_anterior->contexto_regs),
		    &(p_proc_actual->contexto_regs));  //  cambio a listo

    fijar_nivel_int(nivel_anterior);        
    printk("-> hecho RR\n");
  }
    
  //  Tanto si hay mas como si no le doy ticks
  //p_proc_anterior->cuanto_queda_rodaja = TICKS_POR_RODAJA;

  /*    
  printk ("p_proc_anterior                      == %d\n",
	  p_proc_anterior);
  printk ("p_proc_anterior->id                  == %d\n",
	  p_proc_anterior->id);
  printk ("p_proc_anterior->estado              == %d\n",
	  p_proc_anterior->estado);
  printk ("p_proc_anterior->contexto_regs       == %d\n",
	  p_proc_anterior->contexto_regs);
  printk ("p_proc_anterior->pila                == %d\n",
	  p_proc_anterior->pila);
  printk ("p_proc_anterior->siguiente           == %d\n",
	  p_proc_anterior->siguiente);
  printk ("p_proc_anterior->info_mem            == %d\n",
	  p_proc_anterior->info_mem);
  printk ("p_proc_anterior->jiffies             == %d\n",
	  p_proc_anterior->jiffies);
  printk ("p_proc_anterior->tiempos             == %d\n",
	  p_proc_anterior->tiempos);
  printk ("p_proc_anterior->nivel               == %d\n",
	  p_proc_anterior->nivel);
  printk ("p_proc_anterior->cuanto_queda_rodaja == %d\n",
	  p_proc_anterior->cuanto_queda_rodaja);
  printk ("p_proc_anterior->siguiente           == %d\n",
	  p_proc_anterior->siguiente);
  printk ("p_proc_anterior->mutex_consultados   == %d\n",
	  p_proc_anterior->mutex_consultados);
    
  printk ("p_proc_actual                        == %d\n",
	  p_proc_actual);
  printk ("p_proc_actual->id                    == %d\n",
	  p_proc_actual->id);
  printk ("p_proc_actual->estado                == %d\n",
	  p_proc_actual->estado);
  printk ("p_proc_actual->contexto_regs         == %d\n",
	  p_proc_actual->contexto_regs);
  printk ("p_proc_actual->pila                  == %d\n",
	  p_proc_actual->pila);
  printk ("p_proc_actual->siguiente             == %d\n",
	  p_proc_actual->siguiente);
  printk ("p_proc_actual->info_mem              == %d\n",
	  p_proc_actual->info_mem);
  printk ("p_proc_actual->jiffies               == %d\n",
	  p_proc_actual->jiffies);
  printk ("p_proc_actual->tiempos               == %d\n",
	  p_proc_actual->tiempos);
  printk ("p_proc_actual->nivel                 == %d\n",
	  p_proc_actual->nivel);
  printk ("p_proc_actual->cuanto_queda_rodaja   == %d\n",
	  p_proc_actual->cuanto_queda_rodaja);
  printk ("p_proc_actual->siguiente             == %d\n",
	  p_proc_actual->siguiente);
  printk ("p_proc_actual->mutex_consultados     == %d\n",
	  p_proc_actual->mutex_consultados);

  printk ("viene_de_modo_usuario                == %d\n",
	  viene_de_modo_usuario());
  printk ("accediendo_parametro                 == %d\n",
	  accediendo_parametro);
  */

  return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){  //  paux_ant no servia para nada
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
      paux->estado= LISTO;
      printk("-> %d listo\n",paux->id);
      eliminar_elem(&lista_dormidos, paux);
      insertar_ultimo(&lista_listos, paux);
      //  hay que planificar?
      paux = paux_sig;
      if (paux_sig) {
	printk("-> %d siguiendo a %d\n",paux->id, paux->siguiente);
	paux_sig = paux_sig->siguiente;
    }
    } else {
      paux = paux_sig;
      if (paux_sig)
	paux_sig = paux_sig->siguiente;
    }
  }

  if (p_proc_actual && p_proc_actual->estado == EJECUCION) {
    p_proc_actual->cuanto_queda_rodaja =
      p_proc_actual->cuanto_queda_rodaja - 1;
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
    res=-1;         /* servicio no existente */
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
    return -1;      /* no hay entrada libre */

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
	//                  p_proc->mutex_abiertos[i] = -1;
      }
      //              p_proc->indice_mutex_consultados = 0;  //  usar %
      //              p_proc->indice_mutex_abiertos = 0;  //  usar %
      //p_proc->cuanto_queda_rodaja   = TICKS_POR_RODAJA;
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
 *      sis_crear_proceso sis_escribir
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

  printk("el proceso %d quiere terminar\n", p_proc_actual->id);

  liberar_proceso();

  printk("-> FIN PROCESO %d\n", p_proc_actual->id);

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
  short int encontrado = 0;
  BCP *p_proc_anterior = p_proc_actual;
  short int nivel_anterior = 0;

  printk("-> EL PROCESO %d  QUIERE CREAR UN MUTEX\n", p_proc_actual->id);

  printk("El nombre del mutex que se quiere crear sera %s\n",nombre);
  while ((!hay_hueco) && (!encontrado)) {
    i=0;
    while ((i != NUM_MUT) && (!encontrado)) {
      if (strncmp(lista_mutex[i].nombre,nombre, MAX_NOM_MUT) == 0) {
	encontrado = 1;
	printk("Te repites mas que el ajo: lista_mutex[%d].nombre == %s\n",
	       i, lista_mutex[i].nombre);
	return -1;  // innecesario
      }
      else
	i++;
    }
        
    i = 0;
    while ((i != NUM_MUT) && (!hay_hueco) && (!encontrado)) {
      if (lista_mutex[i].tipo == -1)
	hay_hueco = 1;
      else
	i++;
    }
      
    if ((!hay_hueco) && (!encontrado)){
      // a reencolar y a esperar a que un majete nos pase un sitio bloquear
      printk("Y yo sigo aki esperandote\n");
      nivel_anterior                     = fijar_nivel_int(NIVEL_3);
      p_proc_actual->estado = BLOQUEADO;
      eliminar_elem(&lista_listos, p_proc_anterior);  //  kitar de lista_listos
      insertar_ultimo(&lista_esperando_mutex,
		      p_proc_anterior);                  //  metemos al final
      p_proc_actual = planificador();                    //  elijo el siguiente
      cambio_contexto(&(p_proc_anterior->contexto_regs),
		      &(p_proc_actual->contexto_regs));  //  cambio a uno listo
      fijar_nivel_int(nivel_anterior);
    }
  }

  printk("Se va a crear el mutex %d-esimo\n",i);
  
  hay_hueco = 0;


  j = 0;
  while ((j != NUM_MUT_PROC) && (!hay_hueco) && (!encontrado)) {
    if (p_proc_actual->mutex_consultados[j] == -1)
      hay_hueco = 1;
    else
      j++;
  }
  
  printk("j  == %d\n",j);
  
  if ((hay_hueco) && (!encontrado)) {
    p_proc_actual->mutex_consultados[j] = i;
    memset(lista_mutex[i].nombre, 0, MAX_NOM_MUT);    //  innecesario?
    lista_mutex[i].tipo = tipo;
    printk("i == %d\n",i);
    printk("nombre == %s\n",nombre);
    strncpy(lista_mutex[i].nombre,
	    nombre, MAX_NOM_MUT);
    printk("3333\n");
    lista_mutex[i].proceso_poseedor            = NULL;
    lista_mutex[i].procesos_bloqueados.primero = NULL;
    lista_mutex[i].procesos_bloqueados.ultimo  = NULL;
    lista_mutex[i].cerrojos_bloqueados         = 0;
    lista_mutex[i].cerrojos_poseedor           = 0;
    lista_mutex[i].n_veces_abierto             = 1;
    printk("Creado el mutex %d-esimo con lista_mutex[%d].nombre == %s\n",
	   j, i, lista_mutex[i].nombre);
    return j;
  } else {
    // Y si no hay hueco?
    printk("Error creando mutex, no hay hueco o nombre repetido. j == %d\n",j);
    return -2;
  }

  printk("Error en la creacion del mutex, donde vas\n");
  return -3;
}

/*
 * Tratamiento de llamada al abrir_mutex
 */
int sis_abrir_mutex(){
  char *nombre         = (char *)leer_registro(1);
  short int i          = 0;
  short int j          = 0;
  short int encontrado = 0;
  short int hay_hueco  = 0;
  
  printk("-> EL PROCESO QUIERE ABRIR UN MUTEX\n");
  printk("Queremos abrir el mutex con nombre %s\n", nombre);

  while ((i != NUM_MUT) && (!encontrado)) {
    if (strncmp (lista_mutex[i].nombre, nombre, MAX_NOM_MUT) == 0)
      encontrado = 1;
    else
      i++;
  }

  if (!encontrado) {
    printk("No encontramos ningun mutex con ese nombre\n");
    return -1;  // No existe ese mutex en el sistema
  }

  printk("Hemos encontrado el mutex en la posicion %d-esima\n",i);
  printk("En el sistema hay un mutex con el nombre pedido %s\n",
	 lista_mutex[i].nombre);

  while ((j < NUM_MUT_PROC) && (!hay_hueco)) {
    if (p_proc_actual->mutex_consultados[j] == -1)
      hay_hueco = 1;
    else
      j++;
  }
  
  if (!hay_hueco)
    return -2;  //  No hay descriptores libres

  printk("Lo consultaremos mediante la j %d-esima\n", j);
  
  lista_mutex[i].n_veces_abierto++;  //  Abierto otra
  p_proc_actual->mutex_consultados[j] = i;
  return j;
}


/*
 * Tratamiento de llamada al lock
 */
int sis_lock(){
  unsigned int mutexid     = (unsigned int)leer_registro(1);
  short int i              = 0;
  BCP *p_proc_anterior     = p_proc_actual;
  short int cogido         = 0;
  short int nivel_anterior = 0;

  printk("-> EL PROCESO QUIERE LOCKEAR UN MUTEX\n");

  if ((mutexid < 0) || (mutexid >= NUM_MUT_PROC))
    return -1;

  i =  p_proc_actual->mutex_consultados[mutexid];

  if ((i < 0) || (mutexid >= NUM_MUT))
    return -2;

  while (!cogido) {
    if ((lista_mutex[i].proceso_poseedor  == NULL) &&
	(lista_mutex[i].cerrojos_poseedor != 0)) {
      return -3;
    } else if ((lista_mutex[i].proceso_poseedor ==
		p_proc_actual) && 
	       (lista_mutex[i].tipo == NO_RECURSIVO) &&
	       (lista_mutex[i].cerrojos_poseedor != 0)) {
      return -4;
    } else if (lista_mutex[i].proceso_poseedor == NULL) {
      lista_mutex[i].proceso_poseedor = p_proc_actual;
      lista_mutex[i].cerrojos_poseedor = 1;
      cogido = 1;
    } else if ((lista_mutex[i].proceso_poseedor ==
		p_proc_actual) && 
	       (lista_mutex[i].tipo == RECURSIVO)) {
      lista_mutex[i].cerrojos_poseedor++;
      cogido = 1;
    } else {
      lista_mutex[i].cerrojos_bloqueados++;
      //bloquear
      nivel_anterior = fijar_nivel_int(NIVEL_3);
      p_proc_actual->estado = BLOQUEADO;
      eliminar_elem(&lista_listos, p_proc_actual);  //  kitamos de lista_listos
      insertar_ultimo(&lista_mutex[i].procesos_bloqueados,
		      p_proc_anterior);                  //  metemos al final
      p_proc_actual = planificador();                    //  elijo el siguiente
      cambio_contexto(&(p_proc_anterior->contexto_regs),
		      &(p_proc_actual->contexto_regs));  //  cambio a uno listo
      fijar_nivel_int(nivel_anterior);
      // cogido = 0; // innecesario
    }
  }
  printk("Cogido %d deberia ser igual que  i %d restando a i uno",cogido,i);
  return 0;
}

/*
 * Tratamiento de llamada al unlock
 */
int sis_unlock() {
  unsigned int mutexid     = (unsigned int)leer_registro(1);
  short int i              = 0;
  BCP *paux                = NULL;
  short int nivel_anterior = 0;
  
  printk("-> EL PROCESO QUIERE UNLOCKEAR UN MUTEX\n");
  
  if ((mutexid < 0) || (mutexid >= NUM_MUT_PROC))
    return -1;

  i =  p_proc_actual->mutex_consultados[mutexid];

  if ((i < 0) || (i >= NUM_MUT))
    return -2;
  
  if (lista_mutex[i].proceso_poseedor == NULL) {
    return -3;
  } else if (lista_mutex[i].proceso_poseedor !=
             p_proc_actual)
    return -4;
  else {
    if (((lista_mutex[i].tipo == RECURSIVO) &&
         (lista_mutex[i].cerrojos_poseedor >= 1)) ||
        ((lista_mutex[i].tipo == NO_RECURSIVO) &&
         (lista_mutex[i].cerrojos_poseedor == 1))) {
      lista_mutex[i].cerrojos_poseedor --;
      if (lista_mutex[i].cerrojos_poseedor == 0) {  //  CC muy raro !!!!!
        //  el poseedor hay que desbloquearlo y que lo pille otro si existe
        lista_mutex[i].proceso_poseedor = NULL;
        if (lista_mutex[i].cerrojos_bloqueados > 0) {
          nivel_anterior           = fijar_nivel_int(NIVEL_3);
          paux = 
            lista_mutex[i].procesos_bloqueados.primero;
	  paux->estado = LISTO;
	  //paux->cuanto_queda_rodaja = TICKS_POR_RODAJA;
          eliminar_elem(&lista_mutex[i].procesos_bloqueados,
                        paux);  //  quitamos de lista mutex bloqueados
          insertar_ultimo(&lista_listos,
                          paux);  //  metemos al final
          lista_mutex[i].cerrojos_bloqueados --;
          fijar_nivel_int(nivel_anterior);
        } else if (lista_mutex[i].cerrojos_bloqueados < 0) {
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


/*
 * Tratamiento de llamada al cerrar_mutex
 */
int sis_cerrar_mutex(){
  int mutexid              = (unsigned int)leer_registro(1);
  short int i              = 0;
  BCP *p_proc_aux          = NULL;
  short int nivel_anterior = 0;
    
  printk("-> EL PROCESO QUIERE CERRAR UN MUTEX\n");
  printk("mutexid == %d\n", mutexid);    
    
  if ((mutexid < 0) ||
      (mutexid >= NUM_MUT_PROC) ||
      (p_proc_actual->mutex_consultados[mutexid] == -1))
    return -1;
    
  i = p_proc_actual->mutex_consultados[mutexid];
    
    
  if ((i < 0) ||
      (i >= NUM_MUT) ||
      (lista_mutex[i].tipo == -1))
    return -2;
    
    
  printk("i  == %d\n", i);
  printk("lista_mutex[%d].tipo                        == %d\n",
	 i, lista_mutex[i].tipo);
  printk("lista_mutex[%d].nombre                      == %s\n",
	 i, lista_mutex[i].nombre);
  printk("lista_mutex[%d].proceso_poseedor            == %d\n",
	 i, lista_mutex[i].proceso_poseedor);
  printk("lista_mutex[%d].procesos_bloqueados.primero == %d\n", 
	 i, lista_mutex[i].procesos_bloqueados.primero);
  printk("lista_mutex[%d].procesos_bloqueados.ultimo  == %d\n",
	 i, lista_mutex[i].procesos_bloqueados.ultimo);
  printk("lista_mutex[%d].cerrojos_bloqueados         == %d\n",
	 i, lista_mutex[i].cerrojos_bloqueados);
  printk("lista_mutex[%d].cerrojos_poseedor           == %d\n",          
	 i, lista_mutex[i].cerrojos_poseedor);
  printk("lista_mutex[%d].n_veces_abierto             == %d\n",             
	 i, lista_mutex[i].n_veces_abierto);
    
  if ((lista_mutex[i].n_veces_abierto == 1) &&
      ((lista_mutex[i].procesos_bloqueados.primero != NULL) ||
       (lista_mutex[i].procesos_bloqueados.ultimo  != NULL) ||
       (lista_mutex[i].cerrojos_bloqueados         != 0)) )
    return -3;
    
  if ((lista_mutex[i].cerrojos_poseedor > 1) &&
      (lista_mutex[i].tipo != RECURSIVO))
    return -4;  //  Intento cerrar uno que es recursivo y no se puede cerrar
    
  if (((lista_mutex[i].cerrojos_poseedor < 0) ||
       (lista_mutex[i].cerrojos_poseedor > 1))
      && (lista_mutex[i].tipo == NO_RECURSIVO))
    return -5;  //  Intento cerrar uno no recursivo y tiene varios cerrojos!
    
  if (lista_mutex[i].cerrojos_bloqueados < 0)
    return -6;  //  no te quiere ni nadie (nadie es el 0)
    
  if (lista_mutex[i].n_veces_abierto < 1)
    return -7;  //  donde vas alma de dios
    
    
  if (lista_mutex[i].n_veces_abierto == 1) {
    lista_mutex[i].tipo                        = -1;
    memset(lista_mutex[i].nombre, 0, MAX_NOM_MUT);
    lista_mutex[i].proceso_poseedor            = NULL;
    lista_mutex[i].procesos_bloqueados.primero = NULL;
    lista_mutex[i].procesos_bloqueados.ultimo  = NULL;
    lista_mutex[i].cerrojos_bloqueados         = 0;
    lista_mutex[i].cerrojos_poseedor           = 0;
    lista_mutex[i].n_veces_abierto             = 0;
                
    if (lista_esperando_mutex.primero) {
      printk("p_proc_actual->id == %d necesita lista_mutex[%d]\n",
	     p_proc_actual->id, i);
      p_proc_aux = lista_esperando_mutex.primero;
      nivel_anterior                     = fijar_nivel_int(NIVEL_3);
      p_proc_actual->nivel               = nivel_anterior;
      p_proc_aux->estado                 = LISTO;
      eliminar_elem(&lista_esperando_mutex,
		    p_proc_aux);  //  quitamos de lista_listos
      insertar_ultimo(&lista_listos,
		      p_proc_aux);  //  metemos
      fijar_nivel_int(nivel_anterior);
            
    }
  } else if (lista_mutex[i].n_veces_abierto > 1) {
    printk("Hay mas abiertos\n");
    if ((lista_mutex[i].proceso_poseedor == p_proc_actual) &&
        (p_proc_actual->estado == EJECUCION)) {
      lista_mutex[i].proceso_poseedor  = NULL;
      lista_mutex[i].cerrojos_poseedor = 0;
      if (lista_mutex[i].cerrojos_bloqueados > 0) {
	printk("Ahora liberaremos p_proc_actual->id == %d\n",
	       p_proc_actual->id);
	nivel_anterior                     = fijar_nivel_int(NIVEL_3);
	lista_mutex[i].cerrojos_bloqueados--;
	p_proc_aux                         = lista_bloqueados_mutex[i].primero;
	p_proc_aux->estado                 = LISTO;
	//p_proc_aux->cuanto_queda_rodaja    = TICKS_POR_RODAJA;  //  no? <- !!!!
	eliminar_elem(&lista_bloqueados_mutex[i],
		      p_proc_aux);  //  quitamos de lista bloqueados mutex
	insertar_ultimo(&lista_listos, p_proc_aux);  //  metemos
	fijar_nivel_int(nivel_anterior);
      }

    }
    lista_mutex[i].n_veces_abierto--;
  }

  p_proc_actual->mutex_consultados[mutexid] = -1;

  return 0;  //  retornamos lo que toque
}


/*
 * Tratamiento de llamada a leer_caracter
 */
int sis_leer_caracter(){
  char      caracter_a_devolver;
  BCP      *p_proc_anterior;
  int       nivel_anterior;
  short int i = buffer_chars.indice;
  short int hay_char = 0;
    
  printk("-> EL PROCESO QUIERE LEER UN CARACTER\n");
    
  /*
    leer_caracter() {
    Si (buffer vacío)
    bloquear el proceso
    extraer el carácter del buffer y devoverlo como resultado de la función
  */

  while (!hay_char) {
    i = buffer_chars.indice;
    while ((i < TAM_BUF_TERM) && (!hay_char)) {
      if (buffer_chars.chars[i] == 0)
	i++;
      else
	hay_char = 1;
    }

    if (!hay_char) {
      i = 0;
      while ((i < buffer_chars.indice) && (!hay_char)) {
	if (buffer_chars.chars[i] == 0)
	  i++;
	else {
	  hay_char =1;
	}
      }
    }

    if (!hay_char) { // bloqueo
      nivel_anterior                     = fijar_nivel_int(NIVEL_3);
      p_proc_anterior                    = p_proc_actual;
      p_proc_actual->nivel               = nivel_anterior;
      //p_proc_actual->cuanto_queda_rodaja = TICKS_POR_RODAJA;
      p_proc_actual->estado              = BLOQUEADO;
      p_proc_anterior                    = p_proc_actual;
      eliminar_elem(&lista_listos, p_proc_actual);  //  kitamos de lista_listos
      insertar_ultimo(&lista_bloqueados_terminal, p_proc_actual);  //  metemos
      p_proc_actual = planificador();  //  elijo el siguiente
      cambio_contexto(&(p_proc_anterior->contexto_regs),
		      &(p_proc_actual->contexto_regs));  //  cambio a listo
      fijar_nivel_int(nivel_anterior);
    }
  }

  i = buffer_chars.indice;


  while ((i < TAM_BUF_TERM) && (!hay_char)) {
    if (buffer_chars.chars[i] == 0)
      i++;
    else
      hay_char = 1;
  }

  if (!hay_char) {
    i = 0;
    while ((i < buffer_chars.indice) && (!hay_char)) {
      if (buffer_chars.chars[i] == 0)
	i++;
      else {
	hay_char =1;
      }
    }
  }

  if (hay_char) {
    caracter_a_devolver   = buffer_chars.chars[i];
    buffer_chars.chars[i] = 0;
    buffer_chars.indice   = (buffer_chars.indice + 1)%TAM_BUF_TERM;
    printk("devolviendo buffer_chars.chars[%d]=%c\n", i, caracter_a_devolver);
    printk("ahora buffer_chars.indice=%d\n", buffer_chars.indice);
    return ((int) caracter_a_devolver);  //  sera un casting
  } else {
    panico ("debiera haber caracteres");
    return -1;
  }
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
    
  iniciar_cont_int();             /* inicia cont. interr. */
  iniciar_cont_reloj(TICK);       /* fija frecuencia del reloj */
  iniciar_cont_teclado();         /* inici cont. teclado */


  // Yo
  for (i=0; i<NUM_MUT; i++) {
    lista_mutex[i].tipo = -1;
    memset(lista_mutex[i].nombre, 0, MAX_NOM_MUT);    //  necesario
  }

  memset (buffer_chars.chars, 0, TAM_BUF_TERM*sizeof(char));
  buffer_chars.indice = 0;

  for (i=0; i<NUM_MUT; i++) {
    lista_bloqueados_mutex[i].primero = NULL;
    lista_bloqueados_mutex[i].ultimo  = NULL;
  }
        
  /* crea proceso inicial */
  if (crear_tarea((void *)"init")<0)
    panico("no encontrado el proceso inicial");
        
  /* activa proceso inicial */
  p_proc_actual=planificador();
  cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
  panico("S.O. reactivado inesperadamente");
  return 0;
}
