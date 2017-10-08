#ifdef IMPLEMENTATION

void Delay1MS() {
	ProcessorOut8(0x43, 0x30);
	ProcessorOut8(0x40, 0xA9);
	ProcessorOut8(0x40, 0x04);

	while (true) {
		ProcessorOut8(0x43, 0xE2);

		if (ProcessorIn8(0x40) & (1 << 7)) {
			break;
		}
	}
}

#define IRQ_BASE 0x50
IRQHandler irqHandlers[0x20][0x10];
size_t usedIrqHandlers[0x20];

bool RegisterIRQHandler(uintptr_t interrupt, IRQHandler handler) {
	scheduler.lock.Acquire();
	Defer(scheduler.lock.Release());

	// Work out which interrupt the IoApic will sent to the processor.
	// TODO Use the upper 4 bits for IRQ priority.
	uintptr_t thisProcessorIRQ = interrupt + IRQ_BASE;

	// Register the IRQ handler.
	if (interrupt > 0x20) KernelPanic("RegisterIRQHandler - Unexpected IRQ %d\n", interrupt);
	if (usedIrqHandlers[interrupt] == 0x10) {
		// There are too many overloaded interrupts.
		return false;
	}
	irqHandlers[interrupt][usedIrqHandlers[interrupt]] = handler;
	usedIrqHandlers[interrupt]++;

	bool activeLow = false;
	bool levelTriggered = false;

	// If there was an interrupt override entry in the MADT table,
	// then we'll have to use that number instead.
	for (uintptr_t i = 0; i < acpi.interruptOverrideCount; i++) {
		ACPIInterruptOverride *interruptOverride = acpi.interruptOverrides + i;
		if (interruptOverride->sourceIRQ == interrupt) {
			interrupt = interruptOverride->gsiNumber;
			activeLow = interruptOverride->activeLow;
			levelTriggered = interruptOverride->levelTriggered;
			break;
		}
	}

	ACPIIoApic *ioApic;
	bool foundIoApic = false;

	// Look for the IoApic to which this interrupt is sent.
	for (uintptr_t i = 0; i < acpi.ioapicCount; i++) {
		ioApic = acpi.ioApics + i;

		if (interrupt >= ioApic->gsiBase 
				&& interrupt < (ioApic->gsiBase + (0xFF & (ioApic->ReadRegister(1) >> 16)))) {
			foundIoApic = true;
			interrupt -= ioApic->gsiBase;
			break;
		}
	}

	// We couldn't find the IoApic that handles this interrupt.
	if (!foundIoApic) {
		return false;
	}

	// A normal priority interrupt.
	uintptr_t redirectionTableIndex = interrupt * 2 + 0x10;
	uint32_t redirectionEntry = thisProcessorIRQ;
	if (activeLow) redirectionEntry |= (1 << 13);
	if (levelTriggered) redirectionEntry |= (1 << 15);

	// Mask the interrupt while we modify the entry.
	ioApic->WriteRegister(redirectionTableIndex, 1 << 16);

	// Send the interrupt to the processor that registered the interrupt.
	ioApic->WriteRegister(redirectionTableIndex + 1, ProcessorGetLocalStorage()->acpiProcessor->apicID); 
	ioApic->WriteRegister(redirectionTableIndex, redirectionEntry);

	return true;
}

Spinlock ipiLock;

void ProcessorSendIPI(uintptr_t interrupt, bool nmi) {
	ipiLock.AssertLocked();
	ipiVector = interrupt;

#if 0
	// We now send IPIs at a special priority that ProcessorDisableInterrupts doesn't mask.
	// Therefore, this isn't a problem.

	if (!nmi && ProcessorGetLocalStorage()->spinlockCount) {
		KernelPanic("ProcessorSendIPI - Cannot send IPIs with spinlocks acquired.\n");
	}
#endif

	for (uintptr_t i = 0; i < acpi.processorCount; i++) {
		if (acpi.processors + i == ProcessorGetLocalStorage()->acpiProcessor) {
			// Don't send the IPI to this processor.
			continue;
		}

		uint32_t destination = acpi.processors[i].apicID << 24;
		uint32_t command = interrupt | (1 << 14) | (nmi ? 0x400 : 0);
		acpi.lapic.WriteRegister(0x310 >> 2, destination);
		acpi.lapic.WriteRegister(0x300 >> 2, command); 

		// Wait for the interrupt to be sent.
		while (acpi.lapic.ReadRegister(0x300 >> 2) & (1 << 12));
	}
}

void NextTimer(size_t ms) {
	acpi.lapic.NextTimer(ms);
}

extern "C" void SetupProcessor2() {
	// Find the processor for the current LAPIC.
	
	uint8_t lapicID = acpi.lapic.ReadRegister(0x20 >> 2) >> 24;
	ACPIProcessor *processor = nullptr;

	for (uintptr_t i = 0; i < acpi.processorCount; i++) {
		if (acpi.processors[i].apicID == lapicID) {
			processor = acpi.processors + i;
			break;
		}
	}

	if (!processor) {
		KernelPanic("SetupProcessor2 - Could not find processor for current local APIC ID %d\n", lapicID);
	}

	// Setup the local interrupts for the current processor.
		
	for (uintptr_t i = 0; i < acpi.lapicNMICount; i++) {
		if (acpi.lapicNMIs[i].processor == 0xFF
				|| acpi.lapicNMIs[i].processor == processor->processorID) {
			uint32_t registerIndex = (0x350 + (acpi.lapicNMIs[i].lintIndex << 4)) >> 2;
			uint32_t value = 2 | (1 << 10); // NMI exception interrupt vector.
			if (acpi.lapicNMIs[i].activeLow) value |= 1 << 13;
			if (acpi.lapicNMIs[i].levelTriggered) value |= 1 << 15;
			acpi.lapic.WriteRegister(registerIndex, value);
		}
	}

	// Configure the LAPIC's timer.

	acpi.lapic.WriteRegister(0x3E0 >> 2, 2); // Divisor = 16

	// Create the processor's local storage.

	CPULocalStorage *localStorage = (CPULocalStorage *) kernelVMM.Allocate(sizeof(CPULocalStorage), vmmMapAll);
	ProcessorSetLocalStorage(localStorage);

	// Find the ACPIProcessor for the current processor.
	localStorage->acpiProcessor = processor;

	// Setup a GDT and TSS for the processor.
	{
		uintptr_t memoryPhysical = pmm.AllocatePage();
		void *memory = kernelVMM.Allocate(4096, vmmMapAll, vmmRegionPhysical, memoryPhysical);

		uintptr_t gdtPhysical = memoryPhysical +    0;
		uintptr_t tssPhysical = memoryPhysical + 2048;

		(void) gdtPhysical;
		(void) tssPhysical;

		uint32_t *gdt = (uint32_t *) ((uint8_t *) memory +    0);
		void *bootstrapGDT = (void *) (((uint64_t *) ((uint16_t *) processorGDTR + 1))[0]);
		CopyMemory(gdt, bootstrapGDT, 2048);

		uint32_t *tss = (uint32_t *) ((uint8_t *) memory + 2048);
		processor->kernelStack = (void **) (tss + 1);

		ProcessorInstallTSS(gdt, tss);
	}

	// Create the idle thread for this processor.
	scheduler.CreateProcessorThreads();
}

const char *exceptionInformation[] = {
	"0x00: Divide Error (Fault)",
	"0x01: Debug Exception (Fault/Trap)",
	"0x02: Non-Maskable External Interrupt (Interrupt)",
	"0x03: Breakpoint (Trap)",
	"0x04: Overflow (Trap)",
	"0x05: BOUND Range Exceeded (Fault)",
	"0x06: Invalid Opcode (Fault)",
	"0x07: x87 Coprocessor Unavailable (Fault)",
	"0x08: Double Fault (Abort)",
	"0x09: x87 Coprocessor Segment Overrun (Fault)",
	"0x0A: Invalid TSS (Fault)",
	"0x0B: Segment Not Present (Fault)",
	"0x0C: Stack Protection (Fault)",
	"0x0D: General Protection (Fault)",
	"0x0E: Page Fault (Fault)",
	"0x0F: Reserved/Unknown",
	"0x10: x87 FPU Floating-Point Error (Fault)",
	"0x11: Alignment Check (Fault)",
	"0x12: Machine Check (Abort)",
	"0x13: SIMD Floating-Point Exception (Fault)",
	"0x14: Virtualization Exception (Fault)",
	"0x15: Reserved/Unknown",
	"0x16: Reserved/Unknown",
	"0x17: Reserved/Unknown",
	"0x18: Reserved/Unknown",
	"0x19: Reserved/Unknown",
	"0x1A: Reserved/Unknown",
	"0x1B: Reserved/Unknown",
	"0x1C: Reserved/Unknown",
	"0x1D: Reserved/Unknown",
	"0x1E: Reserved/Unknown",
	"0x1F: Reserved/Unknown",
};

void ContextSanityCheck(InterruptContext *context) {
	if (context->cs > 0x100 || context->ds > 0x100 || context->ss > 0x100 || context->rip == 0) {
		KernelPanic("InterruptHandler - Corrupt context (%x/%x/%x)\nRIP = %x, RSP = %x\n", context->cs, context->ds, context->ss, context->rip, context->rsp);
	}
}

extern "C" void InterruptHandler(InterruptContext *context) {
	if (scheduler.panic && context->interruptNumber != 2) {
		return;
	}

	if (ProcessorAreInterruptsEnabled()) {
		KernelPanic("InterruptHandler - Interrupts were enabled at the start of an interrupt handler.\n");
	}

	CPULocalStorage *local = ProcessorGetLocalStorage();
	uintptr_t interrupt = context->interruptNumber;

	if (local) {
		local->interruptContexts[local->interruptRecurseCount] = context;
		local->interruptRecurseCount++;

		if (local->interruptRecurseCount == 4) {
			while (true);
			ProcessorMagicBreakpoint();
			KernelPanic("Interrupt recurse failure on CPU %d: local->interruptContexts[0]->rip = %x.\n", local->processorID, local->interruptContexts[0]->rip);
			while (true);
		}
	}

	if (interrupt < 0x20) {
		// If we received a non-maskable interrupt, idle.
		if (interrupt == 2) ProcessorIdle();

		bool supervisor = (context->cs & 3) == 0;

		if (!supervisor) {
			if (context->cs != 0x5B && context->cs != 0x6B) {
				KernelPanic("InterruptHandler - Unexpected value of CS 0x%X\n", context->cs);
			}

			if (interrupt == 14) {
				if (local && local->spinlockCount) {
					KernelPanic("InterruptHandler - Page fault occurred with spinlock acquired.");
				}

				bool success = HandlePageFault(context->cr2);

				if (success) {
					goto resolved;
				}
			}

			// TODO Usermode exceptions.
			KernelPanic("InterruptHandler - Exception (%z) in userland program.\nRIP = %x (CPU %d)\nX86_64 error codes: [err] %x, [cr2] %x\n", 
					exceptionInformation[interrupt], context->rip, local->processorID, context->errorCode, context->cr2);

			resolved:;
		} else {
			if (context->cs != 0x48) {
				KernelPanic("InterruptHandler - Unexpected value of CS 0x%X\n", context->cs);
			}

			if (interrupt == 14) {
				if (!HandlePageFault(context->cr2)) {
					goto fault;
				}
			} else {
				fault:
				KernelPanic("Unresolvable processor exception encountered in supervisor mode.\n%z\nRIP = %x (CPU %d)\nX86_64 error codes: [err] %x, [cr2] %x\n"
						"Stack: [rsp] %x, [rbp] %x\nThread ID = %d\n", 
						exceptionInformation[interrupt], context->rip, local ? local->processorID : -1, context->errorCode, context->cr2, 
						context->rsp, context->rbp, local && local->currentThread ? local->currentThread->id : -1);
			}
		}
	} else if (interrupt == 0xFF) {
		// Spurious interrupt (APIC), ignore.
	} else if (interrupt >= 0x20 && interrupt < 0x30) {
		// Spurious interrupt (PIC), ignore.
	} else if (interrupt >= 0xF0 && interrupt < 0xFE) {
		// IPI.

		if (ipiVector == TLB_SHOOTDOWN_IPI) {
			uintptr_t page = tlbShootdownVirtualAddress;

			for (uintptr_t i = 0; i < tlbShootdownPageCount; i++, page += PAGE_SIZE) {
				ProcessorInvalidatePage(page);
			}

			__sync_fetch_and_sub(&tlbShootdownRemainingProcessors, 1);
		} else if (ipiVector == KERNEL_PANIC_IPI) {
			ProcessorHalt();
		}

		acpi.lapic.EndOfInterrupt();
	} else if (local) {
		// IRQ.

		local->irqSwitchThread = false;

		if (interrupt == TIMER_INTERRUPT) {
			local->irqSwitchThread = true;
		} else {
			size_t overloads = usedIrqHandlers[interrupt - IRQ_BASE];
			bool handledInterrupt = false;

			for (uintptr_t i = 0; i < overloads; i++) {
				IRQHandler handler = irqHandlers[interrupt - IRQ_BASE][i];
				if (handler(interrupt - IRQ_BASE)) {
					handledInterrupt = true;

					break;
				}
			}

			bool rejectedByAll = !handledInterrupt;
			if (rejectedByAll) {
				// TODO Now what?
				Print("InterruptHandler - Unhandled IRQ %d, rejected by %d overloads\n", interrupt, overloads);
			}
		}

		if (local->irqSwitchThread) {
			scheduler.Yield(context);
			KernelPanic("InterruptHandler - Returned from Scheduler::Yield.\n");
			// TODO If the scheduler hasn't started we might return here. Think about how to detect this.
		}

		acpi.lapic.EndOfInterrupt();
	}

	// Sanity check.
	ContextSanityCheck(context);

	if (local) {
		local->interruptRecurseCount--;
	}

	if (ProcessorAreInterruptsEnabled()) {
		KernelPanic("InterruptHandler - Interrupts were enabled while returning from an interrupt handler.\n");
	}
}

extern "C" void PostContextSwitch(InterruptContext *context) {
	CPULocalStorage *local = ProcessorGetLocalStorage();

	local->interruptRecurseCount--;

	void *kernelStack = (void *) local->currentThread->kernelStack;
	*local->acpiProcessor->kernelStack = kernelStack;

	acpi.lapic.EndOfInterrupt();
	ContextSanityCheck(context);

	if (ProcessorAreInterruptsEnabled()) {
		KernelPanic("PostContextSwitch - Interrupts were enabled. (1)\n");
	}

	// We can only free the scheduler's spinlock when we are no longer using the stack
	// from the previous thread. See DoContextSwitch in x86_64.s.
	scheduler.lock.Release(true);

	if (ProcessorAreInterruptsEnabled()) {
		KernelPanic("PostContextSwitch - Interrupts were enabled. (2)\n");
	}

#if 0
	if ((ProcessorIn8(0x64) & 1)) {
		KernelPanic("Temporary debugger entry.\n");
	}
#endif
}

extern "C" uintptr_t Syscall(uintptr_t argument0, uintptr_t argument1, uintptr_t argument2, 
		uintptr_t unused, uintptr_t argument3, uintptr_t argument4) {
	(void) unused;

	return DoSyscall(argument0, argument1, argument2, argument3, argument4);
}

#endif
