// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/mlib.h"
#include "include/utils.h"
#include <signal.h>

// Variables globales para acceso en cleanup
static game_t *game_ptr = NULL;
static game_sync *sync_ptr = NULL;
static unsigned short g_width = 0;
static unsigned short g_height = 0;
static int g_pipe_fd[MAX_PLAYERS][2];
static unsigned int g_player_count = 0;

static void cleanup(int signo) {
	for (unsigned int i = 0; i < g_player_count; i++) {
		close(g_pipe_fd[i][0]);
		close(g_pipe_fd[i][1]);
	}
	destroy_semaphones(sync_ptr);

	close_and_unmap(SHM_GAME, game_ptr, sizeof(game_t) + sizeof(int) * g_width * g_height, true);
	close_and_unmap(SHM_SYNC, sync_ptr, sizeof(game_sync), true);

	fprintf(stderr, "\nEl programa termino con la señal %d.\n", signo);
	_exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	check_arguments(argc);

	// Valores por defecto
	unsigned short width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;
	int delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, seed = DEFAULT_SEED;
	unsigned int player_count = 0;
	char *view = DEFAULT_VIEW;
	char *players[MAX_PLAYERS];
	int pipe_fd[MAX_PLAYERS][2];

	handle_params(argc, argv, &width, &height, &delay, &timeout, &seed, &view, players, &player_count);
	
	// Print de la configuracion
	printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\nnum_players: %u\n", width, height, delay,
		   timeout, seed, view == NULL ? "-" : view, player_count);
	for (unsigned int i = 0; i < player_count; i++) {
		printf("\t%s\n", players[i]);
	}
	sleep(3);

	game_t *game = (game_t *) open_and_map(SHM_GAME, O_CREAT | O_RDWR, sizeof(game_t) + width * height * sizeof(int),
										   PROT_READ | PROT_WRITE, MAP_SHARED);
	game_sync *sync =
		(game_sync *) open_and_map(SHM_SYNC, O_CREAT | O_RDWR, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED);

	initialize_sems(sync);
	
	// Inicializar variables globales para cleanup
	game_ptr = game;
	sync_ptr = sync;
	g_width = width;
	g_height = height;
	g_player_count = player_count;

	// Registrar cleanup con signal
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	// Inicializar pipes globales
	for (unsigned int i = 0; i < player_count; i++) {
		if (pipe(pipe_fd[i]) == -1) {
			fprintf(stderr, "pipe error");
			exit(EXIT_FAILURE);
		}
		g_pipe_fd[i][0] = pipe_fd[i][0];
		g_pipe_fd[i][1] = pipe_fd[i][1];
	}

	pid_t view_pid = initialize_game(game, width, height, player_count, seed, players, view, pipe_fd);

	sync_view(view, sync, delay);

	int max_fd = find_max_fd(pipe_fd, player_count);

	playChompChamps(game, sync, pipe_fd, player_count, max_fd, delay, timeout, view);
	
	signal_all_players(sync, player_count);

	wait_for_view(view, view_pid, sync);
	wait_for_players(game, player_count, pipe_fd);

	calculate_winner(game, player_count);

	destroy_semaphones(sync);

	close_and_unmap(SHM_GAME, game, sizeof(game_t) + sizeof(int) * width * height, true);
	close_and_unmap(SHM_SYNC, sync, sizeof(game_sync), true);

	game_ptr = NULL;
	sync_ptr = NULL;
	return 0;
}
