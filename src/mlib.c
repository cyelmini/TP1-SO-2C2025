#include "include/mlib.h"

void check_invalid(const char *c) {
    if (c == NULL || strncmp(c, "-", 1) == 0) {
        fprintf(stderr, "ERROR: Argumento inválido\n");
        exit(EXIT_FAILURE);
    }
}

void handle_params(int argc, char *argv[], int *width, int *height, int *delay,int *timeout, int *seed, char **view_path, char **players, int *player_count){
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
                view_path = optarg;
                break;
            case 'p': 
                int index_player = optind - 1;
                if (index_player >= argc) {
                    fprintf(stderr, "ERROR: Debe especificar al menos un jugador después de -p");
                    exit(EXIT_FAILURE);
                }
                for (int j = index_player; j < argc; j++) {
                    if ((j - index_player) >= 9) {
                        fprintf(stderr, "ERROR: Máximo 9 jugadores");
                        exit(EXIT_FAILURE);
                    }
                    (*player_count)++;
                }
                break;
            default:
                fprintf(stderr, "ERROR: Opción no reconocida: -%c\n", optopt);
                exit(EXIT_FAILURE);
        }
    }
}
