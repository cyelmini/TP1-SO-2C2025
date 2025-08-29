#include "include/utils.h"

void *open_and_map(const char *name, int oflags, mode_t mode, size_t size, int prot, int flags) {
	int fd = shm_open(name, oflags, mode);
	if (fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}

	void *addr = mmap(NULL, size, prot, flags, fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

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