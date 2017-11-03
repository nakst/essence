// TODO Everything in this file is a hack just so I can debug the kernel.
// 	Replace all of it!!!

#ifndef IMPLEMENTATION
#endif

#ifdef IMPLEMENTATION

#define TERMINAL_ADDRESS (LOW_MEMORY_MAP_START + 0xB8000)

#if 1
Spinlock terminalLock; 
Spinlock printLock; 
#else
Mutex terminalLock; 
Mutex printLock; 
#endif

bool printToTerminal, printToSerialOutput = true;
uintptr_t terminalPosition = 80;

static void TerminalCallback(int character, void *data) {
	(void) data;

	terminalLock.Acquire();
	Defer(terminalLock.Release());

	if (printToTerminal) {
		if (terminalPosition >= 80 * 25) {
			for (int i = 80; i < 80 * 25; i++) {
				((uint16_t *) (TERMINAL_ADDRESS))[i - 80] = 
					((uint16_t *) (TERMINAL_ADDRESS))[i];
			}

			for (int i = 80 * 24; i < 80 * 25; i++) {
				((uint16_t *) (TERMINAL_ADDRESS))[i] = 0x700;
			}

			terminalPosition -= 80;
		}

		if (character == '\n') {
			terminalPosition = terminalPosition - (terminalPosition % 80) + 80;
		} else {
			((uint16_t *) (TERMINAL_ADDRESS))[terminalPosition] = (uint16_t) character | (uint16_t) (uintptr_t) data;
			terminalPosition++;
		}

		{
			ProcessorOut8(0x3D4, 0x0F);
			ProcessorOut8(0x3D5, terminalPosition);
			ProcessorOut8(0x3D4, 0x0E);
			ProcessorOut8(0x3D5, terminalPosition >> 8);
		}
	}

	if (printToSerialOutput) {
		ProcessorDebugOutputByte((uint8_t) character);

		if (character == '\n') {
			ProcessorDebugOutputByte((uint8_t) 13);
		}
	}
}

int ReadScancode() {
	while (!(ProcessorIn8(0x64) & 1));

	int s = ProcessorIn8(0x60);

	if (s == 224) {
		return 256 + ProcessorIn8(0x60);
	} else {
		return s;
	}
}

#define SCANCODE_ENTER (28)
#define SCANCODE_UP (72)
#define SCANCODE_DOWN (80)

int debuggerSelection = 0;
int debuggerSelectionHistory[256];
int debuggerSelectionHistoryIndex = 0;
int debuggerIndex = 0;

int debuggerScancode;
bool debuggerEnter;

bool DebuggerOption(bool back = false) {
	Print("\n");

	if (debuggerSelection == debuggerIndex) {
		for (uintptr_t i = terminalPosition - 80; i < terminalPosition; i++) {
			((uint8_t *) TERMINAL_ADDRESS)[i * 2 + 1] = 0x4F;
		}
	}

	if (debuggerSelection == debuggerIndex && debuggerScancode == SCANCODE_ENTER) {
		debuggerScancode = -1;
		if (!back) {
			debuggerSelectionHistory[debuggerSelectionHistoryIndex++] = debuggerSelection = debuggerSelection;
			debuggerSelection = 0;
		}
		return true;
	}

	debuggerIndex++;
	return false;
}

bool DebuggerStart(bool back = true) {
	if (debuggerSelection >= debuggerIndex) debuggerSelection = 0;
	debuggerIndex = 0;
	terminalPosition = 0;
	ZeroMemory((void *) TERMINAL_ADDRESS, 80 * 25 * 2);

	Print("                             --- KERNEL DEBUGGER ---\n");
	for (uintptr_t i = terminalPosition - 80; i < terminalPosition; i++) {
		((uint8_t *) TERMINAL_ADDRESS)[i * 2 + 1] = 0x3F;
	}

	if (back) {
		Print("<- Back");
		if (DebuggerOption(true)) {
			debuggerSelection = debuggerSelectionHistory[--debuggerSelectionHistoryIndex];
			return true;
		} else {
			return false;
		}
	} else return false;
}

void DebuggerFinish() {
	terminalPosition = 80 * 25 - 80;
	Print("up/down - move; enter - okay; %d/%d\n", debuggerSelection + 1, debuggerIndex);
	for (uintptr_t i = terminalPosition - 80; i < terminalPosition; i++) {
		((uint8_t *) TERMINAL_ADDRESS)[i * 2 + 1] = 0x3F;
	}

	GetScancode:
	if (!debuggerEnter) {
		debuggerScancode = ReadScancode();
	} else {
		debuggerScancode = SCANCODE_DOWN;
		debuggerEnter = false;
		debuggerIndex = 0;
	}

	if (debuggerScancode == SCANCODE_UP && debuggerSelection) debuggerSelection--;
	else if (debuggerScancode == SCANCODE_DOWN && debuggerSelection != debuggerIndex - 1) debuggerSelection++;
	else if (debuggerScancode != SCANCODE_ENTER && !debuggerEnter) goto GetScancode;

	if (debuggerScancode == SCANCODE_ENTER) {
		debuggerEnter = true;
	}

	Print("...\n");
}

void DebuggerThread(Thread *thread);

void DebuggerMutex(Mutex *mutex) {
	while (true) {
		if (DebuggerStart()) break;

		Print("owner thread %x", mutex->owner);
		if (DebuggerOption()) {
			DebuggerThread(mutex->owner);
		}

		Print("this: %x\n", mutex);
		Print("acquireAddress: %x\n", mutex->acquireAddress);
		Print("releaseAddress: %x\n", mutex->releaseAddress);

		DebuggerFinish();
	}
}

void DebuggerVMM(VMM *vmm) {
	while (true) {
		if (DebuggerStart()) break;

		Print("mutex"); if (DebuggerOption()) DebuggerMutex(&vmm->lock);

		Print("regionsCount: %d\n", vmm->regionsCount);
		Print("lookupRegionsCount: %d\n", vmm->lookupRegionsCount);

		Print("allocatedVirtualMemory = %x\n", vmm->allocatedVirtualMemory);
		Print("allocatedPhysicalMemory = %x\n", vmm->allocatedPhysicalMemory);

		DebuggerFinish();
	}
}

void DebuggerProcess(Process *process);

void DebuggerThread(Thread *thread) {
	while (true) {
		if (DebuggerStart()) break;

		Print("owner Process %d", thread->process->id);
		if (DebuggerOption()) {
			DebuggerProcess(thread->process);
		}

		Print("state = %z\n", thread->state == THREAD_ACTIVE ? "Active" : (thread->state == THREAD_TERMINATED ? "Dead" : "Waiting"));
		Print("type = %z\n", thread->type == THREAD_NORMAL ? "Normal" : (thread->type == THREAD_IDLE ? "Idle" : "AsyncTask"));

		if (thread->executing) {
			Print("Executing on CPU %d\n", thread->executingProcessorID);
		} else {
			Print("not executing\n");
		}

		Print("interruptContext = %x\n", thread->interruptContext);

		if (thread->isKernelThread)
			Print("kernel thread\n");

		DebuggerFinish();
	}
}

void DebuggerProcess(Process *process) {
	while (true) {
		if (DebuggerStart()) break;

		Print("VMM");
		if (DebuggerOption()) {
			DebuggerVMM(process->vmm);
		}

		LinkedItem *item = process->threads.firstItem;

		while (item) {
			Thread *thread = (Thread *) item->thisItem;
			Print("Thread %d (%x)", thread->id, thread);

			if (DebuggerOption()) {
				DebuggerThread(thread);
			}

			item = item->nextItem;
		}

		Print("executable: %s\n", process->executablePathLength, process->executablePath);
		Print("handles: %d\n", process->handles);
		Print("id: %d\n", process->id);
		Print("image state: %z\n", process->executableState == PROCESS_EXECUTABLE_NOT_LOADED ? "not loaded" : 
				(process->executableState == PROCESS_EXECUTABLE_FAILED_TO_LOAD ? "failed to load" : "correctly loaded"));

		DebuggerFinish();
	}
}

void DebuggerProcesses() {
	while (true) {
		if (DebuggerStart()) break;

		LinkedItem *item = scheduler.allProcesses.firstItem;

		while (item) {
			Process *process = (Process *) item->thisItem;
			Print("Process %d - %s", process->id, process->executablePathLength, process->executablePath);

			if (DebuggerOption()) {
				DebuggerProcess(process);
			}

			item = item->nextItem;
		}

		DebuggerFinish();
	}
}

void DebuggerThreads(LinkedList &list) {
	while (true) {
		if (DebuggerStart()) break;

		LinkedItem *item = list.firstItem;

		while (item) {
			Thread *thread = (Thread *) item->thisItem;
			Print("Thread %d (%x)", thread->id, thread);

			Print(" %z", thread->state == THREAD_ACTIVE ? "Active" : (thread->state == THREAD_TERMINATED ? "Terminated" : "Waiting"));
			Print("/%z", thread->type == THREAD_NORMAL ? "Normal" : (thread->type == THREAD_IDLE ? "Idle" : "AsyncTask"));
			Print("/Proc %d", thread->process->id);

			if (thread->executing) {
				Print("/CPU %d", thread->executingProcessorID);
			}

			if (DebuggerOption()) {
				DebuggerThread(thread);
			}

			item = item->nextItem;
		}

		DebuggerFinish();
	}
}

void DebuggerVFS() {
	while (true) {
		if (DebuggerStart()) break;

		Print("MUTEX\n"); if (DebuggerOption()) DebuggerMutex(&vfs.lock);

		DebuggerFinish();
	}
}

void EnterDebugger() {
	printToSerialOutput = false;
	while (ReadScancode() != SCANCODE_ENTER);

	debuggerSelection = 0;
	debuggerScancode = -1;

	while (true) {
		DebuggerStart(false);

		Print("all processes");
		if (DebuggerOption()) {
			DebuggerProcesses();
		}

		Print("all threads");
		if (DebuggerOption()) {
			DebuggerThreads(scheduler.allThreads);
		}

		Print("active threads");
		if (DebuggerOption()) {
			DebuggerThreads(scheduler.activeThreads);
		}

		Print("kernelVMM");
		if (DebuggerOption()) {
			DebuggerVMM(&kernelVMM);
		}

		Print("VFS");
		if (DebuggerOption()) {
			DebuggerVFS();
		}

		DebuggerFinish();
	}
}

void KernelPanic(const char *format, ...) {
	ProcessorDisableInterrupts();
	scheduler.panic = true;
	ProcessorSendIPI(KERNEL_PANIC_IPI, true);

	printToTerminal = true;

	Print("\n--- KERNEL PANIC ---\n[Fatal] ");

	va_list arguments;
	va_start(arguments, format);
	_FormatString(TerminalCallback, (void *) 0x4F00, format, arguments);
	va_end(arguments);

	Print("Current thread = %x\n", GetCurrentThread());
	Print("Trace: %x\n", __builtin_return_address(1));
	Print("Press <ENTER> to enter the kernel debugger...\n");

	for (int i = 0; i < 80 * 25; i++) {
		((uint8_t *) TERMINAL_ADDRESS)[i * 2 + 1] = 0x4F;
	}

	EnterDebugger();
	ProcessorHalt();
}

#define ENABLE_OUTPUT

void Print(const char *format, ...) {
#ifdef ENABLE_OUTPUT
	printLock.Acquire();
	Defer(printLock.Release());

	printToTerminal = true;

	va_list arguments;
	va_start(arguments, format);
	_FormatString(TerminalCallback, (void *) 0x0700, format, arguments);
	va_end(arguments);
#endif
}

void _KernelLog(const char *format, ...) {
	va_list arguments;
	va_start(arguments, format);
	_FormatString(TerminalCallback, (void *) 0x0700, format, arguments);
	va_end(arguments);
}

void KernelLog(LogLevel level, const char *format, ...) {
#ifdef ENABLE_OUTPUT
	printLock.Acquire();
	Defer(printLock.Release());

#define MINIMUM_LOG_LEVEL LOG_VERBOSE
#define MINIMUM_PRINT_LEVEL LOG_INFO

	if (level < MINIMUM_LOG_LEVEL) {
		// Don't log at this level.
		return;
	}

	printToTerminal = level >= MINIMUM_PRINT_LEVEL;

	_KernelLog("[%z] ", 	level == LOG_INFO ? 	"Info"
			      : level == LOG_WARNING ? 	"Warning"
			      : level == LOG_ERROR ? 	"Error"
			      : level == LOG_VERBOSE ? 	"Verbose" : "");

	va_list arguments;
	va_start(arguments, format);
	_FormatString(TerminalCallback, (void *) 0x0700, format, arguments);
	va_end(arguments);
#endif
}

void PrintStats() {
	Print("stats: [pmmalloc] %d; [vmmlook] %d; [vmmreg] %d; [vmmalloc] %d\n", pmm.pagesAllocated * PAGE_SIZE, 
			kernelVMM.lookupRegionsCount, kernelVMM.regionsCount, kernelVMM.allocatedVirtualMemory * PAGE_SIZE);
}
#endif
