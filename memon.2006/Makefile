#
# memon/Makefile
#       Makefile general de la práctica
#

CC=cc
CFLAGS=-Wall -g

all: pruebas memon

fallo.o mapa.o marcos.o: mapa.h

memon: memon.o apoyo.o mapa.o fallo.o marcos.o
	$(CC) -rdynamic -o $@ memon.o apoyo.o mapa.o fallo.o marcos.o -ldl

pruebas:
	cd programas; make

clean:
	rm -f memon.o mapa.o marcos.o fallo.o memon
	cd programas; make clean
