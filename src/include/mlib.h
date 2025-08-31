#ifndef MLIB_H
#define MLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10
#define DEFAULT_SEED time(NULL)
#define DEFAULT_VIEW NULL

#define MIN_PLAYERS 1
#define MAX_PLAYERS 9

void handle_params(int argc, char *argv[], int *width, int *height, int *delay,int *timeout, int *seed, char **view_path, char **players, int *player_count);
    


#endif // MLIB_H