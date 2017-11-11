// TODO Implement OSWait (thread synchronization, events).

#include "kernel.h"
#define IMPLEMENTATION
#include "kernel.h"

void KernelInitilisation() {
	vfs.Initialise();
	deviceManager.Initialise();
	graphics.Initialise(); 
	windowManager.Initialise();

	KernelLog(LOG_INFO, "KernelInitilisation - Starting the executive...\n");
	char *executive = (char *) "/os/executive";
	scheduler.SpawnProcess(executive, CStringLength(executive));

	KernelLog(LOG_VERBOSE, "KernelInitilisation - Complete.\n");
	scheduler.TerminateThread(GetCurrentThread());
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
