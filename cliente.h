#ifndef CLIENTE_H
#define CLIENTE_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>

#include "comunication.h"

#define SERVER_FIFO "tmp/dict_fifo"
#define CLIENT_FIFO "tmp/resp%d_fifo"
#define TAM_MAX 50

#endif