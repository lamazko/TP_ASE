all:tp3

tp3: tp3.c
	gcc -m32 -c tp3.c
	gcc -m32 -o tp3 tp3.o


