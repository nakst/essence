// Timing problems:
// 	- Qemu's timer (sometimes) runs too slow on SMP.

#include "kernel.h"
#define IMPLEMENTATION
#include "kernel.h"

void KernelInitialisation() {
	pmm.Initialise2();
	InitialiseObjectManager();
	graphics.Initialise(); 
	vfs.Initialise();
	deviceManager.Initialise();
	windowManager.Initialise();

	char *desktop = (char *) "/os/desktop";
	desktopProcess = scheduler.SpawnProcess(desktop, CStringLength(desktop));

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
	memoryManagerVMM.Initialise();
	pmm.Initialise();
	scheduler.Initialise();
	acpi.Initialise(); // Initialises CPULocalStorage.
	scheduler.SpawnThread((uintptr_t) KernelInitialisation, 0, kernelProcess, false);
	KernelLog(LOG_VERBOSE, "Starting preemption...\n");
	scheduler.Start();

	KernelAPMain();
}
