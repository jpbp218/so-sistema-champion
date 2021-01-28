# ========================================================
# ================= MAKEFILE TP SO 20/21 =================
# ========================================================

# =================== Regra Principal ====================

all: jogo arbitro cliente

# ====================== Compilação ======================

#Criação código objeto do jogo adivinha
objects/g_adivinha.o : ./g_adivinha.c ./estruturas.h
	gcc -c ./g_adivinha.c -o ./objects/g_adivinha.o

#Criação código objeto do jogo Adivinha Palavra
objects/g_adivinhaPalavra.o : ./g_adivinhaPalavra.c ./estruturas.h
	gcc -c ./g_adivinhaPalavra.c -o ./objects/g_adivinhaPalavra.o

#Criação código objeto do jogo Conta
objects/g_conta.o : ./g_conta.c ./estruturas.h
	gcc -c ./g_conta.c -o ./objects/g_conta.o

#Criação código objeto do árbitro
objects/arbitro.o : ./arbitro.c ./estruturas.h
	gcc -c ./arbitro.c -o ./objects/arbitro.o

#Criação do código objeto do cliente
objects/cliente.o : ./cliente.c ./estruturas.h
	gcc -c ./cliente.c -o ./objects/cliente.o

Games/g_adivinha: ./objects/g_adivinha.o
	gcc -o Games/g_adivinha ./objects/g_adivinha.o

Games/g_adivinhaPalavra: ./objects/g_adivinhaPalavra.o
	gcc -o Games/g_adivinhaPalavra ./objects/g_adivinhaPalavra.o

Games/g_conta: ./objects/g_conta.o
	gcc -o Games/g_conta ./objects/g_conta.o

jogo: Games/g_adivinha Games/g_adivinhaPalavra Games/g_conta

arbitro: ./objects/arbitro.o
	gcc -o arbitro ./objects/arbitro.o -pthread

cliente: ./objects/cliente.o
	gcc -o cliente ./objects/cliente.o

clean:
	find . -name '*.o' -delete
	find . -name 'serv*' -delete
	find . -type f -executable -delete