all:tp3

tp3: tp3.c
	gcc -m32 -I/home/enseign/ASE/include -c tp3.c
	gcc -m32 -L/home/enseign/ASE/lib -o tp3 tp3.o -lhardware


