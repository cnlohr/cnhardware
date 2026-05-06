all : tablegen crcfinder

tablegen : tablegen.c
	gcc -o $@ $^ -Wall

crcfinder : crcfinder.c
	gcc -o $@ $^ -Wall  -Wno-unused -fsanitize=undefined,address

clean :
	rm -rf *.o *~ tablegen crcfinder

