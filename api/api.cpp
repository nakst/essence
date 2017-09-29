#include "os.h"

void Panic() {
	// TODO Temporary.
	OSPrint("OSPanic was called.\n");
	while (true);
}

#include "heap.cpp"
#include "gui.cpp"
#include "common.cpp"
#include "syscall.cpp"

extern "C" void ProgramEntry();

extern "C" void _start() {
	OSHeapInitialise();

	ProgramEntry();

	// TODO Exit the process.
}
