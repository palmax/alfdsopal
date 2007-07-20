
/* Programa que fuerza un error intentando escribir en una dirección
fuera del mapa. Para ello realiza una proyección, la elimina y luego accede
a la dirección. El monitor debe detectarlo y teminar la ejecución del
programa */

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int  argc, char **argv) {
	int fd, i;
	char *org;

	/* Abre el archivo /dev/zero para lectura */
	if ((fd=open("/dev/zero", O_RDONLY))<0) {
		perror("No puede abrirse el archivo");
		return(1);
	}

	/* Se proyecta el archivo */
	if ((org=mmap((caddr_t) 0, 8192, PROT_READ,
			MAP_PRIVATE, fd, 0)) == (void *)MAP_FAILED) {
		perror("Error en la proyeccion del archivo");
		close(fd);
		return(1);
	}

	/* Se cierra el archivo */
	close(fd);

	/* Se elimina la proyeccion */
	munmap(org, 8192);
	i=*org; /* error leyendo fuera del mapa */
        printf("No debe aparecer esto\n");
        return 0;
}

