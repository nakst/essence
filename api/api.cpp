#include "os.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_image.h"

void Panic() {
	// TODO Temporary.
	OSPrint("OSPanic was called.\n");
	while (true);
}

#ifndef CF
#define CF(x) OS ## x
#endif

#include "utf8.h"
#include "heap.cpp"
#include "font.cpp"
#include "gui.cpp"
#include "common.cpp"
#include "syscall.cpp"

extern "C" void ProgramEntry();

extern "C" void _start() {
	void OSFPInitialise();
	OSFPInitialise();

	OSHeapInitialise();
	printMutex = OSCreateMutex();

	ProgramEntry();

	// TODO Exit the process.
	OSPrint("Program executed successfully!\n");
	while (true);
}
