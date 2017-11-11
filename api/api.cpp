#include "os.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC
#define STB_IMAGE_STATIC
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
	// TODO Seed random number generator.

	void OSFPInitialise();
	OSFPInitialise();

	OSHeapInitialise();
	printMutex = OSCreateMutex();

	ProgramEntry();
	OSTerminateThread(OS_CURRENT_THREAD);
}
