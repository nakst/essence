// TODO Priority inversion
// TODO Yield on mutex/spinlock release and event set

#ifndef IMPLEMENTATION

void CloseHandleToThread(void *_thread);
void CloseHandleToProcess(void *_thread);
void KillThread(void *_thread);

void RegisterAsyncTask(AsyncTaskCallback callback, void *argument, struct Process *targetProcess, bool needed /*If false, the task may not be registered if there are many queued tasks.*/);

struct Semaphore {
	void Take(uintptr_t units = 1);
	void Return(uintptr_t units = 1);
	void Set(uintptr_t units = 1);

	Event available;
	Mutex mutex;
	volatile uintptr_t units;
};

struct Timer {
	void Set(uint64_t triggerInMs, bool autoReset);
	void Remove();

	Event event;
	LinkedItem<Timer> item;
	uint64_t triggerTimeMs;
};

struct InterruptContext {
#ifdef ARCH_X86_64
	uint64_t cr2, ds;
	uint8_t  fxsave[512 + 16];
	uint64_t _check, cr8, r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t interruptNumber, errorCode;
	uint64_t rip, cs, flags, rsp, ss;
#endif
};

enum ThreadState {
	THREAD_ACTIVE,			// An active thread. `executing` determines if it executing.
	THREAD_WAITING_MUTEX,		// Waiting for a mutex to be released.
	THREAD_WAITING_EVENT,		// Waiting for a event to be notified.
	THREAD_TERMINATED,		// The thread has been terminated. It will be deallocated when all handles are closed.
					// 	I believe this is called a "zombie thread" in UNIX terminology.
};

enum ThreadType {
	THREAD_NORMAL,			// A normal thread.
	THREAD_IDLE,			// The CPU's idle thread.
	THREAD_ASYNC_TASK,		// A thread that processes the kernel's asynchronous tasks.
};

enum ThreadTerminatableState {
	THREAD_INVALID_TS,
	THREAD_TERMINATABLE,		// The thread is currently executing user code.
	THREAD_IN_SYSCALL,		// The thread is currently executing kernel code from a system call.
					// It cannot be terminated until it returns from the system call.
	THREAD_USER_BLOCK_REQUEST,	// The thread is sleeping because of a user system call to sleep.
					// It can be unblocked, and then terminated when it returns from the system call.
};

struct Thread {
	LinkedItem<Thread> item[OS_MAX_WAIT_COUNT];	// Entry in relevent thread queue or blockedThreads list.
	LinkedItem<Thread> allItem; 			// Entry in the allThreads list.
	LinkedItem<Thread> processItem; 		// Entry in the process's list of threads.

	struct Process *process;

	uintptr_t id;
	uintptr_t timeSlices;

	volatile ThreadState state;
	volatile ThreadTerminatableState terminatableState;
	volatile bool executing;
	volatile bool terminating; // Set when a request to terminate the thread has been registered.
	volatile bool paused;	   // Set to pause a thread, usually when it has crashed or being debugged. The scheduler will skip threads marked as paused when deciding what to run.

	int executingProcessorID;

	Mutex *volatile blockingMutex;
	Event *volatile blockingEvents[OS_MAX_WAIT_COUNT];
	volatile size_t blockingEventCount;

	Event killedEvent;

	uintptr_t userStackBase;
	uintptr_t kernelStackBase;

	uintptr_t kernelStack;
	bool isKernelThread;

	ThreadType type;

	volatile size_t handles;

	// If the type of the thread is THREAD_ASYNC_TASK,
	// then this is the virtual address space that should be loaded
	// when the task is being executed.
	VirtualAddressSpace *volatile asyncTempAddressSpace;

	InterruptContext *interruptContext;  // TODO Store the userland interrupt context instead?
	uintptr_t lastKnownExecutionAddress; // For debugging.

	volatile bool receivedYieldIPI;
};

struct MessageQueue {
	bool SendMessage(OSMessage &message); // Returns false if the message queue is full.
	bool GetMessage(OSMessage &message);

#define MESSAGE_QUEUE_MAX_LENGTH 4096
	OSMessage *messages;
	size_t count, allocated;

	uintptr_t mouseMovedMessage; // Index + 1, 0 indicates none.

	Mutex mutex;
	Event notEmpty;
};

struct Process {
	MessageQueue messageQueue;

	LinkedItem<Process> allItem;
	LinkedList<Thread> threads;

	VMM *vmm;
	VMM _vmm;

	OSCrashReason crashReason;

	char *executablePath;
	size_t executablePathLength;
	void *creationArgument;

	uintptr_t id;
	volatile size_t handles;

#define PROCESS_EXECUTABLE_NOT_LOADED 0
#define PROCESS_EXECUTABLE_FAILED_TO_LOAD 1
#define PROCESS_EXECUTABLE_LOADED 2
	uintptr_t executableState;
	Event executableLoadAttemptComplete;
	Thread *executableMainThread;

	Mutex crashMutex;

	HandleTable handleTable;

	Event killedEvent;
	bool allThreadsTerminated;
};

Process *kernelProcess;

struct Scheduler {
	void Start();
	void Initialise();
	void InitialiseAP();
	void CreateProcessorThreads();
	void Yield(InterruptContext *context);

	Thread *SpawnThread(uintptr_t startAddress, uintptr_t argument, Process *process, bool userland, bool addToActiveList = true);
	void TerminateThread(Thread *thread, bool lockAlreadyAcquired = false);
	void RemoveThread(Thread *thread); // Do not call. Use TerminateThread/CloseHandleToObject.
	void PauseThread(Thread *thread, bool resume /*true to resume, false to pause*/, bool lockAlreadyAcquired = false);

	Process *SpawnProcess(char *imagePath, size_t imagePathLength, bool kernelProcess = false, void *argument = nullptr);
	void TerminateProcess(Process *process);
	void RemoveProcess(Process *process); // Do not call. Use TerminateProcess/CloseHandleToObject.
	void PauseProcess(Process *process, bool resume);

	void CrashProcess(Process *process, OSCrashReason &reason);

	void AddActiveThread(Thread *thread, bool start /*Put it at the start*/);	// Add an active thread into the queue.
	void InsertNewThread(Thread *thread, bool addToActiveList, Process *owner); 	// Used during thread creation.

	void WaitMutex(Mutex *mutex);
	uintptr_t WaitEvents(Event **events, size_t count); // Returns index of notified object.
	void NotifyObject(LinkedList<Thread> *blockedThreads, bool schedulerAlreadyLocked = false, bool unblockAll = false);
	void UnblockThread(Thread *unblockedThread);

	Pool threadPool, processPool;
	LinkedList<Thread>  activeThreads, pausedThreads;
	LinkedList<Timer>   activeTimers;
	LinkedList<Thread>  allThreads;
	LinkedList<Process> allProcesses;
	Spinlock lock;

	uintptr_t nextThreadID;
	uintptr_t nextProcessID;
	uintptr_t processors;

	bool initialised;
	volatile bool started;

	uint64_t timeMs;

	struct CPULocalStorage *localStorage[MAX_PROCESSORS];

	bool panic;
};

extern Scheduler scheduler;

#endif

#ifdef IMPLEMENTATION

Scheduler scheduler;

int temp;

void Spinlock::Acquire() {
	if (scheduler.panic) return;

	bool _interruptsEnabled = ProcessorAreInterruptsEnabled();
	ProcessorDisableInterrupts();

	CPULocalStorage *storage = GetLocalStorage();

	if (storage && storage->currentThread && owner && owner == storage->currentThread) {
		KernelPanic("Spinlock::Acquire - Attempt to acquire a spinlock owned by the current thread (%x/%x, CPU: %d/%d).\nAcquired at %x.\n", 
				storage->currentThread, owner, storage->processorID, ownerCPU, acquireAddress);
	}

	if (storage) {
		storage->spinlockCount++;
		temp++;
	}

	while (__sync_val_compare_and_swap(&state, 0, 1));
	__sync_synchronize();

	interruptsEnabled = _interruptsEnabled;

	if (storage) {
		owner = storage->currentThread;
		ownerCPU = storage->processorID;
	} else {
		// Because spinlocks can be accessed very early on in initialisation there may not be
		// a CPULocalStorage available for the current processor. Therefore, just set this field to nullptr.

		owner = nullptr;
	}

	acquireAddress = (uintptr_t) __builtin_return_address(0);
}

void Spinlock::Release(bool force) {
	if (scheduler.panic) return;

	CPULocalStorage *storage = GetLocalStorage();

	if (storage) {
		storage->spinlockCount--;
		temp--;
	}

	if (!force) {
		AssertLocked();
	}

	owner = nullptr;
	state = 0;

	if (interruptsEnabled) ProcessorEnableInterrupts();

	releaseAddress = (uintptr_t) __builtin_return_address(0);
}

void Spinlock::AssertLocked() {
	if (scheduler.panic) return;

	CPULocalStorage *storage = GetLocalStorage();

	if (!state || ProcessorAreInterruptsEnabled() 
			|| (storage && owner != storage->currentThread)) {
		KernelPanic("Spinlock::AssertLocked - Spinlock not correctly acquired\n"
				"Return address = %x.\n"
				"state = %d, ProcessorAreInterruptsEnabled() = %d, owner = %x\n",
				__builtin_return_address(0), state, 
				ProcessorAreInterruptsEnabled(), owner);
	}
}

void Scheduler::AddActiveThread(Thread *thread, bool start) {
	if (thread->type == THREAD_ASYNC_TASK) {
		// An asynchronous task thread was unblocked.
		// It will be run immediately, so there's no need to add it to the active thread list.
		return;
	}
	
	lock.AssertLocked();

	if (thread->state != THREAD_ACTIVE) {
		KernelPanic("Scheduler::AddActiveThread - Thread %d not active\n", thread->id);
	} else if (thread->executing) {
		KernelPanic("Scheduler::AddActiveThread - Thread %d executing\n", thread->id);
	} else if (thread->type != THREAD_NORMAL) {
		KernelPanic("Scheduler::AddActiveThread - Thread %d has type %d\n", thread->id, thread->type);
	}

	if (thread->paused) {
		// The thread is paused, so we can put it into the paused queue until it is resumed.
		pausedThreads.InsertStart(&thread->item[0]);
	} else {
		if (start) {
			activeThreads.InsertStart(&thread->item[0]);
		} else {
			activeThreads.InsertEnd(&thread->item[0]);
		}
	}
}

void Scheduler::InsertNewThread(Thread *thread, bool addToActiveList, Process *owner) {
	lock.Acquire();
	Defer(lock.Release());

	// New threads are initialised here.
	thread->id = nextThreadID++;
	thread->process = owner;

	owner->handles++; 	// Each thread owns a handles to the owner process.
				// This makes sure the process isn't destroyed before all its threads have been destroyed.

	thread->processItem.thisItem = thread;
	owner->threads.InsertEnd(&thread->processItem);

	// KernelLog(LOG_VERBOSE, "Create thread ID %d, type %d, owner process %d\n", thread->id, thread->type, owner->id);

	for (uintptr_t i = 0; i < OS_MAX_WAIT_COUNT; i++) {
		thread->item[i].thisItem = thread;
	}

	thread->allItem.thisItem = thread;

	if (addToActiveList) {
		// Add the thread to the start of the active thread list to make sure that it runs immediately.
		AddActiveThread(thread, true);
	} else {
		// Some threads (such as idle threads) do this themselves.
	}

	allThreads.InsertStart(&thread->allItem);
}

Thread *Scheduler::SpawnThread(uintptr_t startAddress, uintptr_t argument, Process *process, bool userland, bool addToActiveThreads) {
	Thread *thread = (Thread *) threadPool.Add();
	// KernelLog(LOG_VERBOSE, "Created thread, %x to start at %x\n", thread, startAddress);
	thread->isKernelThread = !userland;

	// 2 handles to the thread:
	// 	One for spawning the thread, 
	// 	and the other for remaining during the thread's life.
	thread->handles = 2;

	// Allocate the thread's stacks.
	uintptr_t kernelStackSize = userland ? 0x4000 : 0x10000;
	uintptr_t userStackSize = userland ? 0x100000 : 0x10000;
	uintptr_t stack, kernelStack = (uintptr_t) kernelVMM.Allocate("KernStack", kernelStackSize, VMM_MAP_ALL);

	if (userland) {
		stack = (uintptr_t) process->vmm->Allocate("UserStack", userStackSize, VMM_MAP_LAZY);
	} else {
		stack = kernelStack;
	}

	// KernelLog(LOG_VERBOSE, "Spawning thread with stacks (k,u): %x->%x, %x->%x\n", kernelStack, kernelStack + kernelStackSize, stack, stack + userStackSize);

	thread->kernelStackBase = kernelStack;
	thread->userStackBase = userland ? stack : 0;

	thread->terminatableState = userland ? THREAD_TERMINATABLE : THREAD_IN_SYSCALL;
	// KernelLog(LOG_VERBOSE, "Thread %x terminatable = %d (creation)\n", thread, thread->terminatableState);

#ifdef ARCH_X86_64
	InterruptContext *context = ((InterruptContext *) (kernelStack + kernelStackSize - 8)) - 1;
	thread->interruptContext = context;
	thread->kernelStack = kernelStack + kernelStackSize - 8;

	if (userland) {
		context->cs = 0x5B;
		context->ds = 0x63;
		context->ss = 0x63;
	} else {
		context->cs = 0x48;
		context->ds = 0x50;
		context->ss = 0x50;
	}

	context->_check = 0x123456789ABCDEF; // Stack corruption detection.
	context->flags = 1 << 9; // Interrupt flag
	context->rip = startAddress;
	context->rsp = stack + userStackSize - 8; // The stack should be 16-byte aligned before the call instruction.
	context->rdi = argument;
#endif

	// KernelLog(LOG_VERBOSE, "Starting off new thread %x at %x\n", thread, startAddress);

	InsertNewThread(thread, addToActiveThreads, process);

	return thread;
}

void Scheduler::TerminateProcess(Process *process) {
	scheduler.lock.Acquire();
	Defer(scheduler.lock.Release());

	Thread *currentThread = GetCurrentThread();
	bool isCurrentProcess = process == currentThread->process;
	bool foundCurrentThread = false;

	LinkedItem<Thread> *thread = process->threads.firstItem;

	while (thread) {
		Thread *threadObject = thread->thisItem;
		thread = thread->nextItem;

		if (threadObject != currentThread) {
			TerminateThread(threadObject, true);
		} else if (isCurrentProcess) {
			foundCurrentThread = true;
		} else {
			KernelPanic("Scheduler::TerminateProcess - Found current thread in the wrong process?!\n");
		}
	}

	if (!foundCurrentThread && isCurrentProcess) {
		KernelPanic("Scheduler::TerminateProcess - Could not find current thread in the current process?!\n");
	} else if (isCurrentProcess) {
		TerminateThread(currentThread, true);
	}
}

void Scheduler::TerminateThread(Thread *thread, bool lockAlreadyAcquired) {
	if (!lockAlreadyAcquired) {
		scheduler.lock.Acquire();
	}

	if (thread->terminating) {
		return;
	}

	thread->terminating = true;

	if (thread == GetCurrentThread()) {
		thread->terminatableState = THREAD_TERMINATABLE;
		// KernelLog(LOG_VERBOSE, "Terminating current thread %x, so making THREAD_TERMINATABLE\n", thread);
		scheduler.lock.Release();

		// We cannot return to the previous function as it expects to be killed.
		ProcessorFakeTimerInterrupt();
		KernelPanic("Scheduler::TerminateThread - ProcessorFakeTimerInterrupt returned.\n");
	} else {
		if (thread->terminatableState == THREAD_TERMINATABLE) {
			if (thread->executing) {
				// The thread is executing, so the next time it tries to make a system call or
				// is pre-empted, it will be terminated.
				if (!lockAlreadyAcquired) scheduler.lock.Release();
			} else {
				if (thread->state != THREAD_ACTIVE) {
					KernelPanic("Scheduler::TerminateThread - Terminatable thread non-active.\n");
				}

				// The thread is terminatable and it isn't executing.
				// Remove it from its queue, and then remove the thread.
				if (!thread->paused) 	activeThreads.Remove(&thread->item[0]);
				else			pausedThreads.Remove(&thread->item[0]);
				RegisterAsyncTask(KillThread, thread, thread->process, true);
				if (!lockAlreadyAcquired) scheduler.lock.Release();
			}
		} else if (thread->terminatableState == THREAD_USER_BLOCK_REQUEST) {
			if (thread->executing) {
				// The mutex and event waiting code is designed to recognise when a thread is in this state,
				// and exit to the system call handler immediately.
				// If the thread however is pre-empted while in a blocked state before this code can execute,
				// Scheduler::Yield will automatically force the thread to be active again.
			} else {
				// Unblock the thread.
				// See comment above.
				UnblockThread(thread);
			}

			if (!lockAlreadyAcquired) scheduler.lock.Release();
		} else {
			// The thread is executing kernel code.
			// Therefore, we can't simply terminate the thread.
			// The thread will set its state to THREAD_TERMINATABLE whenever it can be terminated.
			if (!lockAlreadyAcquired) scheduler.lock.Release();
		}
	}
}

void Scheduler::Start() {
	if (!initialised) {
		KernelPanic("Scheduler::Start - Attempt to start scheduler before it has been initialised.\n");
	} else if (started) {
		KernelPanic("Scheduler::Start - Attempt to start scheduler multiple times.\n");
	} else {
		started = true;
	}
}

void NewProcess() {
	Process *thisProcess = GetCurrentThread()->process;

	// TODO Shared memory with executables.
	uintptr_t processStartAddress = LoadELF(thisProcess->executablePath, thisProcess->executablePathLength);

	if (processStartAddress) {
		thisProcess->executableState = PROCESS_EXECUTABLE_LOADED;
		thisProcess->executableMainThread = scheduler.SpawnThread(processStartAddress, 0, thisProcess, true);
	} else {
		thisProcess->executableState = PROCESS_EXECUTABLE_FAILED_TO_LOAD;
		KernelPanic("NewProcess - Could not start a new process.\n");
	}

	KernelLog(LOG_VERBOSE, "Created process %d %x, %s.\n", thisProcess->id, thisProcess, thisProcess->executablePathLength, thisProcess->executablePath);

	thisProcess->executableLoadAttemptComplete.Set();
	scheduler.TerminateThread(GetCurrentThread());
}

Process *Scheduler::SpawnProcess(char *imagePath, size_t imagePathLength, bool kernelProcess, void *argument) {
	// Print("Creating process, %s...\n", imagePathLength, imagePath);

	// Process initilisation.
	Process *process = (Process *) processPool.Add();
	process->allItem.thisItem = process;
	process->vmm = &process->_vmm;
	process->handles = 1;
	process->creationArgument = argument;

	if (!kernelProcess) {
		process->vmm->Initialise();
	}

	CopyMemory((process->executablePath = (char *) OSHeapAllocate(imagePathLength, false)), imagePath, imagePathLength);
	process->executablePathLength = imagePathLength;

	process->handleTable.process = process;

	lock.Acquire();

	process->id = nextProcessID++;
	allProcesses.InsertEnd(&process->allItem);

	lock.Release();

	if (!kernelProcess) {
		Thread *newProcessThread = SpawnThread((uintptr_t) NewProcess, 0, process, false);
		CloseHandleToObject(newProcessThread, KERNEL_OBJECT_THREAD);
		process->executableLoadAttemptComplete.Wait(OS_WAIT_NO_TIMEOUT);

		if (process->executableState == PROCESS_EXECUTABLE_FAILED_TO_LOAD) {
			KernelLog(LOG_VERBOSE, "Executable failed to load, closing handle to process...\n");
			CloseHandleToObject(process, KERNEL_OBJECT_PROCESS);
			return nullptr;
		}
	}

	return process; 
}

void Scheduler::Initialise() {
	threadPool.Initialise(sizeof(Thread));
	processPool.Initialise(sizeof(Process));

	char *kernelProcessPath = (char *) "Kernel";
	kernelProcess = SpawnProcess(kernelProcessPath, CStringLength(kernelProcessPath), true);
	kernelProcess->vmm = &kernelVMM;

	initialised = true;
}

unsigned currentProcessorID = 0;

void AsyncTaskThread() {
	CPULocalStorage *local = GetLocalStorage();

	while (true) {
		if (local->asyncTasksRead == local->asyncTasksWrite) {
			ProcessorFakeTimerInterrupt();
		} else {
			volatile AsyncTask *task = local->asyncTasks + local->asyncTasksRead;

			if (task->addressSpace) {
				local->currentThread->asyncTempAddressSpace = task->addressSpace;
				ProcessorSetAddressSpace(VIRTUAL_ADDRESS_SPACE_IDENTIFIER(task->addressSpace));
			}

			task->callback(task->argument);
			local->currentThread->asyncTempAddressSpace = nullptr;
			ProcessorSetAddressSpace(VIRTUAL_ADDRESS_SPACE_IDENTIFIER(&kernelVMM.virtualAddressSpace));

			local->asyncTasksRead++;
		}
	}
}

void Scheduler::CreateProcessorThreads() {
	CPULocalStorage *local = GetLocalStorage();

	Thread *idleThread = (Thread *) threadPool.Add();
	idleThread->isKernelThread = true;
	idleThread->state = THREAD_ACTIVE;
	idleThread->executing = true;
	idleThread->type = THREAD_IDLE;
	idleThread->terminatableState = THREAD_IN_SYSCALL;
	local->currentThread = local->idleThread = idleThread;

	lock.Acquire();

	if (currentProcessorID >= MAX_PROCESSORS) { 
		KernelPanic("Scheduler::CreateProcessorThreads - Maximum processor count (%d) exceeded.\n", currentProcessorID);
	}
	
	local->processorID = currentProcessorID++;

	// Force release the lock because we've changed our currentThread value.
	lock.Release(true);

	localStorage[local->processorID] = local;

	InsertNewThread(idleThread, false, kernelProcess);

	local->asyncTaskThread = SpawnThread((uintptr_t) AsyncTaskThread, 0, kernelProcess, false, false);
	local->asyncTaskThread->type = THREAD_ASYNC_TASK;
}

void Scheduler::InitialiseAP() {
//	CreateProcessorThreads(); (Moved to x86_64.cpp)
	CPULocalStorage *local = GetLocalStorage();
	local->schedulerReady = true; // The processor can now be pre-empted.
}

void RegisterAsyncTask(AsyncTaskCallback callback, void *argument, Process *targetProcess, bool needed) {
	scheduler.lock.AssertLocked();

	if (targetProcess == nullptr) {
		targetProcess = kernelProcess;
	}

	CPULocalStorage *local = GetLocalStorage();

	int difference = local->asyncTasksWrite - local->asyncTasksRead;
	if (difference < 0) difference += MAX_ASYNC_TASKS;

	if (difference >= MAX_ASYNC_TASKS / 2 && !needed) {
		return;
	}

	if (difference == MAX_ASYNC_TASKS - 1) {
		KernelPanic("RegisterAsyncTask - Maximum number of queued asynchronous tasks reached.\n");
	}

	// We need to register tasks for terminating processes.
#if 0
	if (!targetProcess->handles) {
		KernelPanic("RegisterAsyncTask - Process has no handles.\n");
	}
#endif

	volatile AsyncTask *task = local->asyncTasks + local->asyncTasksWrite;
	task->callback = callback;
	task->argument = argument;
	task->addressSpace = &targetProcess->vmm->virtualAddressSpace;
	local->asyncTasksWrite++;
}

void Scheduler::RemoveProcess(Process *process) {
	// KernelLog(LOG_INFO, "Removing process %d.\n", process->id);

	// At this point, no pointers to the process (should) remain (I think).

	if (!process->allThreadsTerminated) {
		KernelPanic("Scheduler::RemoveProcess - The process is being removed before all its threads have terminated?!\n");
	}

	// Free all the remaining messages in the message queue.
	OSHeapFree(process->messageQueue.messages);

	// Destroy the virtual memory manager.
	process->vmm->Destroy();

	// Deallocate the executable path.
	OSHeapFree(process->executablePath, process->executablePathLength);

	processPool.Remove(process);
}

void Scheduler::RemoveThread(Thread *thread) {
	// The last handle to the thread has been closed,
	// so we can finally deallocate the thread.

	// KernelLog(LOG_INFO, "Removing thread %d.\n", thread->id);

	scheduler.threadPool.Remove(thread);
}

void Scheduler::CrashProcess(Process *process, OSCrashReason &crashReason) {
	process->crashMutex.Acquire();

	if (process == kernelProcess) {
		KernelPanic("Scheduler::CrashProcess - Kernel process has crashed (%d).\n", crashReason.errorCode);
	}

	if (GetCurrentThread()->process != process) {
		KernelPanic("Scheduler::CrashProcess - Attempt to crash process from different process.\n");
	}

	if (process == desktopProcess) {
		KernelPanic("Scheduler::CrashProcess - Desktop process has crashed (%d).\n", crashReason.errorCode);
	}

	KernelLog(LOG_WARNING, "Process %x has crashed! (%d)\n", process, crashReason.errorCode);

	CopyMemory(&process->crashReason, &crashReason, sizeof(OSCrashReason));

	Handle handle;
	handle.type = KERNEL_OBJECT_PROCESS;
	handle.object = process;

	scheduler.lock.Acquire();
	process->handles++;
	scheduler.lock.Release();

	OSHandle handle2 = desktopProcess->handleTable.OpenHandle(handle);

	OSMessage message = {};
	message.type = OS_MESSAGE_PROGRAM_CRASH;
	message.crash.process = handle2;
	CopyMemory(&message.crash.reason, &crashReason, sizeof(OSCrashReason));
	desktopProcess->messageQueue.SendMessage(message);

	scheduler.PauseProcess(GetCurrentThread()->process, false);

	process->crashMutex.Release();
}

void Scheduler::PauseThread(Thread *thread, bool resume, bool lockAlreadyAcquired) {
	if (!lockAlreadyAcquired) lock.Acquire();

	if (thread->paused == !resume) {
		return;
	}

	thread->paused = !resume;

	if (!resume) {
		if (thread->state == THREAD_ACTIVE) {
			if (thread->executing) {
				if (thread == GetCurrentThread()) {
					lock.Release();

					// Yield.
					ProcessorFakeTimerInterrupt();

					if (thread->paused) {
						KernelPanic("Scheduler::PauseThread - Current thread incorrectly resumed.\n");
					}
				} else {
					// The thread is executing, but on a different processor.
					// Send them an IPI to stop.
					thread->receivedYieldIPI = false;
					ProcessorSendIPI(YIELD_IPI, false);
					while (!thread->receivedYieldIPI); // Spin until the thread gets the IPI.
					// TODO The interrupt context might not be set at this point.
				}
			} else {
				// Remove the thread from the active queue, and put it into the paused queue.
				activeThreads.Remove(thread->item);
				AddActiveThread(thread, false);
			}
		} else {
			// The thread doesn't need to be in the paused queue as it won't run anyway.
			// If it is unblocked, then AddActiveThread will put it into the correct queue.
		}
	} else if (thread->item->list == &pausedThreads) {
		// Remove the thread from the paused queue, and put it into the active queue.
		pausedThreads.Remove(thread->item);
		AddActiveThread(thread, false);
	}

	if (!lockAlreadyAcquired && thread != GetCurrentThread()) lock.Release();
}

void Scheduler::PauseProcess(Process *process, bool resume) {
	Thread *currentThread = GetCurrentThread();
	bool isCurrentProcess = process == currentThread->process;
	bool foundCurrentThread = false;

	{
		scheduler.lock.Acquire();
		Defer(scheduler.lock.Release());

		LinkedItem<Thread> *thread = process->threads.firstItem;

		while (thread) {
			Thread *threadObject = thread->thisItem;
			thread = thread->nextItem;

			if (threadObject != currentThread) {
				PauseThread(threadObject, resume, true);
			} else if (isCurrentProcess) {
				foundCurrentThread = true;
			} else {
				KernelPanic("Scheduler::PauseProcess - Found current thread in the wrong process?!\n");
			}
		}
	}

	if (!foundCurrentThread && isCurrentProcess) {
		KernelPanic("Scheduler::PauseProcess - Could not find current thread in the current process?!\n");
	} else if (isCurrentProcess) {
		PauseThread(currentThread, resume, false);
	}
}

void CloseHandleToProcess(void *_process) {
	// This must be done in the correct virtual address space!
	// Use RegisterAsyncTask to call this function.

	scheduler.lock.Acquire();

	Process *process = (Process *) _process;
	process->handles--;

	bool deallocate = !process->handles;

	// KernelLog(LOG_VERBOSE, "Handles left to process %x: %d\n", process, process->handles);

	scheduler.lock.Release();

	if (deallocate) {
		scheduler.RemoveProcess(process);
	}
}

void CloseHandleToThread(void *_thread) {
	// This must be done in the correct virtual address space!
	// Use RegisterAsyncTask to call this function.

	Thread *thread = (Thread *) _thread;

	scheduler.lock.Acquire();
	if (!thread->handles) {
		KernelPanic("CloseHandleToThread - All handles to thread have been closed.\n");
	}
	thread->handles--;
	bool removeThread = thread->handles == 0;
	// KernelLog(LOG_VERBOSE, "Handles left to thread %x: %d\n", thread, thread->handles);
	scheduler.lock.Release();

	if (removeThread) {
		scheduler.RemoveThread(thread);
	}
}

void KillThread(void *_thread) {
	Thread *thread = (Thread *) _thread;

	scheduler.lock.Acquire();
	scheduler.allThreads.Remove(&thread->allItem);
	thread->process->threads.Remove(&thread->processItem);

	// KernelLog(LOG_VERBOSE, "Killing thread %x...\n", _thread);

	if (thread->process->threads.count == 0) {
		// KernelLog(LOG_VERBOSE, "Killing process %x...\n", thread->process);

		thread->process->allThreadsTerminated = true;

		// Make sure that the process cannot be opened.
		scheduler.allProcesses.Remove(&thread->process->allItem);
		scheduler.lock.Release();

		// TODO Destroy all the windows this process owns.
		// TODO Destroy all the surfaces this process owns.

		// There are no threads left in this process.
		// We should destroy the handle table at this point.
		// Otherwise, the process might never be freed
		// because of a cyclic-dependency.
		thread->process->handleTable.Destroy();

		// We can now also set the killed event on the process.
		thread->process->killedEvent.Set();
	} else {
		scheduler.lock.Release();
	}

	kernelVMM.Free((void *) thread->kernelStackBase);
	if (thread->userStackBase) thread->process->vmm->Free((void *) thread->userStackBase);

	thread->killedEvent.Set();

	// TODO Release all the mutexes this thread owns.

	// Close the handle that this thread owns of its owner process.
	CloseHandleToObject(thread->process, KERNEL_OBJECT_PROCESS);

	CloseHandleToThread(_thread);
}

void Scheduler::Yield(InterruptContext *context) {
	// Deferred statements don't work in this function.
#undef Defer

	CPULocalStorage *local = GetLocalStorage();

	if (!started || !local || !local->schedulerReady) {
		return;
	}

#if 0
	if (local->interruptRecurseCount > 1) {
		KernelLog(LOG_VERBOSE, "yielding with recurse %d\n", local->interruptRecurseCount);
	}
#endif

	local->currentThread->interruptContext = context;

	lock.Acquire();

	local->currentThread->executing = false;

	bool killThread = local->currentThread->terminatableState == THREAD_TERMINATABLE 
		&& local->currentThread->terminating;
	bool keepThreadAlive = local->currentThread->terminatableState == THREAD_USER_BLOCK_REQUEST
		&& local->currentThread->terminating; // The user can't make the thread block if it is terminating.

	if (killThread) {
		local->currentThread->state = THREAD_TERMINATED;
		// KernelLog(LOG_VERBOSE, "terminated yielded thread %x\n", local->currentThread);
		RegisterAsyncTask(KillThread, local->currentThread, local->currentThread->process, true);
	}

	// If the thread is waiting for an object to be notified, put it in the relevant blockedThreads list.
	// But if the object has been notified yet hasn't made itself active yet, do that for it.

	else if (local->currentThread->state == THREAD_WAITING_MUTEX) {
		if (!keepThreadAlive && local->currentThread->blockingMutex->owner) {
			local->currentThread->blockingMutex->blockedThreads.InsertEnd(&local->currentThread->item[0]);
		} else {
			local->currentThread->state = THREAD_ACTIVE;
		}
	}

	else if (local->currentThread->state == THREAD_WAITING_EVENT) {
		if (keepThreadAlive) {
			local->currentThread->state = THREAD_ACTIVE;
		} else {
			bool unblocked = false;

			for (uintptr_t i = 0; i < local->currentThread->blockingEventCount; i++) {
				if (local->currentThread->blockingEvents[i]->state) {
					local->currentThread->state = THREAD_ACTIVE;
					unblocked = true;
					break;
				}
			}

			if (!unblocked) {
				for (uintptr_t i = 0; i < local->currentThread->blockingEventCount; i++) {
					local->currentThread->blockingEvents[i]->blockedThreads.InsertEnd(&local->currentThread->item[i]);
				}
			}
		}
	}

	// Put the current thread at the end of the activeThreads list.
	if (!killThread && local->currentThread->state == THREAD_ACTIVE) {
		if (local->currentThread->type == THREAD_NORMAL) {
			AddActiveThread(local->currentThread, false);
		} else if (local->currentThread->type == THREAD_IDLE || local->currentThread->type == THREAD_ASYNC_TASK) {
			// Do nothing.
		} else {
			KernelPanic("Scheduler::Yield - Unrecognised thread type\n");
		}
	}

	// Notify any triggered timers.
	
	LinkedItem<Timer> *_timer = activeTimers.firstItem;

	while (_timer) {
		Timer *timer = _timer->thisItem;
		LinkedItem<Timer> *next = _timer->nextItem;

		if (timer->triggerTimeMs <= timeMs) {
			activeTimers.Remove(_timer);
			timer->event.Set(true); // The scheduler is already locked at this point.
		}

		_timer = next;
	}

	// Get a thread from the start of the list.
	LinkedItem<Thread> *firstThreadItem = activeThreads.firstItem;
	Thread *newThread;
	bool newThreadIsAsyncTask = false;

	if (local->asyncTasksRead != local->asyncTasksWrite && local->asyncTaskThread->state == THREAD_ACTIVE) {
		firstThreadItem = nullptr;
		newThread = local->currentThread = local->asyncTaskThread;
		newThreadIsAsyncTask = true;
	} else if (!firstThreadItem) {
		newThread = local->currentThread = local->idleThread;
	} else {
		newThread = local->currentThread = (Thread *) firstThreadItem->thisItem;
	}

	if (newThread->executing) {
		KernelPanic("Scheduler::Yield - Thread (ID %d) in active queue already executing with state %d, type %d\n", local->currentThread->id, local->currentThread->state, local->currentThread->type);
	}

	// Remove the thread we're now executing.
	if (firstThreadItem) activeThreads.Remove(firstThreadItem);

	// Store information about the thread.
	newThread->executing = true;
	newThread->executingProcessorID = local->processorID;
	newThread->timeSlices++;

	// Prepare the next timer interrupt.
	uint64_t time = 10;
	NextTimer(time);

	if (!local->processorID) {
		// Update the scheduler's time.
		timeMs += time; 
	}

	InterruptContext *newContext = newThread->interruptContext;
	VirtualAddressSpace *addressSpace = &newThread->process->vmm->virtualAddressSpace;
	if (newThreadIsAsyncTask && newThread->asyncTempAddressSpace) addressSpace = newThread->asyncTempAddressSpace;
#if 0
	KernelLog(LOG_VERBOSE, "%x/%d/%d\n", VIRTUAL_ADDRESS_SPACE_IDENTIFIER(addressSpace), newThread->id, newThread->type);
#endif
	DoContextSwitch(newContext, VIRTUAL_ADDRESS_SPACE_IDENTIFIER(addressSpace), newThread->kernelStack, newThread);

#define Defer(code) _Defer(code)
}

void Scheduler::WaitMutex(Mutex *mutex) {
	Thread *thread = GetCurrentThread();

	if (thread->state != THREAD_ACTIVE) {
		KernelPanic("Scheduler::WaitMutex - Attempting to wait on a mutex in a non-active thread.\n");
	}

	lock.Acquire();

	thread->state = THREAD_WAITING_MUTEX;
	thread->blockingMutex = mutex;

	// TODO Why doesn't this work?
	// bool spin = mutex && mutex->owner && mutex->owner->executing;
	bool spin = false;

	lock.Release();

	if (!spin) {
		ProcessorFakeTimerInterrupt();
	}

	// Early exit if this is a user request to block the thread and the thread is terminating.
	while ((!thread->terminating || thread->terminatableState != THREAD_USER_BLOCK_REQUEST) && thread->blockingMutex->owner) {
		thread->state = THREAD_WAITING_MUTEX;
	}

	thread->state = THREAD_ACTIVE;
}

uintptr_t Scheduler::WaitEvents(Event **events, size_t count) {
	if (count > OS_MAX_WAIT_COUNT) {
		KernelPanic("Scheduler::WaitEvents - count (%d) > OS_MAX_WAIT_COUNT (%d)\n", count, OS_MAX_WAIT_COUNT);
	} else if (!count) {
		KernelPanic("Scheduler::WaitEvents - Count is 0\n");
	}

	Thread *thread = GetCurrentThread();

	thread->blockingEventCount = count;

	for (uintptr_t i = 0; i < count; i++) {
		thread->blockingEvents[i] = events[i];
	}

	while (!thread->terminating || thread->terminatableState != THREAD_USER_BLOCK_REQUEST) {
		thread->state = THREAD_WAITING_EVENT;

		for (uintptr_t i = 0; i < count; i++) {
			if (events[i]->autoReset) {
				if (events[i]->state) {
					thread->state = THREAD_ACTIVE;

					if (__sync_val_compare_and_swap(&events[i]->state, true, false)) {
						return i;
					}

					thread->state = THREAD_WAITING_EVENT;
				}
			} else {
				if (events[i]->state) {
					thread->state = THREAD_ACTIVE;
					return i;
				}
			}
		}
	}

	return -1; // Exited from termination.
}

void Scheduler::UnblockThread(Thread *unblockedThread) {
	lock.AssertLocked();

	if (unblockedThread->state != THREAD_WAITING_MUTEX && unblockedThread->state != THREAD_WAITING_EVENT) {
		KernelPanic("Scheduler::UnblockedThread - Blocked thread in invalid state %d.\n", 
				unblockedThread->state);
	}

	for (uintptr_t i = 0; i < unblockedThread->blockingEventCount; i++) {
		if (unblockedThread->item[i].list) {
			unblockedThread->item[i].list->Remove(unblockedThread->item + i);
		}
	}

	unblockedThread->state = THREAD_ACTIVE;

	if (!unblockedThread->executing) {
		// Put the unblocked thread at the start of the activeThreads list
		// so that it is immediately executed when the scheduler yields.
		unblockedThread->state = THREAD_ACTIVE;
		AddActiveThread(unblockedThread, true);
	} 
}

void Scheduler::NotifyObject(LinkedList<Thread> *blockedThreads, bool schedulerAlreadyLocked, bool unblockAll) {
	if (schedulerAlreadyLocked == false) lock.Acquire();
	lock.AssertLocked();

	LinkedItem<Thread> *unblockedItem = blockedThreads->firstItem;

	if (!unblockedItem) {
		if (schedulerAlreadyLocked == false) lock.Release();

		// There weren't any threads blocking on the mutex.
		return; 
	}

	do {
		LinkedItem<Thread> *nextUnblockedItem = unblockedItem->nextItem;
		blockedThreads->Remove(unblockedItem);
		Thread *unblockedThread = unblockedItem->thisItem;
		UnblockThread(unblockedThread);
		unblockedItem = nextUnblockedItem;
	} while (unblockAll && unblockedItem);

	if (schedulerAlreadyLocked == false) lock.Release();
}

void Mutex::Acquire() {
	if (scheduler.panic) return;

	Thread *currentThread = GetCurrentThread();
	bool hasThread = currentThread;

	if (!currentThread) {
		currentThread = (Thread *) 1;
	} else {
		currentThread->blockingEventCount = 1;

		if (currentThread->terminatableState == THREAD_TERMINATABLE) {
			KernelPanic("Mutex::Acquire - Thread is terminatable.\n");
		}
	}

	if (hasThread && owner && owner == currentThread) {
		KernelPanic("Mutex::Acquire - Attempt to acquire mutex (%x) at %x owned by current thread (%x) acquired at %x.\n", 
				this, __builtin_return_address(0), currentThread, acquireAddress);
	}

	if (!ProcessorAreInterruptsEnabled()) {
		KernelPanic("Mutex::Acquire - Trying to wait on a mutex while interrupts are disabled.\n");
	}

	while (__sync_val_compare_and_swap(&owner, nullptr, currentThread)) {
		__sync_synchronize();

		// TODO This is a bit of a hack.
		if (GetLocalStorage() && GetLocalStorage()->schedulerReady) {
			// Instead of spinning on the lock, 
			// let's tell the scheduler to not schedule this thread
			// until it's released.
			scheduler.WaitMutex(this);

			if (currentThread->terminating && currentThread->terminatableState == THREAD_USER_BLOCK_REQUEST) {
				// We didn't acquire the mutex because the thread is terminating.
				return;
			}
		}
	}

	__sync_synchronize();

	if (owner != currentThread) {
		KernelPanic("Mutex::Acquire - Invalid owner thread (%x, expected %x).\n", owner, currentThread);
	}

	acquireAddress = (uintptr_t) __builtin_return_address(0);
	AssertLocked();
}

void Mutex::Release() {
	if (scheduler.panic) return;

	AssertLocked();

	Thread *currentThread = GetCurrentThread();

	if (currentThread) {
		Thread *temp;
		if (currentThread != (temp = __sync_val_compare_and_swap(&owner, currentThread, nullptr))) {
			KernelPanic("Mutex::Release - Invalid owner thread (%x, expected %x).\n", temp, currentThread);
		}
	} else {
		owner = nullptr;
	}

	__sync_synchronize();

	if (scheduler.started) {
		scheduler.NotifyObject(&blockedThreads, false);
	}

	releaseAddress = (uintptr_t) __builtin_return_address(0);
}

void Mutex::AssertLocked() {
	Thread *currentThread = GetCurrentThread();

	if (!currentThread) {
		currentThread = (Thread *) 1;
	}

	if (owner != currentThread) {
		KernelPanic("Mutex::AssertLocked - Mutex not correctly acquired\n"
				"currentThread = %x, owner = %x\nthis = %x\nReturn %x/%x\nLast used from %x->%x\n", 
				currentThread, owner, this, __builtin_return_address(0), __builtin_return_address(1), 
				acquireAddress, releaseAddress);
	}
}

void Semaphore::Take(uintptr_t u) {
	while (u) {
		available.Wait(OS_WAIT_NO_TIMEOUT);

		mutex.Acquire();
		if (units) { units--; u--; }
		if (!units && available.state) available.Reset();
		mutex.Release();
	}
}

void Semaphore::Return(uintptr_t u) {
	mutex.Acquire();
	if (!available.state) available.Set();
	units += u;
	mutex.Release();
}

void Semaphore::Set(uintptr_t u) {
	mutex.Acquire();
	if (!available.state && u) available.Set();
	else if (available.state && !u) available.Reset();
	units = u;
	mutex.Release();
}

void Event::Set(bool schedulerAlreadyLocked, bool maybeAlreadySet) {
	if (state && !maybeAlreadySet) {
		KernelLog(LOG_WARNING, "Event::Set - Attempt to set a event that had already been set\n");
	}

	state = true;

	if (scheduler.started) {
		scheduler.NotifyObject(&blockedThreads, schedulerAlreadyLocked, !autoReset /*If this is a manually reset event, unblock all the waiting threads.*/);
	}
}

void Event::Reset() {
	if (blockedThreads.firstItem) {
		KernelLog(LOG_WARNING, "Event::Reset - Attempt to reset a event while threads are blocking on the event\n");
	}

	state = false;
}

bool Event::Poll() {
	if (autoReset) {
		return __sync_val_compare_and_swap(&state, true, false);
	} else {
		return state;
	}
}

bool Event::Wait(uint64_t timeoutMs) {
	Event *events[2];
	events[0] = this;

	if (timeoutMs == (uint64_t) OS_WAIT_NO_TIMEOUT) {
		int index = scheduler.WaitEvents(events, 1);
		return index == 0;
	} else {
		Timer timer = {};
		timer.Set(timeoutMs, false);
		events[1] = &timer.event;
		int index = scheduler.WaitEvents(events, 2);
		
		if (index == 1) {
			return false;
		} else {
			timer.Remove();
			return true;
		}
	}
}

void Timer::Set(uint64_t triggerInMs, bool autoReset) {
	scheduler.lock.Acquire();
	Defer(scheduler.lock.Release());

	event.Reset();
	event.autoReset = autoReset;
	triggerTimeMs = triggerInMs + scheduler.timeMs;
	item.thisItem = this;
	scheduler.activeTimers.InsertStart(&item);
}

void Timer::Remove() {
	scheduler.lock.Acquire();
	Defer(scheduler.lock.Release());

	if (item.list) {
		scheduler.activeTimers.Remove(&item);
	}
}

#endif
