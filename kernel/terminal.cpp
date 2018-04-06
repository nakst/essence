// TODO Everything in this file is a hack just so I can debug the kernel.
// 	Replace all of it!!!

#ifndef IMPLEMENTATION
#endif

#ifdef IMPLEMENTATION

uint64_t font[] = {
	0x0000000000000000, 0x2200880022008800, 0xAA00AA00AA00AA00, 0xAB54AB54AB54AB54, 
	0xFFFFFFFFFFFFFFFE, 0x10386C44C0808000, 0x0088D87070D88800, 0x041C78F0781C0400, 
	0x2828282828282828, 0x000001FE01FE0000, 0x282829E809F80000, 0x2828282E203E0000, 
	0x000001F809E82828, 0x0000003E202E2828, 0x0000000000000000, 0x0000000000000000, 
	0x282829E809E82828, 0x2828282E202E2828, 0x282829EE01FE0000, 0x000001FE01EE2828, 
	0x282829EE01EE2828, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
	0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
	0x0000000000000000, 0x3030003030303030, 0x000000000048D8D8, 0x4848FC4848FC4848, 
	0x51FD51FC5455FC50, 0x00C4C810204C8C00, 0x00B844A418242418, 0x0000000000103030, 
	0x6030181818183060, 0x1830606060603018, 0x0000000050207020, 0x303030FC30303000, 
	0x1020303000000000, 0x000000FC00000000, 0x3030000000000000, 0x0404080810102020, 
	0x30488494A4844830, 0x7820202020283020, 0xF810204080808870, 0x7088807080808870, 
	0x404040FC48506040, 0x3048404038080878, 0x3844443C04044438, 0x101020202020407C, 
	0x7884847884848478, 0x808080F088888870, 0x0030300030300000, 0x1020303000303000, 
	0x4020100810204000, 0x0000780000780000, 0x0810204020100800, 0x1818003840404038, 
	0xF804B34B4AB24438, 0x4848487848484830, 0x7888888878484838, 0x7008080808080870, 
	0x7888888888888878, 0x7808087808080878, 0x0808087808080878, 0x7048484808080870, 
	0x4848484878484848, 0xF8202020202020F8, 0x30484040404041F8, 0x4424140C0C141424, 
	0xF808080808080808, 0x252555558D8D0504, 0x88C8C8A8A8989888, 0x7088888888888870, 
	0x04043C444444443C, 0x8078A49484848478, 0x4424143C4444443C, 0x7080807008080870, 
	0x20202020202020F8, 0x7088888888888888, 0x1010282828284444, 0x4848D4D4B4B52322, 
	0x4428281010282844, 0x2020202020505088, 0x7C0808101020207C, 0x3808080808080838, 
	0x2020101008080404, 0x3820202020202038, 0x0000000000442810, 0xFC00000000000000, 
	0x0000000020301800, 0x7088F08088700000, 0x748C848C74040404, 0x6010081060000000, 
	0xF048487040404000, 0x7008784830000000, 0x182020F82020C000, 0x3840704848700000, 
	0x8888986808080800, 0x6010101000100000, 0x3840404040004000, 0x4828281828280800, 
	0x3048080808080800, 0x545454542C000000, 0x8888885828000000, 0x3048483000000000, 
	0x1010305030000000, 0x4040605060000000, 0x0808186800000000, 0x3840300870000000, 
	0x7008080808780808, 0x7844444400000000, 0x1028284400000000, 0x2838544400000000, 
	0x4850284800000000, 0x3840784848000000, 0x7808304078000000, 0x6010100810101060, 
	0x2020202020202020, 0x1820204020202018, 0x0000285000000000, 0x0000000000000000, 
};

#if 0
Font usage example:
uint64_t a = font['a'];
for (int j = 0; j < 8; j++) {
	for (int i = 0; i < 8; i++) {
		printf("%c", (a & 1) ? '=' : ' ');
		a >>= 1;
	}
	printf("\n");
}
#endif

#define TERMINAL_ADDRESS (LOW_MEMORY_MAP_START + 0xB8000)

#if 1
Spinlock terminalLock; 
Spinlock printLock; 
#else
Mutex terminalLock; 
Mutex printLock; 
#endif

void DebugWriteCharacter(uintptr_t character);

#ifdef ARCH_X86_64
bool printToTerminal, printToSerialOutput = true, printToDebugger = false;
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

	if (printToDebugger) {
		DebugWriteCharacter(character);
	}
}
#endif

size_t debugRows, debugColumns, debugCurrentRow, debugCurrentColumn;

void DebugWriteCharacter(uintptr_t character) {
	uintptr_t &row = debugCurrentRow;
	uintptr_t &column = debugCurrentColumn;

	if (character == '\n') {
		debugCurrentRow++;
		debugCurrentColumn = 0;
		return;
	}

	if (character > 127) character = ' ';
	if (row >= debugRows) return;
	if (column >= debugColumns) return;

	uint64_t a = font[character];

	for (int j = 0; j < 8; j++) {
		for (int i = 0; i < 8; i++) {
			uint8_t bit = (a & 1);

			switch (graphics.colorMode) {
				case VIDEO_COLOR_24_RGB: {
					uintptr_t y = row * 18 + j * 2;
					uintptr_t x = column * 18 + i * 2;
					uint8_t c = bit ? 0xEE : 0x44;
					graphics.linearBuffer[y * graphics.resX * 3 + x * 3 + 0] = c;
					graphics.linearBuffer[y * graphics.resX * 3 + x * 3 + 1] = c;
					graphics.linearBuffer[y * graphics.resX * 3 + x * 3 + 2] = c;
					graphics.linearBuffer[y * graphics.resX * 3 + (x + 1) * 3 + 0] = c;
					graphics.linearBuffer[y * graphics.resX * 3 + (x + 1) * 3 + 1] = c;
					graphics.linearBuffer[y * graphics.resX * 3 + (x + 1) * 3 + 2] = c;
					graphics.linearBuffer[(y + 1) * graphics.resX * 3 + x * 3 + 0] = c;
					graphics.linearBuffer[(y + 1) * graphics.resX * 3 + x * 3 + 1] = c;
					graphics.linearBuffer[(y + 1) * graphics.resX * 3 + x * 3 + 2] = c;
					graphics.linearBuffer[(y + 1) * graphics.resX * 3 + (x + 1) * 3 + 0] = c;
					graphics.linearBuffer[(y + 1) * graphics.resX * 3 + (x + 1) * 3 + 1] = c;
					graphics.linearBuffer[(y + 1) * graphics.resX * 3 + (x + 1) * 3 + 2] = c;
				} break;
			
				default:;
			}

			a >>= 1;
		}
	}

	debugCurrentColumn++;

	if (debugCurrentColumn == debugColumns) {
		debugCurrentRow++;
		debugCurrentColumn = 0;
	}
}

void KernelPanic(const char *format, ...) {
	printToDebugger = true;
	debugRows = graphics.resY / 18;
	debugColumns = graphics.resX / 18;

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
	Print("Trace: %x\n", __builtin_return_address(0));
	Print("RSP: %x\n", ProcessorGetRSP());
	Print("Memory: %x/%x\n", pmm.pagesAllocated, pmm.startPageCount);

	{
		Print("Threads:\n");

		LinkedItem<Thread> *item = scheduler.allThreads.firstItem;

		while (item) {
			Thread *thread = item->thisItem;

			if (thread->type == THREAD_NORMAL) {
				Print("- %d %d %x ", thread->id, thread->state, thread);

				if (thread->state == THREAD_WAITING_EVENT) {
					Print("ev %d %x", thread->blockingEventCount, thread->blockingEvents[0]);
				} else if (thread->state == THREAD_WAITING_MUTEX) {
					Print("mu %x", thread->blockingMutex);
				}

				Print("\n");
			}

			item = item->nextItem;
		}
	}

	ProcessorHalt();
}

#if 1
#define ENABLE_OUTPUT
#endif

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
#define MINIMUM_PRINT_LEVEL LOG_VERBOSE

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

#endif
