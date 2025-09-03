#include "./include/utils.h"

#define RESET "\x1B[0m"
#define BOLD "\x1B[1m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN "\x1B[36m"
#define ORANGE "\x1B[38;5;208m"
#define PURPLE "\x1B[38;5;129m"
#define WHITE "\x1B[37m"
#define BG_BLACK "\x1B[40m"
#define BG_GRAY "\x1B[48;5;240m"
#define BG_BRIGHT "\x1B[48;5;250m"

#define TL "┌"
#define TR "┐"
#define BL "└"
#define BR "┘"
#define HL "─"
#define VL "│"
#define TM "┬"
#define BM "┴"
#define LM "├"
#define RM "┤"
#define MM "┼"

#define CELLW 4

const char *colors[9] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, ORANGE, PURPLE, WHITE};
const char *bg_colors[9] = {"\x1B[41m", "\x1B[42m",		  "\x1B[43m",		"\x1B[44m", "\x1B[45m",
							"\x1B[46m", "\x1B[48;5;208m", "\x1B[48;5;129m", "\x1B[47m"};

static void print_board_border(int width, const char *left, const char *mid, const char *right, const char *fill) {
	printf("%s%s", BG_GRAY, left);
	for (int x = 0; x < width; x++) {
		for (int k = 0; k < CELLW; k++)
			printf("%s%s", BG_GRAY, fill);
		if (x < width - 1)
			printf("%s%s", BG_GRAY, mid);
	}
	printf("%s%s\n", BG_GRAY, right);
	printf(RESET);
}

static void print_cell(int cell, int x, int y, game_t *game) {
	if (cell > 0) {
		printf("%s %1d  %s", BG_BRIGHT, cell, RESET);
		return;
	}
	int idx = -cell;
	player_t pl = game->players[idx];
	int is_head = (pl.x == x && pl.y == y);
	if (is_head) {
		printf("%s %1d  %s", bg_colors[idx], idx, RESET);
	}
	else {
		printf("%s    %s", bg_colors[idx], RESET);
	}
}

static void print_players(game_t *game) {
	printf("\n--- Jugadores ---\n");
	for (unsigned int i = 0; i < game->playerCount; i++) {
		player_t *p = &game->players[i];
		printf("%sJugador %u%s (%s): Score=%u, Valid=%u, Invalid=%u, Pos=(%u,%u), %s\n", colors[i], i, RESET, p->name,
			   p->score, p->validMoves, p->invalidMoves, p->x, p->y, p->isBlocked ? "BLOCKED" : "OK");
	}
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);

	game_t *game = open_and_map(SHM_GAME, O_RDONLY, sizeof(game_t) + sizeof(int) * width * height, PROT_READ, MAP_SHARED);
	game_sync *sync = open_and_map(SHM_SYNC, O_RDWR, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED);

	

	// chequear despues si el gameFinished es necesario
	while (!game->gameFinished) {
		sem_wait(&sync->printNeeded);

		printf("\033[2J\033[H");
		print_board_border(width, TL, TM, TR, HL);

		for (int y = 0; y < height; y++) {
			printf("%s%s", BG_GRAY, VL);
			for (int x = 0; x < width; x++) {
				int cell = game->board[y * width + x];
				print_cell(cell, x, y, game);
				if (x < width - 1)
					printf("%s%s", BG_GRAY, VL);
			}
			printf("%s%s\n", BG_GRAY, VL);
			printf(RESET);
			if (y < height - 1)
				print_board_border(width, LM, MM, RM, HL);
		}

		print_board_border(width, BL, BM, BR, HL);
		print_players(game);
		fflush(stdout);

		sem_post(&sync->printDone);
	}

	close_and_unmap(SHM_GAME, game, sizeof(game_t) + sizeof(int) * width * height, 0);
	close_and_unmap(SHM_SYNC, sync, sizeof(game_sync), 0);

	return 0;
}
