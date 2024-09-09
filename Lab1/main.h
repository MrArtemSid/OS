#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

char * fn_list[];
char * fn_list_help[];
int fn_cnt ();
int status = 0; // статус выполнения, если не 0, то программа завершается
bool is_background = false;