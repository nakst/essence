// Timing problems:
// 	- Qemu's timer (sometimes) runs too slow

#include "kernel.h"
#define IMPLEMENTATION
#include "kernel.h"

void KernelInitilisation() {
	pmm.Initialise2();
	InitialiseObjectManager();
	vfs.Initialise();
	deviceManager.Initialise();
	graphics.Initialise(); 
	windowManager.Initialise();

	KernelLog(LOG_INFO, "KernelInitilisation - Starting the desktop...\n");
	char *desktop = (char *) "/os/desktop";
	desktopProcess = scheduler.SpawnProcess(desktop, CStringLength(desktop));

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
