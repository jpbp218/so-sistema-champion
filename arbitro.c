#include "estruturas.h"

void atribuiJogoAleatorio(CLIENTE *c, JOGO *jogos, int numJ);

int flagInicio = 0; // 0 --> Prepara-se para ativar o contador do campeonato assim que tiver 2 jogadores, 1 --> Encontra-se à espera de mais jogadores; -1 --> Já Começou o campeonato
pthread_mutex_t trinco;

void obtemParametros(int argc, char *argv[], int *tempEspera, int *duraCamp)
{
    int c;

    while ((c = getopt(argc, argv, "t:d:h")) != -1)
    {
        switch (c)
        {
        case 't':
            if (optarg != NULL)
                *tempEspera = atoi(optarg);
            break;
        case 'd':
            if (optarg != NULL)
                *duraCamp = atoi(optarg);
            break;
        case 'h':
            printf("\n==================================================================\n");
            printf("Manual:\n\t-t <numerointeiro> --> Tempo de Espera\n\t-d <numerointeiro> --> Duracao do Campeonato\n\t-h                 --> Ajuda\n");
            printf("==================================================================\n\n");
            exit(OK);
            break;
        default:
            printf("Argumentos Invalidos\n");
            break;
        }
    }

    if (argc != 5 && argc != 2)
    {
        printf("Por favor coloque argumentos validos, para mais ajuda coloque \"-h\"\n");
        exit(ERRO);
    }
}

int verificaNome(char nome[], CLIENTE c[], int numC, int *pos) /* Retoma 1 se encontrar um nome igual e 0 se não encontrar */
{
    for (int i = 0; i < numC; i++)
    {
        if (strcmp(nome, c[i].nome) == 0)
        {
            if (pos != NULL)
                *pos = i;
            return 1;
        }
    }
    return 0;
}

void comecaCampUnico(int pid)
{
    kill(pid, SIGUSR1);
}

void mostraC(CLIENTE *c, int numC)
{ // Enviar também o jogo quando for associado aos clientes
    printf("PLAYERS\n");
    if (numC > 0)
    {
        for (int i = 0; i < numC; ++i)
        { // Fazer função para obter quantidade de jogadores
            printf(" -> Player %d: %s\n", i + 1, c[i].nome);
            printf("\tJogo: %s\n", c[i].jogo);
        }
    }
    else
    {
        printf("\n[AVISO] Nao existem jogadores... [AVISO]\n");
    }
}

void encerraC(CLIENTE *c, int *numC, int pid, int valorSinal)
{ // valorSinal: 1 -> Nome repetido, 2 -> Kick, 4 -> Cliente quer desistir, 5 -> Recomeçar campeonato, 6 -> Número máximo de jogadores
    union sigval value;
    value.sival_int = valorSinal;
    sigqueue(pid, SIGUSR2, value);
}

int terminaThread(CLIENTE *c, pthread_t tarefa[], TDATA tinfo[], int numC)
{
    int pos;
    for (pos = 0; pos < numC && tinfo[pos].c != c; pos++)
        ; // Procura posição da thread dedicada ao cliente

    if (pos == numC)
        return 0;
    tinfo[pos].continua = 0;
    tinfo[pos].continuaJogo = 0;
    pthread_kill(tarefa[pos], SIGUSR1); // Apenas para saltar o read e o scanf
    pthread_join(tarefa[pos], NULL);    // espera pelo fim da thread

    for (int i = pos; i < numC - 1; i++)
    {
        tinfo[i].c = tinfo[i + 1].c;
        tarefa[i] = tarefa[i + 1];
    }

    return 1;
}

int kick(CLIENTE *c, int *numC, char nome[], pthread_t tarefa[], TDATA tinfo[], int valorSinal)
{ // valorSinal: 1 -> Nome repetido, 2 -> Kick, 4 -> Cliente quer desistir, 5 -> Recomeçar campeonato
    int pos;
    char aux[20];
    if (valorSinal == 2)
        memcpy(aux, nome + 1, strlen(nome));
    else
        strcpy(aux, nome);
    if (verificaNome(aux, c, *numC, &pos))
    {
        terminaThread(&c[pos], tarefa, tinfo, *numC);
        encerraC(c, numC, c[pos].pid, valorSinal);
        printf("\n[AVISO] O cliente com o %s saiu do jogo! [AVISO]\n", aux);
        for (int i = pos; i < *numC - 1; i++)
        {
            c[i].pid = c[i + 1].pid;
            c[i].pontos = c[i + 1].pontos;
            strcpy(c[i].nome, c[i + 1].nome);
        }
        *numC -= 1;
        if (*numC < 2)
        {
            encerraC(c, numC, c[0].pid, 5);
            flagInicio = 0;
        }
        return 1;
    }
    else
        printf("\n[AVISO] Este jogador não foi encontrado! [AVISO]\n");
    return 0;
}

void imprimeJogo(const char *dir)
{
    DIR *g;
    int i = 0;
    struct dirent *f;
    g = opendir(dir);
    if (g)
    {
        while ((f = readdir(g)) != NULL)
        {
            if (f->d_name[0] == 'g' && f->d_name[1] == '_')
                printf(" -> Jogo %d: %s\n", ++i, f->d_name);
        }
        closedir(g);
    }
}

// Encerrar todos os clientes
void encerraClientes(CLIENTE *c, int numC)
{
    union sigval value;
    value.sival_int = 3;
    for (int i = 0; i < numC; i++)
        sigqueue(c[i].pid, SIGUSR2, value);
}

// Encerrar todos os clientes
void reniciarCampeonato(CLIENTE *c, int numC, TDATA tinfo[], JOGO *jogos, int numJ, pthread_t * tarefa)
{
    int maxPontos = 0, fd, flagEmpate = 0;
    char nomeVencedor[40], fifo[20], msg[80];
    int pontosaux;

    union sigval value;
    value.sival_int = 5;

    // Termina Jogos dos clientes
    for (int i = 0; i < numC; i++){
        pthread_mutex_lock(&trinco);
        tinfo[i].continuaJogo = 0;
        tinfo[i].continua = 1;
        pthread_kill(tarefa[i], SIGUSR1);
        pthread_mutex_unlock(&trinco); // Destrancar
        waitpid(tinfo[i].pidFilho, &pontosaux, 0); // Espera pelo final do filho
        if (c[i].pontos > maxPontos)
        {
            maxPontos = c[i].pontos;
            strcpy(nomeVencedor,c[i].nome);
            flagEmpate = 0;
        }
        else if (maxPontos == c[i].pontos)
            flagEmpate = 1;
    }
    // Informa os jogadores que podem começar a jogar
    for (int i = 0; i < numC; i++)
    {
        if (flagEmpate)
            sprintf(msg, "\n\nO campeonato ficou empatado!\nObteve %d Pontos.\n\n",c[i].pontos);
        else
            sprintf(msg, "\nVencedor: %s\nObteve %d Pontos.\n\n",nomeVencedor, c[i].pontos);  
        sprintf(fifo, FIFO_CLI, c[i].pid);
        fd = open(fifo, O_WRONLY);
        write(fd, msg, strlen(msg));
        close(fd);
        sigqueue(c[i].pid, SIGUSR2, value);
    }

    if (flagEmpate)
        printf("\nCampeonato Empatado!\n");
    else
        printf("\nVencedor do Campeonato: %s\n", nomeVencedor);

    flagInicio = 1;
}

// Devolve um vetor jogo com o nome de todos os jogos disponíveis
void carregaJogos(JOGO jogos[], const char *dir, int *numJ)
{
    DIR *g;
    int i = 0;
    struct dirent *f;
    g = opendir(dir);
    if (g)
    {
        while ((f = readdir(g)) != NULL)
        {
            if (f->d_name[0] == 'g' && f->d_name[1] == '_')
            {
                strcpy(jogos[i].nome, f->d_name);
                ++i;
            }
        }
        closedir(g);
    }
    *numJ = i;
}
// teste
// Atribui um jogo ao cliente de forma aleatória
void atribuiJogoAleatorio(CLIENTE *c, JOGO *jogos, int numJ)
{ // Ptr para o jogo de um cliente, ptr para o vetor de jogos, numero de jogos
    int aux;
    aux = rand() % numJ;
    strcpy(c->jogo, jogos[aux].nome);
}

void atribuiJAClientes(CLIENTE *c, int numC, JOGO *jogos, int numJ)
{
    for (int i = 0; i < numC; i++)
        atribuiJogoAleatorio(&c[i], jogos, numJ);
}

void salta(int s) {}

void *trataContador(void *dados)
{
    TCOUNT *pdata = (TCOUNT *)dados;

    struct sigaction act;
    act.sa_handler = salta;
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    while (pdata->continua)
    {
        // ============ Começa a contar o tempo para esperar por mais clientes ==================
        if (sleep(pdata->tempEspera) > 0)
            pthread_exit(NULL);
        // ================================= Começa o campeonato ================================
        for (int i = 0; i < *pdata->numCli; i++)
        {
            kill(pdata->clientes[i].pid, SIGUSR1);   // Informa os clientes que começou o campeonato
            pthread_kill(pdata->tarefa[i], SIGUSR1); // Informa as threads de cada cliente que começou o campeonato
        }
        flagInicio = -1;
        printf("\n\n[AVISO] O campeonato comecou! [AVISO]\n\nComando: ");
        fflush(stdout);
        // ============== Começa a contar o tempo da duração máxima do campeonato ===============
        if (sleep(pdata->duracao) > 0)
            pthread_exit(NULL);
        // ======================== Informa que o campeonato acabou =============================
        printf("\n\n[AVISO] O campeonato acabou! [AVISO] \n\nComando: ");
        fflush(stdout);
        reniciarCampeonato(pdata->clientes, *pdata->numCli, pdata->tinfo,pdata->jogos, pdata->numJ, pdata->tarefa);
    }

    pthread_exit(NULL);
}

void comecaContador(pthread_t *contador, TCOUNT *cinfo)
{
    cinfo->continua = 1;
    pthread_create(contador, NULL, trataContador, (void *)cinfo);
}

void acabaContador(pthread_t *contador, TCOUNT *cinfo)
{
    cinfo->continua = 0;
    pthread_kill(*contador, SIGUSR1);
    pthread_join(*contador, NULL);
    flagInicio = 0;
}

void comunicacao(char nome[], CLIENTE *c, int numC, TDATA tinfo[], int suspende)
{ // Suspende = 1 para suspender, suspende = 0 para retomar
    int pos, fd;
    char aux[20];
    char mensagem[30];
    if (suspende)
        strcpy(mensagem, "\nO seu jogo foi suspenso!\n");
    else if (!suspende)
        strcpy(mensagem, "\nO seu jogo foi retomado!\n");
    memcpy(aux, nome + 1, strlen(nome));
    if (verificaNome(aux, c, numC, &pos))
    {
        for (int i = 0; i < numC; i++)
        {
            if (tinfo[i].c == c)
            {
                tinfo[i].suspende = suspende;
                sprintf(aux, FIFO_CLI, tinfo[i].c->pid);
                fd = open(aux, O_WRONLY);
                write(fd, mensagem, strlen(mensagem));
                close(fd);
                break;
            }
        }
    }
    else
        printf("[ERRO] Erro a suspender comunicacao de %s\n", nome);
}

void *trataCliente(void *dados)
{
    CLIENTE aux;
    TDATA *pdata = (TDATA *)dados;
    int n, fd, fdcli, pid, estado, res, pidFilho;
    char fifo[20], fifoCli[20], str[100];
    int pai2filho[2], filho2pai[2], max;
    fd_set fds;

    pdata->c->pontos = 0;

    // Tratar o sinal (sigusr1) --> sigaction para saltar o sinal
    struct sigaction act;
    act.sa_handler = salta;
    act.sa_flags = 0; //SA_RESTART
    sigaction(SIGUSR1, &act, NULL);

    sprintf(fifoCli, FIFO_CLI, pdata->c->pid);
    sprintf(fifo, "serv%d", pdata->c->pid);

    fd = open(fifo, O_RDONLY); //Abre o fifo para escrita e leitura

    while (pdata->continua)
    {
        if (flagInicio != -1)
            pause(); // Aguarda o inicio do campeonato

        // Cria pipes anónimos
        pipe(filho2pai); // Filho para Pai
        pipe(pai2filho); // Pai para Filho

        if ((pidFilho = fork()) == -1) // Criara processo filho
            printf("\n[ERRO] Erro a criar processo para o jogo [ERRO]\n\n");
        else
        {
            if (pidFilho == 0) // Filho
            {
                setbuf(stdout, 0);
                
                // Envia STDOUT para o Pai
                close(STDOUT_FILENO);
                dup(filho2pai[1]);
                close(filho2pai[1]);
                close(filho2pai[0]);

                // Recebe STDIN pelo Pai
                close(STDIN_FILENO);
                dup(pai2filho[0]);
                close(pai2filho[0]);
                close(pai2filho[1]);

                // Executa o jogo
                sprintf(str, "%s/%s", pdata->diretorio, pdata->c->jogo);
                execlp(str, str, NULL);
                printf("\n\n[ERRO] Erro a executar o jogo! [ERRO]\n");
                exit(ERRO);
            }
            close(filho2pai[1]); // Fecha o 1 pq vai ler do jogo pelo 0
            close(pai2filho[0]); // Fecha o 0 pq vai escrever para o jogo pelo 1
            pdata->pidFilho = pidFilho;
            pdata->continuaJogo = 1;

            while (pdata->continuaJogo) // Enquanto permanecer neste jogo
            {
                fflush(stdout);
                FD_ZERO(&fds);
                FD_SET(filho2pai[0], &fds); /* JOGO */
                FD_SET(fd, &fds);           /* NPIPE */
                // Encontra o valor máximo entre os dois pipes
                if (filho2pai[0] >= fd)
                    max = filho2pai[0];
                else
                    max = fd;

                res = select(max + 1, &fds, NULL, NULL, NULL);
                if (pdata->continua == 0 || pdata->continuaJogo == 0)
                    break;

                if (res > 0 && FD_ISSET(filho2pai[0], &fds))
                { /*Recebe STDOUT do jogo e envia para o cliente*/
                    if ((n = read(filho2pai[0], str, 100)) > 0 && pdata->suspende == 0)
                    {
                        fflush(stdout);
                        str[n] = '\0';
                        // Abre o fifo do cliente
                        fdcli = open(fifoCli, O_WRONLY);
                        // Envia os dados ao cliente
                        write(fdcli, str, strlen(str));
                        // Fecha o fifo do cliente
                        close(fdcli);
                        memset(str, 0, sizeof(str));
                    }
                    else
                    {
                        printf("\n[ERRO] Erro ao receber informacao do jogo! [ERRO]\n\n");
                        break;
                    }
                }
                else if (res > 0 && FD_ISSET(fd, &fds))
                { /* Envia mensagem do cliente para o STDIN do jogo */
                    if ((n = read(fd, str, 99)) > 0 && pdata->suspende == 0)
                    {
                        // Envia os dados ao jogo
                        write(pai2filho[1], str, strlen(str));
                        memset(str, 0, sizeof(str));
                    }
                }
            }
            pthread_mutex_lock(&trinco);
            // ===== Secção Crítica ==========
            kill(pidFilho, SIGUSR1);       // Termina o filho
            waitpid(pidFilho, &estado, 0); // Espera pelo final do filho
            close(filho2pai[0]);           // Para aqui
            close(pai2filho[1]);
            if (WIFEXITED(estado))
                pdata->c->pontos = WEXITSTATUS(estado);
            // ========= FIM ===============
            pthread_mutex_unlock(&trinco); // Destrancar
        }
    }
    close(fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[], char *envp[])
{
    int tempEspera = 0, duraCamp = 0, numMax, c_total = 0;
    int res, fd, n, numJ, fdcli;
    JOGO j[NJOGOS];
    SERVIDOR p;
    CLIENTE aux;
    char cmd[20], fifo[40];
    fd_set fds;

    obtemParametros(argc, argv, &tempEspera, &duraCamp);

    pthread_mutex_init(&trinco, NULL); // Criar trinco

    srand((unsigned int)time(NULL));

    char *diretorio = getenv("GAMEDIR");
    char *numMaxJ = getenv("MAXPLAYER");

    // INFO - CONSOLA -------------------------------------------------------------------------
    if (numMaxJ != NULL)
        numMax = atoi(numMaxJ);
    else {
        printf("A usar MAXPLAYER padrao...\n");
        numMax = 10; // Padrão
    }
       
    if (diretorio == NULL){
        printf("A usar GAMEDIR padrao...\n");
        diretorio = "./Games";
    }
        
    printf("MAXPLAYER=%s\nGAMEDIR=%s\n", numMaxJ, diretorio);

    // ----------------------------------------------------------------------------------------

    printf("Tempo de espera: %d\nDuracao Camp: %d\n\n", tempEspera, duraCamp);

    /*CRIAR PIPE DO SERVIDOR*/
    if (access(FIFO_SRV, F_OK) != 0) //Cria o fifo caso este não exista
        mkfifo(FIFO_SRV, 0600);
    else
    {
        printf("Já existe um servidor em curso!\n");
        exit(ERRO);
    }

    carregaJogos(j, diretorio, &numJ); // Array com todos os jogos disponíveis
    CLIENTE c[numMax];                 // Array que vai guardar os dados de todos os clientes

    fd = open(FIFO_SRV, O_RDWR); //Abre o fifo para escrita e leitura

    // Estrturas para as threads
    pthread_t tarefa[numMax]; // Threads para clientes
    TDATA tinfo[numMax];

    pthread_t contador; // Thread do contador campeonato
    TCOUNT cinfo;
    cinfo.clientes = c;
    cinfo.numCli = &c_total;
    cinfo.tarefa = tarefa;
    cinfo.duracao = duraCamp;
    cinfo.tempEspera = tempEspera;
    cinfo.tinfo = tinfo;
    cinfo.jogos = j;
    cinfo.numJ = numJ;
    while (1)
    {
        printf("\nComando: ");
        fflush(stdout);
        FD_ZERO(&fds);
        FD_SET(0, &fds);  /* TECLADO */
        FD_SET(fd, &fds); /* NPIPE */
        res = select(fd + 1, &fds, NULL, NULL, NULL);

        if (res > 0 && FD_ISSET(0, &fds))
        { /*Input do servidor */
            scanf("%s", cmd);
            for (int i = 0; cmd[i] != '\0' && cmd[0] != 'k' && cmd[0] != 's' && cmd[0] != 'r'; ++i) // Converte o comando para minusculas
                cmd[i] = tolower(cmd[i]);

            if (strcmp(cmd, "players") == 0)
            { // Mostra todos os jogadores
                mostraC(c, c_total);
            }
            else if (strcmp(cmd, "games") == 0)
            { // Mostra todos os jogos
                printf("GAMES\n");
                imprimeJogo(diretorio);
            }
            else if (cmd[0] == 'k')
            { // Kick
                if (c_total > 0)
                {
                    if (kick(c, &c_total, cmd, tarefa, tinfo, 2) == 1 && c_total < 2)
                    { // Se ficarem menos de 2 jogadores
                        printf("\n\n[AVISO] Apenas existe um jogador no jogo, não é possível continuar o campeonato! [AVISO]\n");
                        flagInicio = 0;
                        reniciarCampeonato(c, c_total, tinfo, j, numJ, tarefa);
                        acabaContador(&contador, &cinfo);
                    }
                }
                else
                    printf("Nao existem jogadores...\n");
            }
            else if (strcmp(cmd, "exit") == 0)
            { // Termina o servidor e todos os clientes
                encerraClientes(c, c_total);
                break;
            }
            else if (strcmp(cmd, "end") == 0)
            { // Termina o campeonato
                acabaContador(&contador, &cinfo);
                reniciarCampeonato(c, c_total, tinfo, j, numJ, tarefa);
                comecaContador(&contador, &cinfo);
            }
            else if (cmd[0] == 's')
            { // Suspende a comunicação entre o jogo e o jogador
                comunicacao(cmd, c, c_total, tinfo, 1);
            }
            else if (cmd[0] == 'r')
            { // Retoma a comunicação entre o jogo e o jogador
                comunicacao(cmd, c, c_total, tinfo, 0);
            }
            else
            { // Lista todos os comandos existentes para auxiliar o utilizador
                printf("\n[AVISO] COMANDO INEXISTENTE [AVISO]\n");
                printf("Lista: 'players', 'games', 'k<nome>','s<nome>','r<nome>', 'exit', 'end'\n");
            }
        }
        else if (res > 0 && FD_ISSET(fd, &fds))
        {                                        /* ENTRA O CÓDIGO PARA QUANDO RECEBE O INPUT DO UTILIZADOR */
            n = read(fd, &aux, sizeof(CLIENTE)); // Le o cliente

            if (n == sizeof(CLIENTE) && aux.cmd[0] == '#')
            {
                if (strcmp(aux.cmd, "#quit") == 0)
                {
                    if (kick(c, &c_total, aux.nome, tarefa, tinfo, 4) == 1 && c_total < 2)
                    { // Se ficarem menos de 2 jogadores
                        printf("\n\n[AVISO] Apenas existe um jogador no jogo, não é possível continuar o campeonato! [AVISO]\n");
                        flagInicio = 0;
                        reniciarCampeonato(c, c_total, tinfo, j, numJ, tarefa);
                        acabaContador(&contador, &cinfo);
                    }
                }
                // Cliente pede informação sobre o seu jogo
                else if (strcmp(aux.cmd, "#mygame") == 0)
                {
                    int aux2;
                    if (verificaNome(aux.nome, c, c_total, &aux2))
                    {
                        CLIENTE cEnvia = c[aux2];

                        // Abre o fifo do cliente
                        sprintf(fifo, FIFO_CLI, aux.pid);
                        fdcli = open(fifo, O_RDWR); // ERRO

                        // Envia os dados ao cliente
                        write(fdcli, &cEnvia, sizeof(CLIENTE));

                        // Fecha o fifo do cliente
                        close(fdcli);
                    }
                }
            }
            else if (n == sizeof(CLIENTE) && verificaNome(aux.nome, c, c_total, NULL) == 0 && c_total < numMax) // Verifica se a leitura foi bem sucedida e se não existe nenhum jogador com o mesmo nome do jogador que está a fazer o pedido)
            {
                c[c_total] = aux;

                // Cria Thread para atender o cliente
                tinfo[c_total].continua = 1;
                tinfo[c_total].continuaJogo = 1;
                tinfo[c_total].suspende = 0;
                tinfo[c_total].c = &c[c_total];
                tinfo[c_total].clientes = c;
                tinfo[c_total].numCli = &c_total;
                tinfo[c_total].diretorio = diretorio;
                pthread_create(&tarefa[c_total], NULL, trataCliente, (void *)&tinfo[c_total]);
                printf("\n\n[AVISO]Um novo cliente com o nome %s foi registado com sucesso! [AVISO]\n", aux.nome);
                ++c_total;

                if (flagInicio == 0 && c_total >= 2) // Verifica se já tem pelo menos dois jogadores e começa a contar o tempo de espera para começar o campeonato
                {
                    flagInicio = 1; // Flag que sinfica que já foi iniciado o alarme para começar o campeonato
                    atribuiJAClientes(c, c_total, j, numJ);
                    comecaContador(&contador, &cinfo);
                }
                else if (flagInicio == -1)
                {
                    atribuiJogoAleatorio(&c[c_total - 1], j, numJ);
                    comecaCampUnico(aux.pid);
                }
                else if (flagInicio == 1)
                    atribuiJogoAleatorio(&c[c_total - 1], j, numJ);
            }
            // Expulsar cliente que tenta fazer a ligação com nome repetido
            else if (n == sizeof(CLIENTE) && verificaNome(aux.nome, c, c_total, NULL) == 1)
                encerraC(c, &c_total, aux.pid, 1);
            // Expulsar Cliente por jão não ser permitido mais jogadores
            else if (n == sizeof(CLIENTE))
                encerraC(c, &c_total, aux.pid, 6);
        }
    }
    pthread_mutex_destroy(&trinco); // Elimina o trinco
    for (int i = 0; i < c_total; i++)
    {
        tinfo[i].continua = 0; // termina a thread
        tinfo[i].continuaJogo = 0;
        pthread_kill(tarefa[i], SIGUSR1); // Apenas para saltar o read e o scanf
        pthread_join(tarefa[i], NULL);    // espera pelo fim da thread
    }
    close(fd);
    unlink(FIFO_SRV);
    exit(OK);
}