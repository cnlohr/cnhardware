all : serpentinegen

#CFLAGS:=-fsanitize=address -O0
CFLAGS:=-O3

serpentinegen : serpentinegen.c
	gcc $(CFLAGS) -g -o $@ $^ -lpthread -lX11 -lGL -lm

serpentinegen.exe : serpentinegen.c
	gcc $(CFLAGS) -mavx512f -DUSE_AVX512 -g -o $@ $^ -luser32 -lopengl32 -lgdi32


clean :
	rm -rf serpentinegen

