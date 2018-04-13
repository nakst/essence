#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

struct Line {
	char thread[21];
	char mutex[21];
	int op;
};

struct Mutex {
	char n[21];
	uint8_t connections[1024 / 8];
};

struct Thread {
	char n[21];
};

Line lines[1048576];
size_t lineCount;

Mutex mutexes[1024];
size_t mutexCount;

Thread threads[1024];
size_t threadCount;

int main(int argc, char **argv) {
	FILE *log = fopen("out.txt", "rb");
	fseek(log, 0, SEEK_END);
	size_t fileSize = ftell(log);
	fseek(log, 0, SEEK_SET);
	char *buffer = (char *) malloc(fileSize);
	char *end = buffer + fileSize;
	fread(buffer, 1, fileSize, log);

	while (*buffer) {
		char *start = buffer;

		while (*buffer != '\n' && buffer < end) {
			buffer++;
		}

		*buffer = 0;
		buffer += 2;

		if (*start == '0') {
			memcpy(lines[lineCount].thread, start, 21);
			memcpy(lines[lineCount].mutex, start + 22, 21);
			lines[lineCount].op = start[44] - '0';
			lineCount++;
		}
	}

	// printf("found %d mutex operations\n", (int) lineCount);

	for (uintptr_t i = 0; i < lineCount; i++) {
		bool repeat = false;

		for (uintptr_t j = 0; j < mutexCount; j++) {
			if (0 == memcmp(lines[i].mutex, mutexes[j].n, 21)) {
				repeat = true;
				break;
			}
		}

		if (!repeat) {
			memcpy(mutexes[mutexCount].n, lines[i].mutex, 21);
			mutexCount++;
		}
	}

	// printf("found %d mutexes\n", (int) mutexCount);

	for (uintptr_t i = 0; i < lineCount; i++) {
		bool repeat = false;

		for (uintptr_t j = 0; j < threadCount; j++) {
			if (0 == memcmp(lines[i].thread, threads[j].n, 21)) {
				repeat = true;
				break;
			}
		}

		if (!repeat) {
			memcpy(threads[threadCount].n, lines[i].thread, 21);
			threadCount++;
		}
	}

	// printf("found %d threads\n", (int) threadCount);

	int maxX = 0;

	for (uintptr_t i = 0; i < threadCount; i++) {
		Thread thread = threads[i];
		// printf("---thread %.*s\n", 21, thread.n);
		int x = 0;

		int stack[16];

		for (uintptr_t j = 0; j < lineCount; j++) {
			if (memcmp(lines[j].thread, thread.n, 21)) {
				continue;
			}

#if 0
			for (int i = 0; i <= x; i++) {
				putchar(' ');
			}
#endif

			if (lines[j].op) {
				for (uintptr_t k = 0; k < mutexCount; k++) {
					if (0 == memcmp(lines[j].mutex, mutexes[k].n, 21)) {
						stack[x] = k;
					}
				}
				
				if (x) {
					// printf("%d->%d\n", stack[x], stack[x-1]);
					int id = stack[x - 1];
					mutexes[stack[x]].connections[id / 8] |= 1 << (id & 7);
				}

				// putchar(' ');
				// printf("acquire %.*s\n", 21, lines[j].mutex);
				x++;
				if (x > maxX) maxX = x;
			} else {
				x--;
				// printf("release %.*s\n", 21, lines[j].mutex);
			}
		}
	}

	// printf("maxX = %d\n", maxX);

	for (uintptr_t i = 0; i < mutexCount; i++) {
		for (uintptr_t j = i + 1; j < mutexCount; j++) {
			if (mutexes[i].connections[j % 8] & (1 << (j & 7))) {
				if (mutexes[j].connections[i % 8] & (1 << (i & 7))) {
					printf("deadlock on pair %.*s, %.*s\n", 21, mutexes[i].n, 21, mutexes[j].n);
				}
			}
		}
	}
}
