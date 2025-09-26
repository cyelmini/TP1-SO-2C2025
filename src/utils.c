// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/utils.h"

void *open_and_map(const char *name, int oflags, size_t size, int prot, int flags) {
	int fd = shm_open(name, oflags, 0666);
	if (fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}

	if (oflags & O_CREAT) {
		if (ftruncate(fd, (off_t) size) == -1) {
			perror("ftruncate");
			close(fd);
			exit(EXIT_FAILURE);
		}
	}

	void *addr = mmap(NULL, size, prot, flags, fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		close(fd);
		exit(EXIT_FAILURE);
	}
	close(fd);
	return addr;
}

void close_and_unmap(char *name, void *addr, size_t size, bool unlink) {
	
	if (munmap(addr, size) == -1) {
		perror("munmap");
		exit(EXIT_FAILURE);
	}
	if (unlink) {
		if (shm_unlink(name) == -1) {
			perror("shm_unlink");
			exit(EXIT_FAILURE);
		}
	}
}

void enter_reader(game_sync *sync) {
	sem_wait(&sync->readersMutex);
	sync->readersCount++;
	if (sync->readersCount == 1) {
		sem_wait(&sync->gameMutex);
	}
	sem_post(&sync->readersMutex);
}

void exit_reader(game_sync *sync) {
	sem_wait(&sync->readersMutex);
	sync->readersCount--;
	if (sync->readersCount == 0) {
		sem_post(&sync->gameMutex);
	}
	sem_post(&sync->readersMutex);
}