#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include "./utils/utils.h"

#define SHM_GAME "/game_state"
#define SHM_SYNC "/game_sync"

#define RESET   "\x1B[0m"
#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"
#define ORANGE  "\x1B[38;5;208m"
#define PURPLE  "\x1B[38;5;129m"
#define WHITE   "\x1B[37m"

const char * colors[9] = {
    RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, ORANGE, PURPLE, WHITE
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    // --- Conectar memoria compartida ---
    int fd_game = shm_open(SHM_GAME, O_RDONLY, 0);
    if (fd_game == -1) { 
        perror("shm_open game"); 
        exit(EXIT_FAILURE); 
    }

    size_t game_size = sizeof(game_t) + sizeof(int) * width * height;
    game_t *game = mmap(NULL, game_size, PROT_READ, MAP_SHARED, fd_game, 0);
    if (game == MAP_FAILED) { 
        perror("mmap game"); 
        exit(EXIT_FAILURE); 
    }

    int fd_sync = shm_open(SHM_SYNC, O_RDWR, 0);
    if (fd_sync == -1) { 
        perror("shm_open sync"); 
        exit(EXIT_FAILURE); }

    game_sync *sync = mmap(NULL, sizeof(game_sync),PROT_READ | PROT_WRITE, MAP_SHARED, fd_sync, 0);
    if (sync == MAP_FAILED) { 
        perror("mmap sync"); 
        exit(EXIT_FAILURE); }

    while (!game->gameFinished) {
        sem_wait(&sync->printNeeded);

        // imprimir tablero
        printf("\033[2J\033[H"); // limpiar pantalla
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int cell = game->board[y * width + x];
                if (cell > 0) {
                    printf(" %d ", cell);
                } else {
                    int idx = -cell;
                    player_t pl = game->players[idx];
                    if (pl.x == x && pl.y == y)
                        printf("%s██%s", colors[idx], RESET);
                    else
                        printf("%s⯀ %s", colors[idx], RESET);
                }
            }
            printf("\n");
        }

        // imprimir estado jugadores
        printf("\n--- Jugadores ---\n");
        for (unsigned int i = 0; i < game->playerCount; i++) {
            player_t *p = &game->players[i];
            printf("%sJugador %u%s (%s): Score=%u, Valid=%u, Invalid=%u, Pos=(%u,%u), %s\n",
                   colors[i], i, RESET, p->name, p->score,
                   p->validMoves, p->invalidMoves,
                   p->x, p->y, p->isBlocked ? "BLOCKED" : "OK");
        }
        fflush(stdout);

        sem_post(&sync->printDone);
    }

    munmap(game, game_size);
    munmap(sync, sizeof(game_sync));
    close(fd_game);
    close(fd_sync);

    return 0;
}
