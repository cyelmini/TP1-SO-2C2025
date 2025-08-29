#include "include/utils.h"

void* open_and_map(const char *name, int oflag, )

int open_shared_memory(const char *name, int oflag, mode_t mode) {
	int fd = shm_open(name, oflag, mode);
	if (fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}
	return fd;
}

void *map_shared_memory(int fd, size_t size, int prot, int flags) {
	void *addr = mmap(NULL, size, prot, flags, fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	return addr;
}

void close_and_unmap(void *addr, size_t size, int fd) {
	munmap(addr, size);
	close(fd);
}