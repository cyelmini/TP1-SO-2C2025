#ifndef UTILS_H
#define UTILS_H

#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SHM_GAME "/game_state"
#define SHM_SYNC "/game_sync"

typedef struct {
	char name[16];			   // Nombre del jugador
	unsigned int score;		   // Puntaje
	unsigned int invalidMoves; // Cantidad de solicitudes de movimientos inválidas realizadas
	unsigned int validMoves;   // Cantidad de solicitudes de movimientos válidas realizadas
	unsigned short x, y;	   // Coordenadas x e y en el tablero
	pid_t player_pid;		   // Identificador de proceso
	bool isBlocked;			   // Indica si el jugador está bloqueado
} player_t;

typedef struct {
	unsigned short width;	  // Ancho del tablero
	unsigned short height;	  // Alto del tablero
	unsigned int playerCount; // Cantidad de jugadores
	player_t players[9];	  // Lista de jugadores
	bool gameFinished;		  // Indica si el juego se ha terminado
	int board[];			  // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} game_t;

typedef struct {
	sem_t printNeeded;		   // El máster le indica a la vista que hay cambios por imprimir
	sem_t printDone;		   // La vista le indica al máster que terminó de imprimir
	sem_t masterMutex;		   // Mutex para evitar inanición del máster al acceder al estado
	sem_t gameMutex;		   // Mutex para el estado del juego
	sem_t readersMutex;		   // Mutex para la siguiente variable
	unsigned int readersCount; // Cantidad de jugadores leyendo el estado
	sem_t playerTurn[9];	   // Le indican a cada jugador que puede enviar 1 movimiento
} game_sync;

void *open_and_map(const char *name, int oflags, mode_t mode, size_t size, int prot, int flags);
void close_and_unmap(char *name, void *addr, size_t size, bool unlink);

#endif // UTILS_H