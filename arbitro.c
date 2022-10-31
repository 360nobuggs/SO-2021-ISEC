#include "arbitro.h"
// Variáveis globais
int s_fifo, c_fifo, res;
int connectedUsers = 0;
int championshipStarted = 0;
int championshipFinished = 0;
int alarmPID;
struct LigacaoC userList[30];
//Userlist não está declarada como global pois tem tamanho MAXPLAYERS e o valor não é constante

//Variáveis de Ambiente
int MAXPLAYERS = DEFAULT_MAXPLAYERS;
char* GAMEDIR = DEFAULT_GAMEDIR;
//Variáveis recebidas por argumentos
int tempoCampeonato, tempoEspera;

void shutdown() {
  printf("Exiting program...\n");
  close(s_fifo);
  unlink(SERVER_FIFO);
}

void sigHandler(int sig) {
  if (sig == SIGINT) {
    shutdown();
    exit(0);
  }
}

void alarmHandler(int sig) {
  if (sig == SIGALRM) {
    //Tem de mandar um sigUSR1 a todos os clientes e todos os jogos para notificar que o jogo terminou.
    for (int i = 0; i < connectedUsers; i++) {
#ifdef DEBUG
      printf("Sending USR1 to user %s (%d)\n", userList[i].nome, userList[i].userPID);
      printf("Sending USR1 to game (%d)\n", userList[i].gamePID);
#endif
      kill(userList[i].userPID, SIGUSR1);
      kill(userList[i].gamePID, SIGUSR1);
    }
  }
}


void listAllGames(char* gamedir) {
  DIR* d; // Usado para ler os ficheiros existentes
  struct dirent* dir;

  d = opendir(gamedir);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      //Verifica se o ficheiro começa por g e se acaba em c
      if (dir->d_name[0] == 'g' && dir->d_name[strlen(dir->d_name) - 1] == 'c')
        printf("-> %s\n", dir->d_name);
    }
    closedir(d);
  }
}

// void removerUser(int userPID) {
//   for (int i = 0; i < connectedUsers; i++) {

//   }
// }

int verifInt(char* arg) {
  int num;
  if ((num = atoi(arg)) == 0 && arg != "0") {
    printf("[ERRO] Argumento invalido!\n");
    printf("Utilize -h para mais informacoes.\n");
    exit(4); // Erro Invalid Arguments
  }
  return num;
}

char* randomGame(char* dir, int pid) {
  srand(time(NULL) + pid);
  DIR* gd; // Usado para ler os ficheiros existentes
  struct dirent* gamesdir;
  int gamecount = 0;
  int currgame = 1;
  int randgame;

  //Obtém o número total de jogos disponíveis
  gd = opendir(dir);
  if (gd) {
    while ((gamesdir = readdir(gd)) != NULL) {
      //Verifica se o ficheiro começa por g e se acaba em c
      if (gamesdir->d_name[0] == 'g' && gamesdir->d_name[strlen(gamesdir->d_name) - 1] == 'c')
        gamecount++;
    }
    closedir(gd);
  }

  //Número aleatório entre 1 e gamecount
  randgame = rand() % gamecount + 1;
#ifdef DEBUG
  printf("Game count: %d\n", gamecount);
  printf("Random number generated: %d\n", randgame);
#endif

  gd = opendir(dir);
  if (gd) {
    while ((gamesdir = readdir(gd)) != NULL) {
      if (gamesdir->d_name[0] == 'g' && gamesdir->d_name[strlen(gamesdir->d_name) - 1] == 'c') {
#ifdef DEBUG
        printf("Current Game index: %d\n", currgame);
#endif
        if (currgame == randgame) {
#ifdef DEBUG
          printf("Return value: %s\n", gamesdir->d_name);
#endif
          gamesdir->d_name[strlen(gamesdir->d_name) - 2] = '\0';
          return gamesdir->d_name;
        }

        currgame++;
      }
    }
    closedir(gd);
  }
#ifdef DEBUG
  printf("Return value: NULL\n");
#endif
  return NULL; // Se não encontrar o jogo por alguma razão.
}

struct LigacaoC* checkUserConnected(struct LigacaoC* userList, int MAXPLAYERS, struct LigacaoC mensagemForServer) {
  for (int i = 0; i < MAXPLAYERS; i++) {
    if (strcmp(userList[i].nome, mensagemForServer.nome) == 0) {
      return &userList[i];
    }
  }
  return NULL;
}

struct LigacaoC getWinner() {
  struct LigacaoC winner;
  int maxScore = 0;
  for (int i = 0; i < connectedUsers; i++) {
    if (userList[i].gamescore >= maxScore)
      winner = userList[i];
  }

  return winner;
}

void* waitGameFinish(void* index) {
  char nome_fifo_cliente[50];
  struct LigacaoS mensagemForClient;
  long i = (long) index;
  int status;

  waitpid(userList[i].gamePID, &status, 0);
  if (WIFEXITED(status) || WIFSIGNALED(status)) {
    userList[i].gamescore = WEXITSTATUS(status);
  }

  championshipStarted = 0;
  championshipFinished = 1;

  sprintf(nome_fifo_cliente, CLIENT_FIFO, userList[i].userPID);

  if ((c_fifo = open(nome_fifo_cliente, O_WRONLY)) < 0) {
    printf("Erro a abrir o fifo do cliente %s (PID: %d)\n", userList[i].nome, userList[i].userPID);
  } else {
    sprintf(mensagemForClient.palavra, "O seu resultado foi %d\n", userList[i].gamescore);
    res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));

    strcpy(userList[i].gameName, "");
    userList[i].gamePID = 0;
  }
}

void* gameThread(void* index) {
  //struct LigacaoC* userList = (struct LigacaoC*) list;
  pthread_t gameFinishT;
  long i = (long) index;
  struct LigacaoS mensagemForClient;
  char game[50], str[100], nome_fifo_cliente[50];
  int readRes;
  pipe(userList[i].gpFD);
  pipe(userList[i].pgFD);

  //Variáveis do select
  fd_set readfds;
  struct timeval tv;
  int selectValue;

  sprintf(game, "%s/%s", GAMEDIR, randomGame(GAMEDIR, userList[i].userPID));
  strcpy(userList[i].gameName, game);

  if ((userList[i].gamePID = fork()) == 0) {
    //userList[i].gamePID = getpid();

#ifdef DEBUG
    printf("Game path: %s\n", game);
#endif

    close(1); // fecha o stdout do jogo
    close(userList[i].gpFD[0]); // fecha o stdin do pipe
    dup(userList[i].gpFD[1]); // coloca o stdout do pipe no stdout do jogo
    close(userList[i].gpFD[1]); // fecha o stdout do pipe (o que já não precisamos de usar)

    close(0);
    close(userList[i].pgFD[1]);
    dup(userList[i].pgFD[0]);
    close(userList[i].pgFD[0]);

    //execl("/home/code/TrabalhoPratico/g_rps", "g_rps", NULL, NULL);
    execl(game, game, NULL);
  } else {
    close(userList[i].pgFD[0]);
    close(userList[i].gpFD[1]);

    // Começa uma "mini-thread" responsável por aguardar pelo fim do jogo
    pthread_create(&gameFinishT, NULL, waitGameFinish, (void*) i);

    while (championshipStarted) {

      //escreve para o jogador
      while ((readRes = read(userList[i].gpFD[0], str, sizeof(str) - sizeof(char))) > 0) {
        str[readRes] = '\0';
        strcpy(mensagemForClient.palavra, str);

        // Antes de escrever para o filho é necessário reabrir o fifo do filho respetivo 
        sprintf(nome_fifo_cliente, CLIENT_FIFO, userList[i].userPID);

        if ((c_fifo = open(nome_fifo_cliente, O_WRONLY)) < 0) {
          printf("Erro a abrir o fifo do cliente %s (PID: %d)\n", userList[i].nome, userList[i].userPID);
        } else {
          res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));
        }
      }
    }
    close(userList[i].gpFD[0]);
    close(userList[i].pgFD[1]);

    pthread_join(gameFinishT, NULL);
  }
}


//Thread responsável pelo campeonato
void* championshipCountdown(void* list) {
  //struct LigacaoC* userList = (struct LigacaoC*) list;
  struct LigacaoC winner;
  char nome_fifo_cliente[50];
  struct LigacaoS mensagemForClient;
  pthread_t gameT;
  printf("O campeonato vai comecar daqui a %ld segundos!\n", tempoEspera);
  sleep(tempoEspera);
  printf("O campeonato vai comecar!\n");
  alarm(tempoCampeonato);
  championshipStarted = 1;


  //Sorting de cada jogo para um jogador
  /*Para cada jogador chamar a função randomGame(GAMEDIR) de forma a atribuir um jogo aleatório*/
  for (long i = 0; i < connectedUsers; i++) {
    //pthread_create(&gameT, NULL, gameThread, (void*) &userList[i]);
    pthread_create(&gameT, NULL, gameThread, (void*) i);
  }

  pthread_join(gameT, NULL);

  winner = getWinner();
  // Vencedor do campeonato
  for (int i = 0; i < connectedUsers; i++) {
    sprintf(nome_fifo_cliente, CLIENT_FIFO, userList[i].userPID);

    if ((c_fifo = open(nome_fifo_cliente, O_WRONLY)) < 0) {
#ifdef DEBUG
      perror("\nErro ao abrir o FIFO do cliente (WRONLY/BLOCKING).\n");
#endif
      // shutdown();
      // exit(EXIT_FAILURE);
    } else {
      sprintf(mensagemForClient.palavra, "O vencedor do campeonato foi %s com %d pontos\n", winner.nome, winner.gamescore);
      res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));
    }
  }
}

void* clientServerComm(void* list) {
  struct LigacaoS mensagemForClient;
  struct LigacaoC mensagemForServer;
  struct LigacaoC winner;
  char nome_fifo_cliente[50];
  char auxMsg[TAM_MAX];
  int countdownStart = 0;
  //Thread
  pthread_t champThread;

  //struct LigacaoC* userList = (struct LigacaoC*) list;
  struct LigacaoC* currentUser;

  do {
    res = read(s_fifo, &mensagemForServer, sizeof(mensagemForServer));
    if (res < 0) {
      perror("\n Erro a ler do cliente.");
    }

    if (mensagemForServer.status == 0)
      fprintf(stderr, "\n Cliente com o PID %d esta a tentar conectar.\n", mensagemForServer.userPID);
    else
      fprintf(stderr, "\n Mensagem recebida do cliente com o PID %d: [%s]\n", mensagemForServer.userPID, mensagemForServer.palavra);

    sprintf(nome_fifo_cliente, CLIENT_FIFO, mensagemForServer.userPID);

    // Remover cliente da lista de clientes
    if ((c_fifo = open(nome_fifo_cliente, O_WRONLY)) < 0) {
#ifdef DEBUG
      perror("\nErro ao abrir o FIFO do cliente (WRONLY/BLOCKING).\n");
#endif
      // shutdown();
      // exit(EXIT_FAILURE);
    } else {
      // Adiciona o users ao array se este não existir
      if (connectedUsers < MAXPLAYERS) {
        //Verifica se o utilizador já existe
        // - Se existir avisa o utilizador que o nome já se encontra em uso e manda um sinal para terminar a execução do programa
        // - Se não existir adiciona à lista de utilizadores ligados
        if (mensagemForServer.status == 0) {
          if (checkUserConnected(userList, MAXPLAYERS, mensagemForServer) == NULL) {
            strcpy(userList[connectedUsers].nome, mensagemForServer.nome);
            userList[connectedUsers].userPID = mensagemForServer.userPID;
            userList[connectedUsers].locked = 0;
            strcpy(userList[connectedUsers].gameName, "");
            connectedUsers++;
            strcpy(mensagemForClient.palavra, "Bem-vindo! Pode agora inserir comandos.\n\n");
            res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));
            if (res < 0) {
              perror("\n Erro a escrever para o cliente.");
            }

            //Se o número de utilizadores for igual ou superior a 2 inicia o cronometro
            if (connectedUsers >= 2 && countdownStart == 0) {
              countdownStart = 1;
              //pthread_create(&champThread, NULL, championshipCountdown, (void*) userList);
              pthread_create(&champThread, NULL, championshipCountdown, NULL);
            }
          } else {
            kill(mensagemForServer.userPID, SIGINT);
            connectedUsers--;
          }
        }
      } else {
        kill(mensagemForServer.userPID, SIGINT);
        connectedUsers--;
      }

      if (mensagemForServer.status != 0) {
        currentUser = checkUserConnected(userList, MAXPLAYERS, mensagemForServer);

        if (currentUser->locked) {
          strcpy(mensagemForClient.palavra, "Comunicacoes interrompidas. Aguarde.\n\n");
          res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));
        } else {
          // Tratamento dos comandos
          fprintf(stderr, "\nFIFO do cliente aberto para escrita.\n");
          if (strcmp(mensagemForServer.palavra, "#mygame") == 0) {
            //currentUser = checkUserConnected(userList, MAXPLAYERS, mensagemForServer);
            if (currentUser == NULL || strcmp(currentUser->gameName, "") == 0)
              strcpy(auxMsg, "Nao ha nenhum jogo em execucao.\n\n");
            else
              sprintf(auxMsg, "Estas a jogar %s.\n\n", currentUser->gameName);
            strcpy(mensagemForClient.palavra, auxMsg);
            res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));
            if (res < 0) {
              perror("\n Erro a escrever para o cliente.");
            }
          } else if (strcmp(mensagemForServer.palavra, "#quit") == 0) {
            strcpy(mensagemForClient.palavra, "Pedido para sair recebido.\n\n");
            res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));

            kill(mensagemForServer.userPID, SIGINT);
            if(strcmp(mensagemForServer.gameName, "") != 0) {
              kill(mensagemForServer.gamePID, SIGINT);
            }

            if (res < 0) {
              perror("\n Erro a escrever para o cliente.");
            }
          } else {
            // É necessário verificar se o campeonato começou ou não
            // Se não tiver começado, mostra a mensagem de comando não reconhecido
            // Se tiver, o árbitro deve reencaminhar a mensagem para o jogo

            if (championshipStarted) {
              // Reencaminha mensagem para o jogo
              //encontrar o cliente na lista userList e usar esse file Descriptor para ser igual à da thread.
              currentUser = checkUserConnected(userList, MAXPLAYERS, mensagemForServer);
              char nl = '\n';
              //strncat(mensagemForServer.palavra, &nl, 1);
              mensagemForServer.palavra[strlen(mensagemForServer.palavra) + 1] = '\0';
              mensagemForServer.palavra[strlen(mensagemForServer.palavra)] = '\n';
              //printf("Vou mandar '%s'\n", mensagemForServer.palavra);
              res = write(currentUser->pgFD[1], mensagemForServer.palavra, strlen(mensagemForServer.palavra));
              if (res < 0) {
                perror("\n Erro a escrever para o cliente.");
              }
              memset(mensagemForServer.palavra, '\0', sizeof(mensagemForServer.palavra));
            } else {
              strcpy(mensagemForClient.palavra, "Comando nao reconhecido.\n\n");
              res = write(c_fifo, &mensagemForClient, sizeof(mensagemForClient));
              if (res < 0) {
                perror("\n Erro a escrever para o cliente.");
              }
            }
          }
        }
      }
    }
  } while (1); // TEMPORÁRIO, ADICIONAR FORMA DE SAIR
}

int main(int argc, char* argv[], char* envp[]) {
  int opt;
  char cmd[250];
  char* token;
  struct LigacaoS mensagemForCliente;
  char nome_fifo_cliente[50];

  signal(SIGINT, sigHandler);
  signal(SIGALRM, alarmHandler);

  char* envAux; // Variável auxiliar para a obtenção das VA

  /* -- Leitura dos Argumentos -- */

  if (argc <= 1 || argc < 5 && (strcmp(argv[1], "-h") != 0)) {
    printf("[ERRO] Argumentos em falta!\n");
    printf("Utilize -h para mais informacoes.\n\n");
    exit(3); // Erro Missing Arguments
  }

  while ((opt = getopt(argc, argv, "ht:e:")) != -1) {
    switch (opt) {
    case 't':
      tempoCampeonato = verifInt(optarg);
      break;
    case 'e':
      tempoEspera = verifInt(optarg);
      break;
    case 'h':
      printf("-t <int> -> Duracao do campeonato em minutos.\n");
      printf("-e <int> -> Tempo de espera em segundos (apos pelo menos 2 jogadores se conectarem).\n\n");
      exit(0);
      break;
    default:
      printf("[ERRO] Opcao invalida. Utilize -h para mais informacoes.\n\n");
      break;
    }
  }

#ifdef DEBUG
  printf("[DEBUG] Duracao do campeonato: %d\n", tempoCampeonato);
  printf("[DEBUG] Tempo de espera: %d\n", tempoEspera);
#endif

  /* -- TRATAMENTO DAS VARIÁVEIS DE AMBIENTE -- */
  if ((envAux = getenv("MAXPLAYERS")) != NULL) {
    MAXPLAYERS = atoi(envAux);
  } else {
    printf("[WARNING] Nao foi possivel carregar a variavel de ambiente 'MAXPLAYERS'. A usar o valor predefinido '%d'.\n", MAXPLAYERS);
  }

  if (getenv("GAMEDIR") != NULL) {
    GAMEDIR = getenv("GAMEDIR");
  } else {
    printf("[WARNING] Nao foi possivel carregar a variavel de ambiente 'GAMEDIR'. A usar o valor de defeito '%s'.\n", GAMEDIR);
  }

#ifdef DEBUG
  printf("[DEBUG] MAXPLAYERS: %d\n", MAXPLAYERS);
  printf("[DEBUG] GAMEDIR: '%s'\n", GAMEDIR);
#endif

  /* -- CRIAÇÃO DO FIFO SERVIDOR -- */

  if ((res = mkfifo(SERVER_FIFO, 0777)) < 0) {
    perror("\nErro ao criar o FIFO do Servidor.\n");
    shutdown();
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "\nFIFO servidor criado com sucesso!");

  if ((s_fifo = open(SERVER_FIFO, O_RDWR)) < 0) {
    perror("\nErro ao abrir o FIFO do servidor(RDWR/BLOCKING).\n");
    shutdown();
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "\nFIFO aberto para leitura.\n");

  // -- Cria thread para a comunicação entre servidor e cliente  --
  pthread_t commThread;
  // pthread_create(&commThread, NULL, clientServerComm, (void*) userList);
  pthread_create(&commThread, NULL, clientServerComm, NULL);
  // -- END --

  do {
    fgets(cmd, sizeof(cmd), stdin);
    strtok(cmd, "\n");

#ifdef DEBUG
    printf("[DEBUG] Command: %s\n", cmd);
#endif

    if (strcmp(cmd, "players") == 0) {
      for (int i = 0; i < connectedUsers; i++) {
        printf("-> %s\n", userList[i].nome);
      }
    } else if (strcmp(cmd, "games") == 0) {
      listAllGames(GAMEDIR);
    } else if (strcmp(cmd, "exit") == 0) {
      for(int i = 0; i < connectedUsers; i++){
        kill(userList[i].userPID, SIGINT);
        if(strcmp(userList[i].gameName, "") != 0)
          kill(userList[i].gamePID, SIGINT);
      }
      shutdown();
      exit(0);
      break;
    } else if (cmd[0] == 'k') {
      token = strtok(cmd, "k");

      for (int i = 0; i < connectedUsers; i++) {
        if (strcmp(userList[i].nome, token) == 0)
          kill(userList[i].userPID, SIGINT);
      }

      printf("\nKick Player\n");
    } else if (cmd[0] == 's') {
      token = strtok(cmd, "s");

      for (int i = 0; i < connectedUsers; i++) {
        if (strcmp(userList[i].nome, token) == 0) {
          // Abre o fifo para enviar ao cliente
          sprintf(nome_fifo_cliente, CLIENT_FIFO, userList[i].userPID);

          // Remover cliente da lista de clientes
          if ((c_fifo = open(nome_fifo_cliente, O_WRONLY)) < 0) {
#ifdef DEBUG
            perror("\nErro ao abrir o FIFO do cliente (WRONLY/BLOCKING).\n");
#endif
            // shutdown();
            // exit(EXIT_FAILURE);
          } else {
            strcpy(mensagemForCliente.palavra, "A sua ligacao com o servidor foi suspensa.\n");
            res = write(c_fifo, &mensagemForCliente, sizeof(mensagemForCliente));
            userList[i].locked = 1;
          }
        }
      }
      printf("Jogador suspendido com sucesso.\n");
    } else if (cmd[0] == 'r') {
      token = strtok(cmd, "r");

      for (int i = 0; i < connectedUsers; i++) {
        if (strcmp(userList[i].nome, token) == 0) {
          // Abre o fifo para enviar ao cliente
          sprintf(nome_fifo_cliente, CLIENT_FIFO, userList[i].userPID);

          // Remover cliente da lista de clientes
          if ((c_fifo = open(nome_fifo_cliente, O_WRONLY)) < 0) {
#ifdef DEBUG
            perror("\nErro ao abrir o FIFO do cliente (WRONLY/BLOCKING).\n");
#endif
            // shutdown();
            // exit(EXIT_FAILURE);
          } else {
            kill(SIGCONT, userList[i].userPID);
            userList[i].locked = 0;
            strcpy(mensagemForCliente.palavra, "A sua ligacao com o servidor foi restablecida.\n");
            res = write(c_fifo, &mensagemForCliente, sizeof(mensagemForCliente));
          }
        }
      }
      printf("Jogador retomado com sucesso.\n");
    } else if (strcmp(cmd, "end") == 0) {
      if(championshipStarted == 1 && championshipFinished == 0)
        raise(SIGALRM);
      printf("Campeonato terminado.\n");
    } else {
      printf("\nComando nao detetado!\n");
    }
  } while (strcmp(cmd, "exit"));

  return 0;
}