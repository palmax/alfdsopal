
/* Programa que fuerza un error intentando escribir en una dirección de
sólo lectura. El monitor debe detectarlo y teminar la ejecución del
programa */

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

void f(){
        return;
}

int main(int argc, char **argv)
{
        void **p;

	p=(void *)&f;
	*p=(void *)7; /* error escribiendo en el código */
	printf("No debe aparecer esto\n");
        return 0;
}
