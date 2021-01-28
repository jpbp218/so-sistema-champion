#include <string.h>
#include <time.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int count = 0;
int flagContinua = 1;

void timeout(int sig)
{
    flagContinua = 0;
}

int main()
{
    int num1, num2, res, i = 0, op;
    char operacoes[3] = {'+', '-', '*'};
    char aux[20];

    struct sigaction act;
    act.sa_handler = timeout;
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    srand((unsigned int)time(NULL));

    printf("\nInsira o resultado da operação apresentada!\n\n");
    fflush(stdout);

    while (flagContinua)
    {
        num1 = rand() % 101 + 1;
        num2 = rand() % 101 + 1;
        op = rand() % 3;

        printf("\n%d %c %d = ", num1, operacoes[op], num2);
        fflush(stdout);
        if((res = scanf(" %s", aux)) != 1) {
            break;
        }
        res = atoi(aux);

        switch (op)
        {
        case 0:
            if (num1 + num2 == res)
            {
                printf("Parabens, acertou a conta!\n");
                fflush(stdout);
                count++;
            }
            else{
                printf("Nao acertou, o resultado era %d!\n", num1 + num2);
                fflush(stdout);
            } 
            break;

        case 1:
            if (num1 - num2 == res)
            {
                printf("Parabens, acertou a conta!\n");
                fflush(stdout);
                count++;
            }
            else{
                printf("Nao acertou, o resultado era %d!\n", num1 - num2);
                fflush(stdout);
            } 
            break;

        case 2:
            if (num1 * num2 == res)
            {
                printf("Parabens, acertou a conta!\n");
                fflush(stdout);
                count++;
            }
            else{
                printf("Nao acertou, o resultado era %d!\n", num1 * num2);
                fflush(stdout);
            } 
            break;
        }
    }
    exit(count);
}