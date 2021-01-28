#include "estruturas.h"

int flagContinua = 1;

void comecaCamp(int s)
{
    printf("O campeonato comecou!\n");
    flagContinua = 1;
}

void encerrar(int s, siginfo_t *info, void *context)
{
    switch (info->si_value.sival_int)
    {
    case 1: // Encerrar devido a nome não disponível (repetido)
        printf("\nO nome introduzido nao esta disponivel!\n");
        flagContinua = 0;
        break;

    case 2: // Encerrar devido a kick pelo servidor
        printf("\nFoi kickado pelo servidor!\n");
        flagContinua = 0;
        break;

    case 3: // Servidor desligado
        printf("\nO servidor foi desligado!\n");
        alarm(0.0000000001); // Sinal sem nenhuma ação, apenas para ultrapassar o pause()
        flagContinua = 0;
        break;

    case 4: // Cliente saiu do jogo
        printf("\nSaiu do jogo!\n");
        flagContinua = 0;
        break;

    case 5: // Servidor recomeça o cameponato
        printf("\n\n\nO campeonato terminou!\n");
        printf("A aguardar que o campeonato comece...\n");
        flagContinua = -1;
        break;

    case 6: // Encerrar devido a numero maximo de jogadores atingido
        printf("\nO numero maximo de jogadores foi atingido!\n");
        flagContinua = 0;
        break;

    default:
        break;
    }
}

int main()
{
    CLIENTE c;
    char fifo[40];
    int algo, fd, fdr, fdServ, num, res;
    char cmd[20], fifoServ[20];
    fd_set fds;

    // Tenta aceder ao fifo do servidor
    if (access(FIFO_SRV, F_OK) != 0)
    {
        fprintf(stderr, "[ERRO] O Servidor não está a funcionar\n");
        exit(ERRO);
    }

    // Prepara-se para receber sinais do Servidor
    //SIGUSR1
    signal(SIGUSR1, comecaCamp);
    signal(SIGALRM, NULL);
    //SIGUSR2
    struct sigaction act;
    act.sa_sigaction = encerrar;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &act, NULL);

    // Obtem dados do jogador
    c.pid = getpid();
    printf("Bem-vindo Jogador!\n\nInserir o seu nome: ");
    scanf("%[^\n]", c.nome);

    // Abre o fifo do servidor
    if ((fdServ = open(FIFO_SRV, O_WRONLY)) == -1)
    {
        printf("O servidor ja nao esta disponivel\n");
        exit(ERRO);
    }

    // Cria a os seus fifos de comunicação
    sprintf(fifoServ, "serv%d", getpid());
    mkfifo(fifoServ, 0600);
    fd = open(fifoServ, O_RDWR);

    sprintf(fifo, FIFO_CLI, c.pid);
    mkfifo(fifo, 0600);
    fdr = open(fifo, O_RDWR);

    // Envia pedido para se resgistar ao servidor
    write(fdServ, &c, sizeof(CLIENTE));
    close(fdServ);

    // Aguarda pelo início do campeonato
    if (flagContinua != 0)
    {
        printf("A aguardar que o campeonato comece...\n");
        pause();
    }
    

    // Começou o campeonato --> O Cliente vai jogar
    while (flagContinua != 0)
    {
        if (flagContinua == -1)
            pause();
        //printf("Comando: ");
        //fflush(stdout);
        FD_ZERO(&fds);
        FD_SET(0, &fds);  /* TECLADO */
        FD_SET(fdr, &fds); /* NPIPE */

        res = select(fdr + 1, &fds, NULL, NULL, NULL);

        if (res > 0 && FD_ISSET(0, &fds))
        { /*Input do cliente */
            scanf("%s", cmd);
            for (int i = 0; cmd[i] != '\0' && cmd[0] != 'k'; i++)
                cmd[i] = tolower(cmd[i]);

            if (cmd[0] == '#')
            {
                if (strcmp(cmd, "#quit") == 0)
                {
                    strcpy(c.cmd, cmd);
                    if ((fdServ = open(FIFO_SRV, O_WRONLY)) == -1)
                    {
                        printf("O servidor ja nao esta disponivel\n");
                        flagContinua = 0;
                        break;
                    }
                    write(fdServ, &c, sizeof(CLIENTE));
                    close(fdServ);
                }
                else if (strcmp(cmd, "#mygame") == 0)
                {
                    int n;
                    strcpy(c.cmd, cmd);
                    if ((fdServ = open(FIFO_SRV, O_WRONLY)) == -1)
                    {
                        printf("O servidor ja nao esta disponivel\n");
                        flagContinua = 0;
                        break;
                    }
                    write(fdServ, &c, sizeof(CLIENTE));
                    close(fdServ);
                    //fdr = open(fifo, O_RDONLY);           // Abrir o fifo do cliente para leitura
                    n = read(fdr, &c, sizeof(CLIENTE));     // Le o servidor
                    //close(fdr);                           // Fecha o fifo do cliente
                    if (n == sizeof(CLIENTE))
                        printf("O jogo que lhe foi atribuido foi %s!\n", c.jogo);
                }
                else
                {
                    printf("[AVISO] COMANDO INEXISTENTE [AVISO]\n");
                    printf("Lista: '#mygame', '#quit'\n");
                }
            }
            else
            {
                strcat(cmd, "\n");
                write(fd, cmd, strlen(cmd));
                memset(cmd, 0, sizeof(cmd));
            }
            
        }
        else if (res > 0 && FD_ISSET(fdr, &fds))
        { /* ENTRA O CÓDIGO PARA QUANDO RECEBE O INPUT DO UTILIZADOR */
            char buffer[100];
            int n;
            if (n = read(fdr, buffer, 100) > 0){
                printf("%s", buffer);
                fflush(stdout);
                memset(buffer, 0, sizeof(buffer));
            }
        }
    }
    close(fd);
    close(fdr);
    unlink(fifoServ);
    unlink(fifo);
    exit(OK);
}