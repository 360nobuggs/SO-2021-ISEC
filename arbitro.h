#ifndef ARBITRO_H
#define ARBITRO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <pthread.h> 
#include <string.h>

#include "comunication.h"

#define DEFAULT_MAXPLAYERS 10
#define DEFAULT_GAMEDIR "."

#define SERVER_FIFO "tmp/dict_fifo"
#define CLIENT_FIFO "tmp/resp%d_fifo"
#define TAM_MAX 50

int verifInt(char* arg);

#endif