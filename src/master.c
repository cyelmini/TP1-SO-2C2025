// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/mlib.h"
#include "include/utils.h"

int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "ERROR: No se ha especificado un jugador.");
		exit(EXIT_FAILURE);
	}

	game_t *game = NULL;
	game_sync *sync = NULL;
	unsigned short width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;
	int delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, seed = DEFAULT_SEED;
	unsigned int player_count = 0;
	char *view = DEFAULT_VIEW;
	char *players[MAX_PLAYERS];
	int pipe_fd[MAX_PLAYERS][2];

	handle_params(argc, argv, &width, &height, &delay, &timeout, &seed, &view, players, &player_count);
	global_player_count = player_count;
	global_width = width ;
	global_height = height ;
	global_pipe_fd = pipe_fd;

	// Instalar handler de señal
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

	printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\nnum_players: %u\n", width, height, delay,
		   timeout, seed, view == NULL ? "-" : view, player_count);
	for (unsigned int i = 0; i < player_count; i++) {
		printf("\t%s\n", players[i]);
	}
	sleep(3);

	game = (game_t *) open_and_map(SHM_GAME, O_CREAT | O_RDWR, sizeof(game_t) + width * height * sizeof(int),
										   PROT_READ | PROT_WRITE, MAP_SHARED);

	sync = (game_sync *) open_and_map(SHM_SYNC, O_CREAT | O_RDWR, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED);
	global_game=game;
	global_sync=sync;

	initialize_sems(sync);

	for (unsigned int i = 0; i < player_count; i++) {
		if (pipe(pipe_fd[i]) == -1) {
			fprintf(stderr, "pipe error");
			exit(EXIT_FAILURE);
		}
	}

	pid_t view_pid = initialize_game(game, width, height, player_count, seed, players, view, pipe_fd);

	if (view != NULL) {
		sem_post(&sync->printNeeded);
		sem_wait(&sync->printDone);
	}

	int max_fd = 0;
	for (unsigned int i = 0; i < player_count; i++) {
		if (pipe_fd[i][READ_END] > max_fd) {
			max_fd = pipe_fd[i][READ_END];
		}
	}

	playChompChamps(game, sync, pipe_fd, player_count, max_fd, delay, timeout, view);

	if (view != NULL) {
		sem_post(&sync->printNeeded);
		int status;
		waitpid(view_pid, &status, 0);
		printf("La vista terminó con código %d\n", WEXITSTATUS(status));
	}

	for (unsigned int i = 0; i < player_count; i++) {
		int status;
		waitpid(game->players[i].player_pid, &status, 0);
		printf("Jugador %u terminó con código %d, puntaje %u / %u / %u\n", i, WEXITSTATUS(status),
			   game->players[i].score, game->players[i].validMoves, game->players[i].invalidMoves);
		close(pipe_fd[i][READ_END]);
		close(pipe_fd[i][WRITE_END]);

	}
	
	calculate_winner(game, player_count);

	destroy_semaphones(sync);

	close_and_unmap(SHM_GAME, game, sizeof(game_t) + sizeof(int) * width * height, true);
	close_and_unmap(SHM_SYNC, sync, sizeof(game_sync), true);

	return 0;
}
