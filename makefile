
all: cliente.o arbitro.o g_rps.o
	gcc cliente.o -o cliente
	gcc arbitro.o -o arbitro -lpthread
	gcc g_rps.o -o g_rps

debug: cliente.c arbitro.c g_rps.c
	gcc cliente.c -o cliente -g -DDEBUG
	gcc arbitro.c -o arbitro -g -DDEBUG -lpthread
	gcc g_rps.c -o g_rps -g -DDEBUG

cliente: cliente.c cliente.h
	gcc cliente.c -c

arbitro: arbitro.c arbitro.h
	gcc arbitro.c -c 
g_rps: g_rps.c g_rps.h
	gcc g_rps.c -c

clean:
	rm *.o
	rm cliente
	rm arbitro
	rm g_rps
