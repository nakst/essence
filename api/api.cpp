#include "os.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC
#define STB_IMAGE_STATIC
#include "stb_image.h"

void Panic() {
	OSCrashProcess(OS_FATAL_ERROR_UNKNOWN);
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

extern "C" void OSInitialiseAPI() {
	// TODO Seed random number generator.

	void OSFPInitialise();
	OSFPInitialise();

	OSHeapInitialise();
	printMutex = OSCreateMutex();

	OSPrint("API initialised.\n");
}

#ifndef NO_START
extern "C" void _start() {
	OSInitialiseAPI();
	ProgramEntry();
	OSTerminateThread(OS_CURRENT_THREAD);
}
#endif
