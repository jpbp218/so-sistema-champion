#include <string.h>
#include <time.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TAM_MAX_PALAVRAS 20
#define NUM_PALAVRAS 26

int count = 0;
int flagContinua = 1;

void timeout(int sig)
{
    flagContinua = 0;
}

int main()
{
    char palavra[NUM_PALAVRAS][TAM_MAX_PALAVRAS]={"abacate","abacaxi","azeitona","ameixa","amora","banana","cacau","cereja",
    "caju","coco","figo","framboesa","groselha","kiwi","laranja","lima","manga","marmelo","melancia","mirtilo","morango",
    "nectarina","tangerina","tomate","toranja","uva"};

    char palavraEsc[TAM_MAX_PALAVRAS];
    char palavraAux[TAM_MAX_PALAVRAS];
    char resposta[30];
    int aux,res, escolhidos[10], flag;
    const char * p;

    struct sigaction act;
    act.sa_handler = timeout;
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    srand((unsigned int)time(NULL));

    printf("\nAdivinhe qual e a fruta!\n\n");
    fflush(stdout);
    
    for (int i = 0; flagContinua; i++)
    {
        do
        {
            flag = 0;
            aux = rand() % NUM_PALAVRAS;
            for (int j = 0; j < i; j++)
            {
                if (escolhidos[j] == aux) {
                    flag = 1;
                    break;
                }
            }
        } while (flag);
        escolhidos[i] = aux;
        p = palavra[aux];
        strcpy(palavraEsc,p);
        strcpy(palavraAux,p);

        aux = rand() % (strlen(p)-3) + 1;
        if (aux < 1)
            aux = 1;
        for (int k = 0; k < aux; k++)
            palavraAux[rand() % strlen(p)] = '*';

        printf("Qual e esta fruta? %s\n>>> ", palavraAux);
        fflush(stdout);
        if((res = scanf(" %s", resposta)) != 1) {
            break;
        }
        
        if (strcmp(resposta, palavraEsc) == 0)
        {
            printf("Parabens, acertou a palavra!\n\n");
            fflush(stdout);
            ++count;
        }
        else {
            printf("A resposta correta era %s!\n\n", palavraEsc);
            fflush(stdout);
        }    
    }
    exit(count);
}