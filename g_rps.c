#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "g_rps.h"

int score = 0;

int scoreHandler(char* option) {
  // 1 - Rock
  // 2 - Paper
  // 3 - Scissors
  int userOption;
  int cpuOption;

  //Gera um número aleatório entre 1 e 3 para a jogada do CPU
  cpuOption = rand() % 3 + 1;

  //Interpretar o input do utilizador para número
  if (strcmp(option, "rock") == 0)
    userOption = 1;
  else if (strcmp(option, "paper") == 0)
    userOption = 2;
  else if (strcmp(option, "scissors") == 0)
    userOption = 3;
  else
    return 0; // Caso não consiga ler o comando ignora.

  if (userOption == cpuOption) {
    printf("Empate!\n");
    return 0;
  } else if (userOption == 1 && cpuOption == 3) {
    printf("Jogador ganha!\n");
    return 1;
  } else if (userOption == 2 && cpuOption == 1) {
    printf("Jogador ganha!\n");
    return 1;
  } else if (userOption == 3 && cpuOption == 2) {
    printf("Jogador ganha!\n");
    return 1;
  } else {
    printf("CPU ganha.\n");
    return -1;
  }
}

void handler(int sig, siginfo_t* si, void* ucontext) {
  exit(score);
}

int main() {
  setbuf(stdout, NULL);
  srand(time(NULL));
  char option[50];
  int chr;

  //Tratamento do Sinal SIGUSR1
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGUSR1, &sa, NULL);

  do {
    printf("rock, paper or scissors?\n");
    if(!scanf("%s", option)){
      while((chr = getchar()) != '\n' && chr != EOF);
      continue;
    }
    
    score += scoreHandler(option);

    if(score < 0)
      score = 0;
      
  } while(1);
}