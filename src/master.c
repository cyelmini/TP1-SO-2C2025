#include "include/utils.h"

int main (int argc, char *argv[]){
    if(argc < 3){
        fprintf(stderr, "ERROR: No se ha especificado jugador");
        exit(EXIT_FAILURE);
    }
    int width;
    int height;
    int delay;
    int timeout;
    int seed; 
    char *view_path;
    int *players;
    int player_count;
    handle_params(argc, argv, &width, &height, &delay, &timeout, &seed, &view_path, &players, player_count);

    

}
