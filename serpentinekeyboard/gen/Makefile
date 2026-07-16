all : serpentinegen

#CFLAGS:=-fsanitize=address -O0
CFLAGS:=-O3  -mavx2
#-DUSE_AVX512 -mavx512f

serpentinegen : serpentinegen.c
	gcc $(CFLAGS) -g -o $@ $^ -lpthread -lX11 -lGL -lm

clean :
	rm -rf serpentinegen

