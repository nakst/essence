// TODO File API.

// TODO Implement OSWait (thread synchronization, events).

// TODO DoSyscall not terminatable bug,

#include "kernel.h"
#define IMPLEMENTATION
#include "kernel.h"

void KernelInitilisation() {
	graphics.Initialise();
	windowManager.Initialise();
	vfs.Initialise();
	deviceManager.Initialise();

	KernelLog(LOG_INFO, "KernelInitilisation - Starting the shell...\n");
	char *shellExecutable = (char *) "/os/shell";
	scheduler.SpawnProcess(shellExecutable, CStringLength(shellExecutable));

	KernelLog(LOG_VERBOSE, "KernelInitilisation - Complete.\n");
	scheduler.TerminateThread(ProcessorGetLocalStorage()->currentThread);
}

extern "C" void KernelAPMain() {
	while (!scheduler.started);
	scheduler.InitialiseAP();

	NextTimer(20);

	// The InitialiseAP call makes this an idle thread.
	// Therefore there are always enough idle threads for all processors.
	// TODO Enter low power state?
	ProcessorIdle();
}

extern "C" void KernelMain() {
	Print("---------------------------\n");
	kernelVMM.Initialise();
	pmm.Initialise();
	scheduler.Initialise();
	acpi.Initialise(); // Initialises CPULocalStorage.
	scheduler.SpawnThread((uintptr_t) KernelInitilisation, 0, kernelProcess, false);
	KernelLog(LOG_VERBOSE, "Starting preemption...\n");
	scheduler.Start();

	KernelAPMain();
}
