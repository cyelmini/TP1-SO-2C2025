#include "include/mlib.h"

void handle_params(int argc, char *argv[], int *width, int *height, int *delay,int *timeout, int *seed, char **view_path, char **players, int *player_count){
    *width = DEFAULT_WIDTH;
    *height = DEFAULT_HEIGHT;
    *delay = DEFAULT_DELAY;
    *timeout = DEFAULT_TIMEOUT;
    *seed = DEFAULT_SEED;
    *view_path = DEFAULT_VIEW;
    *player_count= 0;
    
}
