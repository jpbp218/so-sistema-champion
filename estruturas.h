#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>

#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

#define TAM 30                  // Tamanho para o array de caracteres
#define ERRO 7                  // Código de erro
#define OK 3                    // Código de sucesso
#define NJOGOS 3                // Número máximo de jogos disponíveis

#define FIFO_SRV "serv"
#define FIFO_CLI "cli%d"


typedef struct {
    char nome[TAM];
} JOGO;

typedef struct {
    char nome[TAM];		        // Como o nome é único podemos usar-lo para identificar o cliente
    char cmd[TAM];              // Guarda o último comando feito pelo cliente
    char jogo[TAM];             // Guarda o nome do jogo que foi atribuido aleatóriamente ao cliente
    int pontos;                 // Guarda pontos do cliente
    int pid;                    // PID do cliente
}CLIENTE;

typedef struct {
    int * numJogadores;
    CLIENTE * c;
} CAMPEONATO;

typedef struct{
    char instruc[TAM];          // Para guardar a mensagem enviada pelo cliente
    int duracaoCamp;
    int tempoEspera;  
}SERVIDOR;

typedef struct{
    char continua;
    char continuaJogo;
    char suspende;
    CLIENTE * clientes; 
    CLIENTE * c; 
    int * numCli; 
    const char * diretorio;
    int pidFilho;
} TDATA;

typedef struct{
    char continua;
    int * numCli;
    int tempEspera;
    int duracao;
    CLIENTE * clientes;
    pthread_t * tarefa;
    TDATA * tinfo;
    JOGO * jogos;
    int numJ;
} TCOUNT;

#endif /* estruturas_H */