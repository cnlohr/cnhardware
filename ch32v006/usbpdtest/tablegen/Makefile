all : tablegen crcfinder

tablegen : tablegen.c
	gcc -o $@ $^ -Wall

crcfinder : crcfinder.c
	gcc -o $@ $^ -Wall -fsanitize=undefined,address

clean :
	rm -rf *.o *~ tablegen crcfinder

