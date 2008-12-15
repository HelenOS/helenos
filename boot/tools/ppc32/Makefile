debug: debug.o font-8x16.o
	gcc -o debug debug.o font-8x16.o

debug.o: debug.c font-8x16.h
	gcc -c -o debug.o debug.c

font-8x16.o: font-8x16.c font-8x16.h
	gcc -c -o font-8x16.o font-8x16.c
