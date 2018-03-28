#include "utf8.h"

#ifndef CF
#define CF(x) OS ## x
#endif

size_t CF(CStringLength)(char *string) {
	if (!string) {
		return (size_t) -1;
	}

	size_t size = 0;

	while (true) {
		if (*string) {
			size++;
			string++;
		} else {
			return size;
		}
	}
}

size_t CF(CStringLength)(const char *string) {
	if (!string) {
		return (size_t) -1;
	}

	size_t size = 0;

	while (true) {
		if (*string) {
			size++;
			string++;
		} else {
			return size;
		}
	}
}

void CF(CopyMemory)(void *_destination, void *_source, size_t bytes) {
	if (!bytes) {
		return;
	}

	uint8_t *destination = (uint8_t *) _destination;
	uint8_t *source = (uint8_t *) _source;

#ifdef ARCH_X86_64
	while (bytes >= 16) {
		_mm_storeu_si128((__m128i *) destination, 
				_mm_loadu_si128((__m128i *) source));

		source += 16;
		destination += 16;
		bytes -= 16;
	}
#endif

	while (bytes >= 1) {
		((uint8_t *) destination)[0] = ((uint8_t *) source)[0];

		source += 1;
		destination += 1;
		bytes -= 1;
	}
}

void CF(CopyMemoryReverse)(void *_destination, void *_source, size_t bytes) {
	if (!bytes) {
		return;
	}

	uint8_t *destination = (uint8_t *) _destination;
	uint8_t *source = (uint8_t *) _source;

	destination += bytes - 1;
	source += bytes - 1;

	while (bytes >= 1) {
		((uint8_t *) destination)[0] = ((uint8_t *) source)[0];

		source -= 1;
		destination -= 1;
		bytes -= 1;
	}
}

void CF(ZeroMemory)(void *destination, size_t bytes) {
	if (!bytes) {
		return;
	}

	for (uintptr_t i = 0; i < bytes; i++) {
		((uint8_t *) destination)[i] = 0;
	}
}

int CF(CompareBytes)(void *a, void *b, size_t bytes) {
	if (!bytes) {
		return 0;
	}

	uint8_t *x = (uint8_t *) a;
	uint8_t *y = (uint8_t *) b;

	for (uintptr_t i = 0; i < bytes; i++) {
		if (x[i] < y[i]) {
			return -1;
		} else if (x[i] > y[i]) {
			return 1;
		}
	}

	return 0;
}

uint8_t CF(SumBytes)(uint8_t *source, size_t bytes) {
	if (!bytes) {
		return 0;
	}

	uint8_t total = 0;

	for (uintptr_t i = 0; i < bytes; i++) {
		total += source[i];
	}

	return total;
}

typedef void (*CF(PutCharacterCallback))(int character, void *data); 

void CF(_FormatString)(CF(PutCharacterCallback) callback, void *callbackData, const char *format, va_list arguments) {
	int c;

	char buffer[32];
	const char *hexChars = "0123456789ABCDEF";

	while ((c = utf8_value((char *) format))) {
		if (c == '%') {
			format = utf8_advance((char *) format);
			c = utf8_value((char *) format);

			switch (c) {
				case 'd': {
					long value = va_arg(arguments, long);
					if (value < 0) {
						callback('-', callbackData);
						value = -value;
					} else if (value == 0) {
						callback('0', callbackData);
					}
					int bp = 0;
					while (value) {
						buffer[bp++] = ('0' + (value % 10));
						value /= 10;
					}
					int cr = bp % 3;
					for (int i = bp - 1; i >= 0; i--, cr--) {
						if (!cr) {
							if (i != bp - 1) callback(',', callbackData);
							cr = 3;
						}

						callback(buffer[i], callbackData);
					}
				} break;

				case 'X': {
					uintptr_t value = va_arg(arguments, uintptr_t);
					callback(hexChars[(value & 0xF0) >> 4], callbackData);
					callback(hexChars[(value & 0xF)], callbackData);
				} break;

				case 'x': {
					uintptr_t value = va_arg(arguments, uintptr_t);
					callback('0', callbackData);
					callback('x', callbackData);
					int bp = 0;
					while (value) {
						buffer[bp++] = hexChars[value % 16];
						value /= 16;
					}
					int j = 0, k = 0;
					for (int i = 0; i < 16 - bp; i++) {
						callback('0', callbackData);
						j++;k++;if (k != 16 && j == 4) callback('_',callbackData);j&=3;
					}
					for (int i = bp - 1; i >= 0; i--) {
						callback(buffer[i], callbackData);
						j++;k++;if (k != 16 && j == 4) callback('_',callbackData);j&=3;
					}
				} break;

				case 'c': {
					callback(va_arg(arguments, int), callbackData);
				} break;

				case 's': {
					size_t length = va_arg(arguments, size_t);
					char *string = va_arg(arguments, char *);
					char *position = string;

					while (position < string + length) {
						callback(utf8_value(position), callbackData);
						position = utf8_advance(position);
					}
				} break;

				case 'z': {
					char *string = va_arg(arguments, char *);
					char *position = string;

					while (*position) {
						callback(utf8_value(position), callbackData);
						position = utf8_advance(position);
					}
				} break;

				case 'S': {
					size_t length = va_arg(arguments, size_t);
					uint16_t *string = va_arg(arguments, uint16_t *);
					uint16_t *position = string;

					while (position != string + (length >> 1)) {
						callback((position[0] >> 8) & 0xFF, callbackData);
						callback(position[0] & 0xFF, callbackData);
						position++;
					}
				} break;

				case 'F': {
					double number = va_arg(arguments, double);

					if (number < 0) {
						callback('-', callbackData);
						number = -number;
					}

					uint64_t integer = (uint64_t) number;
					uint64_t fractional = 0;

					const int max = 15;
					int o = 0;
					double m = 1, n;

					number -= integer;

					while (o < max) {
						n = number * m;
						fractional = (uint64_t) n;

						if ((double) fractional == n) {
							break;
						} else {
							m *= 10;
							o++;
						}
					}

					if (o == max) {
						o--;
					}

					int bp = 0;

					if (!integer) {
						callback('0', callbackData);
					}

					while (integer) {
						buffer[bp++] = (integer % 10) + '0';
						integer /= 10;
					}

					while (--bp != -1) {
						callback(buffer[bp], callbackData);
					}

					if (fractional) {
						callback('.', callbackData);

						bp = 0;

						while (fractional) {
							buffer[bp++] = (fractional % 10) + '0';
							fractional /= 10;
							o--;
						}

						while (o--) buffer[bp++] = '0';

						while (--bp != -1) {
							callback(buffer[bp], callbackData);
						}
					}
				} break;
			}
		} else {
			callback(c, callbackData);
		}

		format = utf8_advance((char *) format);
	}
}

typedef struct {
	char *buffer;
	size_t bytesRemaining;
} CF(FormatStringInformation);

void CF(FormatStringCallback)(int character, void *data) {
	CF(FormatStringInformation) *fsi = (CF(FormatStringInformation) *) data;

	if (!fsi->buffer) {
		// Just measure the length of the formatted string.
		return;
	}

	{
		char data[4];
		size_t bytes = utf8_encode(character, data);

		if (fsi->bytesRemaining < bytes) return;
		else {
			utf8_encode(character, fsi->buffer);
			fsi->buffer += bytes;
			fsi->bytesRemaining -= bytes;
		}
	}
}

#ifndef KERNEL
static OSHandle printMutex;

static char printBuffer[4096];
static uintptr_t printBufferPosition = 0;

void CF(PrintCallback)(int character, void *data) {
	(void) data;

	if (printBufferPosition >= 4090) {
		OSSyscall(OS_SYSCALL_PRINT, (uintptr_t) printBuffer, printBufferPosition, 0, 0);
		printBufferPosition = 0;
	}

	printBufferPosition += utf8_encode(character, printBuffer + printBufferPosition); 
}

void CF(Print)(const char *format, ...) {
	OSAcquireMutex(printMutex);
	printBufferPosition = 0;
	va_list arguments;
	va_start(arguments, format);
	CF(_FormatString)(CF(PrintCallback), nullptr, format, arguments);
	va_end(arguments);
	OSSyscall(OS_SYSCALL_PRINT, (uintptr_t) printBuffer, printBufferPosition, 0, 0);
	OSReleaseMutex(printMutex);
}

void CF(PrintDirect)(char *string, size_t stringLength) {
	OSSyscall(OS_SYSCALL_PRINT, (uintptr_t) string, stringLength, 0, 0);
}
#endif

size_t CF(FormatString)(char *buffer, size_t bufferLength, const char *format, ...) {
	CF(FormatStringInformation) fsi = {buffer, bufferLength};
	va_list arguments;
	va_start(arguments, format);
	CF(_FormatString)(CF(FormatStringCallback), &fsi, format, arguments);
	va_end(arguments);
	return bufferLength - fsi.bytesRemaining;
}

uint64_t osRandomByteSeed;

uint8_t CF(GetRandomByte)() {
	osRandomByteSeed = osRandomByteSeed * 214013 + 2531011;
	return (uint8_t) (osRandomByteSeed >> 16);
}

void OSSort(void *_base, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *argument) {
	if (nmemb <= 1) return;

	uint8_t *base = (uint8_t *) _base;
	uint8_t swap[size];

	intptr_t i = -1, j = nmemb;

	while (true) {
		while (compar(base + ++i * size, base, argument) < 0);
		while (compar(base + --j * size, base, argument) > 0);

		if (i >= j) break;

		CF(CopyMemory)(swap, base + i * size, size);
		CF(CopyMemory)(base + i * size, base + j * size, size);
		CF(CopyMemory)(base + j * size, swap, size);
	}

	OSSort(base, ++j, size, compar, argument);
	OSSort(base + j * size, nmemb - j, size, compar, argument);
}

#ifndef KERNEL
static int64_t ConvertCharacterToDigit(int character, int base) {
	int64_t result = -1;

	if (character >= '0' && character <= '9') {
		result = character - '0';
	} else if (character >= 'A' && character <= 'Z') {
		result = character - 'A' + 10;
	} else if (character >= 'a' && character <= 'z') {
		result = character - 'a' + 10;
	}

	if (result >= base) {
		result = -1;
	}

	return result;
}

void OSHelloWorld() {
	OSPrint("Hello, world!!\n");
}

void OSAssertionFailure() {
	OSPrint("Assertion failure.\n");
	while (true);
}
#include "cstdlib.c"
#endif
