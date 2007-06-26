/*
 * usuario/bajador.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 * Programa de usuario que abre un semáforo y lo baja.
 */

#include "servicios.h"

int main(){
	int id, desc;


	id=obtener_id_pr();
	printf("bajador (%d) comienza\n", id);

	if ((desc=abrir_sem("s1"))<0)
		printf("error creando s1. NO DEBE SALIR\n");

	printf("bajador (%d) baja semáforo\n", id);

	/* Se debe bloquear */
	if (bajar(desc)<0)
		printf("error en bajar de semaforo. NO DEBE SALIR\n");

	/* desbloqueado por el subir */
	printf("bajador (%d) termina\n", id);

	/* cierre implícito de semáforos */
	return 0;
}
