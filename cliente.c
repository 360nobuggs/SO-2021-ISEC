#include "cliente.h"

//Variáveis globais, usadas para podermos fechar os fifos com sinais.
int server_fifo;
int client_fifo;
char nome_fifo[25];

void handler(int sig, siginfo_t* si, void* ucontext) {
    close(client_fifo);
    close(server_fifo);
    unlink(nome_fifo);
    exit(1);
}

void champShutdown(int sig) {
    fprintf(stderr, "\n O campeonato terminou.\n");
}

void myabort(const char* msg, int exit_status) {
    perror(msg);
    _exit(exit_status);
}

int main() {
    // Tratamento do Sinal SIGINT
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    signal(SIGUSR1, champShutdown);

    struct LigacaoS mensagem_server;//pergunta
    struct LigacaoC mensagem_client;//resposta

    int read_res;

    //Variáveis do select
    fd_set readfds;
    struct timeval tv;
    int selectValue;

    mensagem_client.userPID = getpid();//identifica o cliente
    sprintf(nome_fifo, CLIENT_FIFO, mensagem_client.userPID);

    if (mkfifo(nome_fifo, 0777) < 0) {
        perror("\n Erro ao criar o FIFO do cliente.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "\n FIFO do cliente criado.\n");

    server_fifo = open(SERVER_FIFO, O_WRONLY);//abre o FIFO do servidor para escrita
    if (server_fifo < 0) {
        fprintf(stderr, "\n O cliente não consegue ligar-se ao servidor.\n");
        unlink(nome_fifo);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "\n FIFO do servidor aberto para escrita.\n");

    if ((client_fifo = open(nome_fifo, O_RDWR)) < 0) { //o fifo do cliente so le
        perror("\n Erro ao abrir o FIFO do cliente.\n");
        close(server_fifo);
        unlink(nome_fifo);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "\n FIFO do Cliente aberto para leitura.\n");

    //A este ponto o cliente pode escrever no fifo do servidor, que este vai poder ler
    //E pode ler o fifo do cliente, no qual o servidor vai estar a escrever

    printf("Nome de utilizador: ");
    scanf("%s", mensagem_client.nome);
    mensagem_client.status = 0; // Indica ao árbitro que é a primeira mensagem que envia.
    write(server_fifo, &mensagem_client, sizeof(mensagem_client));//escreve no fifo do servidor para ele ler usando a mensagem para servidor

    //le a mensagem do server no pipe do cliente
    read_res = read(client_fifo, &mensagem_server, sizeof(mensagem_server));
    if (read_res == sizeof(mensagem_server)) {
        fprintf(stderr, "\nO cliente recebeu mensagem do servidor.\n");
        fprintf(stderr, mensagem_server.palavra);
    } else {
        fprintf(stderr, "\nO servidor não conseguiu entregar a mensagem ao cliente.\n");
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(client_fifo, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        selectValue = select(client_fifo + 1, &readfds, NULL, NULL, &tv);
        if (selectValue > 0) {
            // STDIN
            if (FD_ISSET(0, &readfds)) {
                scanf("%s", mensagem_client.palavra);//input do user
                mensagem_client.status = 1; // A partir daqui todas as mensagens são comandos.
                if (strcmp(mensagem_client.palavra, "fim") == 0) {
                    break;//se fim vai fechar pipes e sair
                }
                write(server_fifo, &mensagem_client, sizeof(mensagem_client));//escreve no fifo do servidor para ele ler usando a mensagem para servidor
            }

            if (FD_ISSET(client_fifo, &readfds)) {
                //le a mensagem do server no pipe do cliente
                read_res = read(client_fifo, &mensagem_server, sizeof(mensagem_server));
                if (read_res == sizeof(mensagem_server)) {
                    //fprintf(stderr, "\nO cliente recebeu mensagem do servidor.\n");
                    fprintf(stderr, mensagem_server.palavra);
                } else {
                    fprintf(stderr, "\nO servidor não conseguiu entregar a mensagem ao cliente.\n");
                }
            }
        }
    }
    //fechar os fifos
    close(client_fifo);
    close(server_fifo);
    unlink(nome_fifo);
    return 1;
}