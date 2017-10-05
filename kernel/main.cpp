// TODO Reference counting for more kernel objects 
// 		- files, filesystems, mountpoints, devices, etc.

// TODO I don't think floats work.

// TODO Startup bug page faulting from trying to execute at 0?
// 		- I can't reproduce this.
// 		- Possibly related to the mutex bug?

// TODO Implement OSWait (thread synchronization, events).

#include "kernel.h"
#define IMPLEMENTATION
#include "kernel.h"

void KernelInitilisation() {
	graphics.Initialise();
	windowManager.Initialise();
	vfs.Initialise();
	deviceManager.Initialise();

	KernelLog(LOG_INFO, "KernelInitilisation - Spawning process for /os/test...\n");
	char *testProcessImage = (char *) "/os/test";
	scheduler.SpawnProcess(testProcessImage, CStringLength(testProcessImage));

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
	kernelVMM.Initialise();
	pmm.Initialise();
	scheduler.Initialise();
	acpi.Initialise(); // Initialises CPULocalStorage.
	scheduler.SpawnThread((uintptr_t) KernelInitilisation, 0, kernelProcess, false);
	KernelLog(LOG_VERBOSE, "Starting preemption...\n");
	scheduler.Start();

	KernelAPMain();
}
