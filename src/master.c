#include "include/utils.h"
#include "include/mlib.h"

#define READ_END 0
#define WRITE_END 1

int main (int argc, char *argv[]){
    if(argc < 3){
        fprintf(stderr, "ERROR: No se ha especificado jugador");
        exit(EXIT_FAILURE);
    }
    unsigned short width, height;
    int delay, timeout, seed;
    unsigned int player_count; 
    char *view_path;
    char *players[MAX_PLAYERS];
    handle_params(argc, argv, &width, &height, &delay, &timeout, &seed, &view_path, &players, &player_count);

    printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\nnum_players: %d\n",
        width, height, delay, timeout, seed, view_path == NULL ? "-" : view_path, player_count);
    for(unsigned int i = 0; i < player_count; i++){
        printf("\t%s\n", players[i]);
    }
    sleep(3);

    //Crear memoria compartida
    game_t *game = open_and_map(SHM_GAME, O_CREAT | O_RDWR, sizeof(game_t) + width * height * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED);
    game_sync *sync = open_and_map(SHM_SYNC, O_CREAT | O_RDWR, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED);

    initialize_sems(sync);
    pid_t view_pid = initialize_game(game, width, height, player_count, seed, players);

    // Creacion de pipes
    int pipe_fd[MAX_PLAYERS][2];
    for(int i = 0; i < player_count ; i++){
        if(pipe(pipe_fd[i] == -1)){
            fprintf(stderr, "pipe error");
            exit(EXIT_FAILURE);
        }
    }


    int max_fd = 0;
    for(int i = 0 ; i < player_count ; i++){
        if(pipe_fd[i][READ_END] > max_fd){
            max_fd = pipe_fd[i][READ_END];
        }
    }
    fd_set read_fd;
    int timeout = DEFAULT_TIMEOUT;

    // Loop principal de juego
    while(!game->gameFinished){

        int ready = select(max_fd + 1, &read_fd, NULL, NULL, timeout);
        if(ready == -1){
            fprintf(stderr, "error select");
            exit(EXIT_FAILURE);
        } else if(ready == 0){
            fprintf(stderr, "timeout select");
            break;
        }

        for(int i = 0 ; i < player_count ; i++){
            
        }

    }
    

    // Recordar: limpiar todos los recursos del sistema (memoria compartida, pipes y procesos), hacer un sleep antes del return 

}
