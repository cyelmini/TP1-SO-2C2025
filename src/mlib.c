#include "include/mlib.h"
#include "include/utils.h"
#include <string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

static const int DIRS[8][2] = {
    { 0,-1}, { 1,-1}, { 1, 0}, { 1, 1},
    { 0, 1}, {-1, 1}, {-1, 0}, {-1,-1}
};

void initialize_board(unsigned short width, unsigned short height, int board[], int seed){
    srand(seed);
    for(unsigned short i = 0; i < height; i++){
        for(unsigned short j = 0; j < width; j++){
            board[i * width + j] = rand() % 9 + 1;
        }
    }
}

pid_t initialize_game(game_t *game, unsigned short width, unsigned short height, int player_count, int seed, char **players, char *view, int pipe_fd[][2]){
    game->width = width;
    game->height = height;
    game->playerCount = player_count;
    game->gameFinished = 0;
    initialize_board(game->width, game->height, game->board, seed);
    return initialize_players_and_view(game, players, view, pipe_fd);
}

pid_t initialize_players_and_view(game_t *game, char **players, char *view, int pipe_fd[][2]){
    char width[16];
    char height[16];
    sprintf(width,"%d", game->width); 
    sprintf(height,"%d", game->height);
    char * argv[4] = {NULL, width, height, NULL}; 

    // Lanzar jugadores
    for(unsigned int i = 0; i < game->playerCount; i++){
        snprintf(game->players[i].name, sizeof(game->players[i].name), "%s", players[i]);
        game->players[i].score = 0;
        game->players[i].invalidMoves = 0;
        game->players[i].validMoves = 0;
        game->players[i].x = (i * game->width) / game->playerCount; 
        game->players[i].y = (i * game->height) / game->playerCount;
        game->players[i].isBlocked = 0;

        pid_t pid = fork();
        if(pid < 0){
            fprintf(stderr, "ERROR: fork");
            exit(EXIT_FAILURE);

        } else if(pid == 0){ // hijo
            // Close all unused pipe ends
            for (unsigned int j = 0; j < game->playerCount; j++) {
                if (j != i) {
                    close(pipe_fd[j][READ_END]);
                    close(pipe_fd[j][WRITE_END]);
                }
            }
            // Close read end of this player's pipe
            close(pipe_fd[i][READ_END]);
            // Redirect stdout to this player's write end
            dup2(pipe_fd[i][WRITE_END], STDOUT_FILENO);
            close(pipe_fd[i][WRITE_END]);

            argv[0] = game->players[i].name;
            if(execve(game->players[i].name, argv, NULL) == -1){
                fprintf(stderr, "ERROR: execve");
                exit(EXIT_FAILURE);
            }
        }else{ // padre
            game->players[i].player_pid = pid;
            close(pipe_fd[i][WRITE_END]);
        }
    }

    // Lanzar vista
    pid_t pid_view = -1;
    if(view != NULL){
        argv[0]= view;
        pid_view = fork();

        if(pid_view < 0){
            fprintf(stderr, "ERROR: fork");
            exit(EXIT_FAILURE);
        }else if( pid_view == 0 ){
            if(execve(view, argv, NULL) == -1){
                fprintf(stderr, "ERROR: execve");
                exit(EXIT_FAILURE);
            }
        }
    }
    return pid_view;
}

void initialize_sems(game_sync *sync){
    sem_init(&sync->printNeeded, 1, 0);
    sem_init(&sync->printDone, 1, 0);
    sem_init(&sync->masterMutex, 1, 1);
    sem_init(&sync->gameMutex, 1, 1);
    sem_init(&sync->readersMutex, 1, 1);
    sync->readersCount = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        sem_init(&sync->playerTurn[i], 1, 0);
    }
}

void check_invalid(const char *c) {
    if (c == NULL || strncmp(c, "-", 1) == 0) {
        fprintf(stderr, "ERROR: Argumento inválido\n");
        exit(EXIT_FAILURE);
    }
}

void handle_params(int argc, char *argv[], unsigned short *width, unsigned short *height, int *delay,int *timeout, int *seed, char **view_path, char **players, unsigned int *player_count){
    *width = DEFAULT_WIDTH;
    *height = DEFAULT_HEIGHT;
    *delay = DEFAULT_DELAY;
    *timeout = DEFAULT_TIMEOUT;
    *seed = DEFAULT_SEED;
    *view_path = DEFAULT_VIEW;
    *player_count= 0;
    int param;

    int opt;
    while((opt = getopt (argc, argv, "w:h:d:t:s:v:p:")) != -1){
        switch(opt){
            case 'w':
                check_invalid(optarg);
                if((param = atoi(optarg)) < 10){
                    perror("ERROR: El ancho debe ser al menos 10\n");
                    exit(EXIT_FAILURE);
                }
                *width = param;
                break;
            case 'h':
                check_invalid(optarg);
                if((param = atoi(optarg)) < 10){
                    perror("ERROR: El alto debe ser al menos 10\n");
                    exit(EXIT_FAILURE);
                }
                *height = param;
                break;
            case 'd':
                check_invalid(optarg);
                *delay = atoi(optarg);
                break;
            case 't':
                check_invalid(optarg);
                *timeout = atoi(optarg);
                break;
            case 's':
                check_invalid(optarg);
                *seed = atoi(optarg);
                break;
            case 'v':
                check_invalid(optarg);
                *view_path = optarg;
                break;
            case 'p': 
                {
                int index_player = optind - 1;
                if (index_player >= argc) {
                    fprintf(stderr, "ERROR: Debe especificar al menos un jugador después de -p");
                    exit(EXIT_FAILURE);
                }
                unsigned int added = 0;
                int k = index_player;
                while (k < argc && argv[k][0] != '-') {
                    if ((*player_count + added) >= MAX_PLAYERS) {
                        fprintf(stderr, "ERROR: Máximo 9 jugadores");
                        exit(EXIT_FAILURE);
                    }
                    players[*player_count + added] = argv[k];
                    added++;
                    k++;
                }
                *player_count += added;
                optind = index_player + added;
                }
                break;
            default:
                fprintf(stderr, "ERROR: Opción no reconocida: -%c\n", optopt);
                exit(EXIT_FAILURE);
        }
    }
}

int validate_and_apply_move(game_t *game, unsigned int idx, unsigned char move){
    if(move > 7) return 0;
    int nx = game->players[idx].x + DIRS[move][0];
    int ny = game->players[idx].y + DIRS[move][1];

    if(nx < 0 || nx >= game->width || ny < 0 || ny >= game->height) return 0;

    int pos = ny * game->width + nx;
    if(game->board[pos] <= 0) return 0;

    for(unsigned int i = 0; i < game->playerCount; i++){
        if(i != idx && !game->players[i].isBlocked && game->players[i].x == nx && game->players[i].y == ny){
            return 0;
        }
    }

    game->players[idx].x = nx;
    game->players[idx].y = ny;
    game->players[idx].score += game->board[pos];
    game->board[pos] = -idx;

    return 1;
}