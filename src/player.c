#include "include/utils.h"
#include <stdlib.h>
#include <time.h>
#include <sched.h>

static const int DIRS[8][2] = {
    { 0,-1}, // 0 ARRIBA
    { 1,-1}, // 1 ARRIBA A LA DERECHA
    { 1, 0}, // 2 DERECHA
    { 1, 1}, // 3 ABAJO A LA DERECHA
    { 0, 1}, // 4 ABAJO
    {-1, 1}, // 5 ABAJO A LA IZQUIERDA
    {-1, 0}, // 6 IZQUIERDA
    {-1,-1}, // 7 ARRIBA A LA IZQUIERDA
};

static int in_bounds(const game_t *game, int x, int y) {
    return (x >= 0 && x < game->width && y >= 0 && y < game->height);
}
 
static int is_valid_cell(const game_t *game, int x, int y) {
    if (!in_bounds(game, x, y)){
		return 0;
	} 

    if (game->board[y * game->width + x] <= 0){
		return 0;
	}

    for (unsigned int i = 0; i < game->playerCount; ++i) {
        const player_t * player = &game->players[i];
        if (!player->isBlocked && player->x == x && player->y == y) {
            return 0;
        }
    }
    return 1;
}

static int choose_move(const game_t *game, const player_t *me) {
    int best_dir = -1;
    int best_score = -1; 

    for (int dir = 0; dir < 8; ++dir) {
        int nx = me->x + DIRS[dir][0];
        int ny = me->y + DIRS[dir][1];
       
		if(is_valid_cell(game, nx, ny)){
			int score = game->board[ny * game->width + nx];
			if (score > best_score) {
				best_score = score;
				best_dir = dir;
			}
		}
    }
    return best_dir; 
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width  = atoi(argv[1]);
    int height = atoi(argv[2]);

    // Mapear memoria compartida
    game_t *game = (game_t *)open_and_map(SHM_GAME, O_RDONLY, sizeof(game_t) + sizeof(int) * width * height, PROT_READ, MAP_SHARED);
    game_sync *sync = (game_sync *)open_and_map(SHM_SYNC, O_RDWR, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED);

    // Identificar pid
    pid_t me_pid = getpid();
    int my_id = -1;
    for (unsigned int i = 0; i < game->playerCount; ++i) {
        if (game->players[i].player_pid == me_pid) { 
			my_id = (int)i; 
			break; 
		}
    }
    if (my_id < 0) {
        fprintf(stderr, "Jugador no encontrado en game->players (pid=%d)\n", me_pid);
        exit(EXIT_FAILURE);
    }

    while (!game->gameFinished && !game->players[my_id].isBlocked) {
        // Esperar turno
        sem_wait(&sync->playerTurn[my_id]);

        // Ver si el master quiere escribir
        sem_wait(&sync->masterMutex);
        sem_post(&sync->masterMutex);

        // Entrar como lector
        sem_wait(&sync->readersMutex);
        sync->readersCount++;
        if (sync->readersCount == 1) {
			sem_wait(&sync->gameMutex);
		}
        sem_post(&sync->readersMutex);

        player_t *me = &game->players[my_id];
        int dir = choose_move(game, me);

        // Salir como lector
        sem_wait(&sync->readersMutex);
        sync->readersCount--;
        if (sync->readersCount == 0) {
			sem_post(&sync->gameMutex);
		}
        sem_post(&sync->readersMutex);

        // Si no hay movimiento válido, termino
        if (dir < 0) {
			break;
		}

        // Enviar movimiento al master 
        unsigned char move = (unsigned char)dir;
        if (write(STDOUT_FILENO, &move, sizeof(move)) == -1) {
            perror("write");
            break;
        }
    }

    close_and_unmap(SHM_GAME, game, sizeof(game_t) + sizeof(int) * width * height, false);
    close_and_unmap(SHM_SYNC, sync, sizeof(game_sync), false);

    return 0;
}
