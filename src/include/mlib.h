#ifndef MLIB_H
#define MLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10
#define DEFAULT_SEED time(NULL)
#define DEFAULT_VIEW NULL

#define MIN_PLAYERS 1
#define MAX_PLAYERS 9

void check_invalid(const char *c);
void handle_params(int argc, char *argv[], unsigned short *width, unsigned short *height, int *delay,int *timeout, int *seed, char **view_path, char **players, unsigned int *player_count);
void initialize_sems(game_sync *sync);
void initialize_board(unsigned short width, unsigned short height, int board[], int seed);
void initialize_players(game_t *game, char **players);
pid_t initialize_game(game_t *game, unsigned short width, unsigned short height, int player_count, int seed, char **players);

#endif // MLIB_H