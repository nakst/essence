// TODO Priority inversion
// TODO Yield on mutex/spinlock release and event set

#ifndef IMPLEMENTATION

void CloseHandleToThread(void *_thread);
void CloseHandleToProcess(void *_thread);
void KillThread(void *_thread);

void RegisterAsyncTask(AsyncTaskCallback callback, void *argument, struct Process *targetProcess);

struct Event {
	void Set(bool schedulerAlreadyLocked = false);
	void Reset(); 
	bool Poll();

	bool Wait(uint64_t timeoutMs); // See Scheduler::WaitEvents to wait for multiple events.
				       // Returns false if the wait timed out.

	bool autoReset; // This should be first field in the structure,
			// so that the type of Event can be easily declared with {autoReset}.

	volatile uintptr_t state;

	LinkedList blockedThreads;
};

struct Timer {
	void Set(uint64_t triggerInMs, bool autoReset);
	void Remove();

	Event event;
	LinkedItem item;
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
	void *temporary;

#define MAX_BLOCKING_EVENTS 16
	LinkedItem item[MAX_BLOCKING_EVENTS];	// Entry in activeThreads or blockedThreads list.
	LinkedItem allItem; 			// Entry in the allThreads list.
	LinkedItem processItem; 		// Entry in the process's list of threads.

	struct Process *process;

	uintptr_t id;
	uintptr_t timeSlices;

	volatile ThreadState state;
	volatile bool executing;
	volatile bool terminating; // Set when a request to terminate the thread has been registered.

	int executingProcessorID;

	Mutex *volatile blockingMutex;
	Event *volatile blockingEvents[MAX_BLOCKING_EVENTS];
	volatile size_t blockingEventCount;

	InterruptContext *interruptContext;

	Event killedEvent;

	uintptr_t userStackBase;
	uintptr_t kernelStackBase;

	uintptr_t kernelStack;
	bool isKernelThread;

	ThreadType type;

	volatile ThreadTerminatableState terminatableState;

	volatile size_t handles;

	// If the type of the thread is THREAD_ASYNC_TASK,
	// then this is the virtual address space that should be loaded
	// when the task is being executed.
	VirtualAddressSpace *volatile asyncTempAddressSpace;

	// TODO Temporary.
	uintptr_t lastKnownExecutionAddress;
};

struct Process {
	bool SendMessage(OSMessage &message); // Returns false if the message queue is full.
#define MESSAGE_QUEUE_MAX_LENGTH 4096
	LinkedList messageQueue;
	Mutex messageQueueMutex;
	Event messageQueueIsNotEmpty;

	LinkedItem allItem;
	LinkedList threads;

	VMM *vmm;
	VMM _vmm;

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

	HandleTable handleTable;

	Event killedEvent;
};

struct Message {
	LinkedItem item;
	OSMessage data;
};

Pool messagePool;
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

	Process *SpawnProcess(char *imagePath, size_t imagePathLength, bool kernelProcess = false, void *argument = nullptr);
	void TerminateProcess(Process *process);
	void RemoveProcess(Process *process); // Do not call. Use TerminateProcess/CloseHandleToObject.

	void AddActiveThread(Thread *thread, bool start);
	void InsertNewThread(Thread *thread, bool addToActiveList, Process *owner);

	void WaitMutex(Mutex *mutex);
	uintptr_t WaitEvents(Event **events, size_t count); // Returns index of notified object.
	void NotifyObject(LinkedList *blockedThreads, bool schedulerAlreadyLocked = false, bool unblockAll = false);
	void UnblockThread(Thread *unblockedThread);

	Pool threadPool, processPool;
	LinkedList activeThreads;
	LinkedList activeTimers;
	LinkedList allThreads;
	LinkedList allProcesses;
	Spinlock lock;

	uintptr_t nextThreadID;
	uintptr_t nextProcessID;
	uintptr_t processors;

	bool initialised;
	volatile bool started;

	uint64_t timeMs;

	struct CPULocalStorage *localStorage[MAX_PROCESSORS];

	bool panic;

	Pool globalMutexPool;
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
		Print("__builtin_return_address(0) = %x\n", __builtin_return_address(0));
		Print("__builtin_return_address(1) = %x\n", __builtin_return_address(1));
		Print("__builtin_return_address(2) = %x\n", __builtin_return_address(2));
		Print("__builtin_return_address(3) = %x\n", __builtin_return_address(3));
		Print("__builtin_return_address(4) = %x\n", __builtin_return_address(4));
		Print("__builtin_return_address(5) = %x\n", __builtin_return_address(5));
		Print("__builtin_return_address(6) = %x\n", __builtin_return_address(6));
		KernelPanic("Spinlock::Acquire - Attempt to acquire a spinlock owned by the current thread (%x/%x).\n", storage->currentThread, owner);
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

	if (start) {
		activeThreads.InsertStart(&thread->item[0]);
	} else {
		activeThreads.InsertEnd(&thread->item[0]);
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

	for (uintptr_t i = 0; i < MAX_BLOCKING_EVENTS; i++) {
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
	// KernelLog(LOG_VERBOSE, "Created thread, %x -> %x\n", thread, thread + 1);
	thread->isKernelThread = !userland;

	// 2 handles to the thread:
	// 	One for spawning the thread, 
	// 	and the other for remaining during the thread's life.
	thread->handles = 2;

	// Allocate the thread's stacks.
	uintptr_t kernelStackSize = userland ? 0x4000 : 0x10000;
	uintptr_t userStackSize = userland ? 0x100000 : 0x10000;
	uintptr_t stack, kernelStack = (uintptr_t) kernelVMM.Allocate(kernelStackSize, vmmMapAll);;

	if (userland) {
		stack = (uintptr_t) process->vmm->Allocate(userStackSize, vmmMapLazy);
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

	LinkedItem *thread = process->threads.firstItem;

	while (thread) {
		Thread *threadObject = (Thread *) thread->thisItem;
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
				// Remove it from the executing list, and then remove the thread.
				activeThreads.Remove(&thread->item[0]);
				RegisterAsyncTask(KillThread, thread, thread->process);
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
	// KernelLog(LOG_VERBOSE, "Created process %d, %s.\n", thisProcess->id, thisProcess->executablePathLength, thisProcess->executablePath);

	// TODO Shared memory with executables.
	uintptr_t processStartAddress = LoadELF(thisProcess->executablePath, thisProcess->executablePathLength);

	if (processStartAddress) {
		thisProcess->executableState = PROCESS_EXECUTABLE_LOADED;
		thisProcess->executableMainThread = scheduler.SpawnThread(processStartAddress, 0, thisProcess, true);
	} else {
		thisProcess->executableState = PROCESS_EXECUTABLE_FAILED_TO_LOAD;
		KernelPanic("NewProcess - Could not start a new process.\n");
	}

	thisProcess->executableLoadAttemptComplete.Set();
	scheduler.TerminateThread(GetCurrentThread());
}

Process *Scheduler::SpawnProcess(char *imagePath, size_t imagePathLength, bool kernelProcess, void *argument) {
	// Process initilisation.
	Process *process = (Process *) processPool.Add();
	// KernelLog(LOG_VERBOSE, "Created process, %x -> %x\n", process, process + 1);
	process->allItem.thisItem = process;
	process->vmm = &process->_vmm;
	process->handles = 1;
	process->creationArgument = argument;
	if (!kernelProcess) process->vmm->Initialise();
	CopyMemory((process->executablePath = (char *) OSHeapAllocate(imagePathLength, false)), imagePath, imagePathLength);
	process->executablePathLength = imagePathLength;

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

	globalMutexPool.Initialise(sizeof(Mutex));

	messagePool.Initialise(sizeof(Message));

	char *kernelProcessPath = (char *) "Kernel";
	kernelProcess = SpawnProcess(kernelProcessPath, CStringLength(kernelProcessPath), true);
	kernelProcess->vmm = &kernelVMM;

	initialised = true;
}

unsigned currentProcessorID = 0;

void AsyncTaskThread() {
	CPULocalStorage *local = GetLocalStorage();

	if (!local->asyncTasksCount) {
		KernelPanic("AsyncTaskThread - Thread started with no async tasks to execute.\n");
	}

	while (true) {
		uintptr_t i = 0;
		while (true) {
			volatile AsyncTask *task = local->asyncTasks + i;

			if (task->addressSpace) {
				local->currentThread->asyncTempAddressSpace = task->addressSpace;
				ProcessorSetAddressSpace(VIRTUAL_ADDRESS_SPACE_IDENTIFIER(task->addressSpace));
			} else {
				local->currentThread->asyncTempAddressSpace = nullptr;
				ProcessorSetAddressSpace(VIRTUAL_ADDRESS_SPACE_IDENTIFIER(&kernelVMM.virtualAddressSpace));
			}

			task->callback(task->argument);
			i++;

			local->currentThread->asyncTempAddressSpace = nullptr;
			ProcessorSetAddressSpace(VIRTUAL_ADDRESS_SPACE_IDENTIFIER(&kernelVMM.virtualAddressSpace));

			// If that was the last task, exit.
			if (i == __sync_val_compare_and_swap(&local->asyncTasksCount, i, 0)) {
				break;
			}
		}
			
		ProcessorFakeTimerInterrupt();

		if (local->asyncTasksCount == 0) {
			KernelPanic("AsyncTaskThread - ProcessorFakeTimerInterrupt returned with no async tasks to execute.\n");
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
	GetLocalStorage()->schedulerReady = true; // The processor can now be pre-empted.
}

void RegisterAsyncTask(AsyncTaskCallback callback, void *argument, Process *targetProcess) {
	scheduler.lock.AssertLocked();

	CPULocalStorage *local = GetLocalStorage();

	if (local->asyncTasksCount == MAX_ASYNC_TASKS) {
		KernelPanic("RegisterAsyncTask - Maximum number of queued asynchronous tasks reached.\n");
	}

	if (!targetProcess->handles) {
		KernelPanic("RegisterAsyncTask - Process has no handles.\n");
	}

	volatile AsyncTask *task = local->asyncTasks + local->asyncTasksCount;
	task->callback = callback;
	task->argument = argument;
	task->addressSpace = &targetProcess->vmm->virtualAddressSpace;
	local->asyncTasksCount++;
}

void Scheduler::RemoveProcess(Process *process) {
	// KernelLog(LOG_INFO, "Removing process %d.\n", process->id);

	// At this point, no pointers to the process (should) remain (I think).

	// Free all the remaining messages in the message queue.
	LinkedList &messageQueue = process->messageQueue;
	LinkedItem *item = messageQueue.firstItem;
	while (item != messageQueue.lastItem) {
		Message *message = (Message *) item->thisItem;
		messagePool.Remove(message);
	}

	// Destroy the virtual memory manager.
	process->vmm->Destroy();

	// Deallocate the executable path.
	OSHeapFree(process->executablePath, process->executablePathLength);

	processPool.Remove(process);
}

void Scheduler::RemoveThread(Thread *thread) {
	// The last handle to the thread has been closed,
	// so we can finally deallocate the thread.

	scheduler.threadPool.Remove(thread);
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

		// Make sure that the process cannot be opened.
		scheduler.allProcesses.Remove(&thread->process->allItem);
		scheduler.lock.Release();

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
		RegisterAsyncTask(KillThread, local->currentThread, local->currentThread->process);
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
	
	LinkedItem *_timer = activeTimers.firstItem;

	while (_timer) {
		Timer *timer = (Timer *) _timer->thisItem;
		LinkedItem *next = _timer->nextItem;

		if (timer->triggerTimeMs <= timeMs) {
			activeTimers.Remove(_timer);
			timer->event.Set(true); // The scheduler is already locked at this point.
		}

		_timer = next;
	}

	// Get a thread from the start of the list.
	LinkedItem *firstThreadItem = activeThreads.firstItem;
	Thread *newThread;
	bool newThreadIsAsyncTask = false;

	if (local->asyncTasksCount && local->asyncTaskThread->state == THREAD_ACTIVE) {
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
	uint64_t time = 20;
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

	lock.Release();

	// Early exit if this is a user request to block the thread and the thread is terminating.
	while ((!thread->terminating || thread->terminatableState != THREAD_USER_BLOCK_REQUEST) && thread->blockingMutex->owner);
	thread->state = THREAD_ACTIVE;
}

uintptr_t Scheduler::WaitEvents(Event **events, size_t count) {
	if (count > MAX_BLOCKING_EVENTS) {
		KernelPanic("Scheduler::WaitEvents - count (%d) > MAX_BLOCKING_EVENTS (%d)\n", count, MAX_BLOCKING_EVENTS);
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

void Scheduler::NotifyObject(LinkedList *blockedThreads, bool schedulerAlreadyLocked, bool unblockAll) {
	if (schedulerAlreadyLocked == false) lock.Acquire();
	lock.AssertLocked();

	LinkedItem *unblockedItem = blockedThreads->firstItem;

	if (!unblockedItem) {
		if (schedulerAlreadyLocked == false) lock.Release();

		// There weren't any threads blocking on the mutex.
		return; 
	}

	do {
		LinkedItem *nextUnblockedItem = unblockedItem->nextItem;
		blockedThreads->Remove(unblockedItem);
		Thread *unblockedThread = (Thread *) unblockedItem->thisItem;
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

void Event::Set(bool schedulerAlreadyLocked) {
	if (state) {
		KernelPanic("Event::Set - Attempt to set a event that had already been set\n");
	}

	state = true;

	if (scheduler.started) {
		scheduler.NotifyObject(&blockedThreads, schedulerAlreadyLocked, !autoReset /*If this is a manually reset event, unblock all the waiting threads.*/);
	}
}

void Event::Reset() {
	if (blockedThreads.firstItem) {
		KernelPanic("Event::Reset - Attempt to reset a event while threads are blocking on the event\n");
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
