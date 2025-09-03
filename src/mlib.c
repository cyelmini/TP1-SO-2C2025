#include "include/mlib.h"
#include "include/utils.h"


void initialize_board(unsigned short width, unsigned short height, int board[], int seed){
    srand(seed);
    for(unsigned short i = 0; i < height; i++){
        for(unsigned short j = 0; j < width; j++){
            board[i * width + j] = rand() % 9 + 1; // Números entre 1 y 9
        }
    }
}

pid_t initialize_game(game_t *game, unsigned short width, unsigned short height, int player_count, int seed, char **players){
    game->width = width;
    game->height = height;
    game->playerCount = player_count;
    game->gameFinished = false;
    initialize_board(game->width, game->height, game->board, seed);
    initialize_players(game, players);
    return 1; //esto esta obviamente mal lo dejo asi porque no llegue a la vista
}

void initialize_players(game_t *game, char **players){ //hace falta aun distribuirlos en el espacio x y
    for(unsigned int i = 0; i < game->playerCount; i++){
        snprintf(game->players[i].name, sizeof(game->players[i].name), "%s", players[i]);
        game->players[i].score = 0;
        game->players[i].invalidMoves = 0;
        game->players[i].validMoves = 0;
        game->players[i].x = (i * game->width) / game->playerCount; 
        game->players[i].y = (i * game->height) / game->playerCount;
        game->players[i].isBlocked = false;
        game->players[i].player_pid = 0; 
    }
}

void initialize_sems(game_sync *sync){
    sem_init(&sync->printNeeded, 1, 0);
    sem_init(&sync->printDone, 1, 0);
    sem_init(&sync->masterMutex, 1, 1);
    sem_init(&sync->gameMutex, 1, 1);
    sem_init(&sync->readersMutex, 1, 1);
    sync->readersCount = 0;
    for (int i = 0; i < 9; i++) {
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

