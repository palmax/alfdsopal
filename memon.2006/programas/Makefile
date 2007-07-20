#
# memon/programas/Makefile
#       Makefile de los programas de prueba
#

CC=cc
CFLAGS=-Wall -g

all: prueba1 prueba2 error_acceso error_escritura

prueba1: prueba1.o
	$(CC) -shared -o $@ prueba1.o

prueba2: prueba2.o
	$(CC) -shared -o $@ prueba2.o

error_escritura: error_escritura.o
	$(CC) -shared -o $@ error_escritura.o

error_acceso: error_acceso.o
	$(CC) -shared -o $@ error_acceso.o

clean:
	rm -f *.o prueba1 prueba2 error_acceso error_escritura
