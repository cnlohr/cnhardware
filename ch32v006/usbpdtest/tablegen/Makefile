all : tablegen

tablegen : tablegen.c
	gcc -o $@ $^ -Wall

clean :
	rm -rf *.o *~ tablegen

