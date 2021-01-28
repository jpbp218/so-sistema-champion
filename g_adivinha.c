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
    int num, resNum, i = 0, res;
    char aux[40];

    struct sigaction act;
    act.sa_handler = timeout;
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    srand((unsigned int)time(NULL));

    while (flagContinua)
    {
        do
        {
            printf("\nAdivinhe um numero entre 0 - 5 >>> ");
            fflush(stdout);
            if ((res = scanf(" %s", aux)) != 1)
            {
                break;
            }
            num = atoi(aux);
            fflush(stdin);
        } while (num < 0 || num > 5);
        resNum = rand() % 6;
        if (resNum == num)
        {
            printf("Parabens, acertou o numero!\n");
            fflush(stdout);
            count++;
        }
        else
        {
            printf("Tente de novo, o numero era %d!\n", resNum);
            fflush(stdout);
        }
    }
    exit(count);
}