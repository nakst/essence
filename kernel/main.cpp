// TODO Reference counting for more kernel objects 
// 		- files, filesystems, mountpoints, devices, etc.

// TODO Implement OSWait (thread synchronization, events).

// TODO Switch from pools to heap?

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

	{
		SharedMemoryRegion *region = sharedMemoryManager.CreateSharedMemory(1000000);
		uint8_t *memory1 = (uint8_t *) kernelVMM.Allocate(1000000, vmmMapLazy, vmmRegionShared, 0, VMM_REGION_FLAG_CACHABLE, region);
		uint8_t *memory2 = (uint8_t *) kernelVMM.Allocate(1000000, vmmMapLazy, vmmRegionShared, 0, VMM_REGION_FLAG_CACHABLE, region);

		for (int i = 0; i < 1000000; i++) {
			memory1[i] = (uint8_t) (i * i);

			if (memory2[i] != memory1[i]) {
				KernelPanic("Shared memory test failed at %d.\n", i);
			}
		}
	}

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
