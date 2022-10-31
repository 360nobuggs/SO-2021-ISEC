#ifndef COMUNICATION_H
#define COMUNICATION_H

#define TAM_MAX 50

// Struct Ligacao
typedef struct LigacaoC ligacao_servtocli, * pLigacao;
typedef struct LigacaoC ligacao_clitoserv, * sLigacao;

struct LigacaoS {//pergunta ao cliente
    int userPID;     // PID do jogador
    char palavra[TAM_MAX];
};
struct LigacaoC {//resposta do cliente
    char nome[TAM_MAX];
    char palavra[TAM_MAX];
    int userPID;
    char gameName[TAM_MAX];
    int gamePID;
    int gamescore;
    int gpFD[2];
    int pgFD[2];
    int status; // status-> 0 para primeira mensagem (onde o user é registado), 1 para tudo o resto
    int locked;
};


#endif