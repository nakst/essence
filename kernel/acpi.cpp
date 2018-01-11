#ifdef IMPLEMENTATION

#define SIGNATURE_RSDP 0x2052545020445352

#define SIGNATURE_RSDT 0x54445352
#define SIGNATURE_XSDT 0x54445358
#define SIGNATURE_MADT 0x43495041

struct RootSystemDescriptorPointer {
	uint64_t signature;
	uint8_t checksum;
	char OEMID[6];
	uint8_t revision;
	uint32_t rsdtAddress;
	uint32_t length;
	uint64_t xsdtAddress;
	uint8_t extendedChecksum;
	uint8_t reserved[3];
};

struct ACPIDescriptorTable {
#define ACPI_DESCRIPTOR_TABLE_HEADER_LENGTH 36
	uint32_t signature;
	uint32_t length;
	uint64_t id;
	uint64_t tableID;
	uint32_t oemRevision;
	uint32_t creatorID;
	uint32_t creatorRevision;
};

struct MultipleAPICDescriptionTable {
	uint32_t lapicAddress; 
	uint32_t flags;
};

struct ACPIProcessor {
	uint8_t processorID, kernelProcessorID;
	uint8_t apicID;
	bool bootstrapProcessor;
	bool started;
	void **kernelStack;
};

struct ACPIIoApic {
	uint32_t ReadRegister(uint32_t reg);
	void WriteRegister(uint32_t reg, uint32_t value);

	uint8_t id;
	uint32_t address;
	uint32_t gsiBase;
};

uint32_t ACPIIoApic::ReadRegister(uint32_t reg) {
	uint32_t volatile *addr = (uint32_t volatile *) (LOW_MEMORY_MAP_START + address);
	addr[0] = reg; return addr[4];
}

void ACPIIoApic::WriteRegister(uint32_t reg, uint32_t value) {
	uint32_t volatile *addr = (uint32_t volatile *) (LOW_MEMORY_MAP_START + address);
	addr[0] = reg; addr[4] = value;
}

struct ACPIInterruptOverride {
	uint8_t sourceIRQ;
	uint32_t gsiNumber;
	bool activeLow, levelTriggered;
};

struct ACPILapicNMI {
	uint8_t processor; // 0xFF for all processors
	uint8_t lintIndex;
	bool activeLow, levelTriggered;
};

struct ACPILapic {
	uint32_t ReadRegister(uint32_t reg);
	void EndOfInterrupt();
	void WriteRegister(uint32_t reg, uint32_t value);
	void NextTimer(size_t ms);

	volatile uint32_t *address;
	size_t ticksPerMs;
};

void ACPILapic::NextTimer(size_t ms) {
	WriteRegister(0x320 >> 2, TIMER_INTERRUPT | (1 << 17)); 
	WriteRegister(0x380 >> 2, ticksPerMs * ms); 
}

void ACPILapic::EndOfInterrupt() {
	WriteRegister(0xB0 >> 2, 0);
}

uint32_t ACPILapic::ReadRegister(uint32_t reg) {
	return address[reg];
}

void ACPILapic::WriteRegister(uint32_t reg, uint32_t value) {
	address[reg] = value;
}

struct ACPI {
	void Initialise();
	void FindRootSystemDescriptorPointer();
	void *FindTable(uint32_t tableSignature, ACPIDescriptorTable **header = nullptr);

	size_t processorCount;
	size_t ioapicCount;
	size_t interruptOverrideCount;
	size_t lapicNMICount;

	ACPIProcessor processors[256];
	ACPIProcessor *bootstrapProcessor;
	ACPIIoApic ioApics[16];
	ACPIInterruptOverride interruptOverrides[256];
	ACPILapicNMI lapicNMIs[32];
	ACPILapic lapic;

	private:

	RootSystemDescriptorPointer *rsdp;
	ACPIDescriptorTable *sdt; bool isXSDT;

#define ACPI_MAX_TABLE_LENGTH (16384)
#define ACPI_MAX_TABLES (512)
	ACPIDescriptorTable *tables[ACPI_MAX_TABLES];
	size_t tablesCount;
};

ACPI acpi;

#ifdef ARCH_X86_64
void ACPI::FindRootSystemDescriptorPointer() {
	PhysicalMemoryRegion searchRegions[2];

	searchRegions[0].baseAddress = (uintptr_t) (((uint16_t *) LOW_MEMORY_MAP_START)[0x40E] << 4) + LOW_MEMORY_MAP_START;
	searchRegions[0].pageCount = 0x400;
	searchRegions[1].baseAddress = (uintptr_t) 0xE0000 + LOW_MEMORY_MAP_START;
	searchRegions[1].pageCount = 0x20000;

	for (uintptr_t i = 0; i < 2; i++) {
		for (uintptr_t address = searchRegions[i].baseAddress;
				address < searchRegions[i].baseAddress + searchRegions[i].pageCount;
				address += 16) {
			rsdp = (RootSystemDescriptorPointer *) address;

			if (rsdp->signature != SIGNATURE_RSDP) {
				continue;
			}

			if (rsdp->revision == 0) {
				if (SumBytes((uint8_t *) rsdp, 20)) {
					continue;
				}

				return;
			} else if (rsdp->revision == 2) {
				if (SumBytes((uint8_t *) rsdp, sizeof(RootSystemDescriptorPointer))) {
					continue;
				}

				return;
			}
		}
	}

	// We didn't find the RSDP.
	rsdp = nullptr;
}
#endif

void ACPI::Initialise() {
	ZeroMemory(this, sizeof(ACPI));
	FindRootSystemDescriptorPointer();

	if (rsdp) {
		if (rsdp->revision == 2 && rsdp->xsdtAddress) {
			isXSDT = true;
			sdt = (ACPIDescriptorTable *) rsdp->xsdtAddress;
		} else {
			isXSDT = false;
			sdt = (ACPIDescriptorTable *) (uintptr_t) rsdp->rsdtAddress;
		}

		sdt = (ACPIDescriptorTable *) kernelVMM.Allocate("ACPI", ACPI_MAX_TABLE_LENGTH, vmmMapAll, VMM_REGION_PHYSICAL, (uintptr_t) sdt);
	} else {
		KernelPanic("ACPI::Initialise - Could not find supported root system descriptor pointer.\nACPI support is required.\n");
	}

	if (((sdt->signature == SIGNATURE_XSDT && isXSDT) || (sdt->signature == SIGNATURE_RSDT && !isXSDT)) 
			&& sdt->length < ACPI_MAX_TABLE_LENGTH && !SumBytes((uint8_t *) sdt, sdt->length)) {
		tablesCount = (sdt->length - sizeof(ACPIDescriptorTable)) >> (isXSDT ? 3 : 2);

		if (tablesCount >= ACPI_MAX_TABLES || tablesCount < 1) {
			KernelPanic("ACPI::Initialise - The system descriptor table contains an unsupported number of tables (%d).\n", tablesCount);
		} 

		uintptr_t tableListAddress = (uintptr_t) sdt + ACPI_DESCRIPTOR_TABLE_HEADER_LENGTH;

		for (uintptr_t i = 0; i < tablesCount; i++) {
			uintptr_t address;

			if (isXSDT) {
				address = ((uint64_t *) tableListAddress)[i];
			} else {
				address = ((uint32_t *) tableListAddress)[i];
			}

			tables[i] = (ACPIDescriptorTable *) kernelVMM.Allocate("ACPI", ACPI_MAX_TABLE_LENGTH, vmmMapAll, VMM_REGION_PHYSICAL, address);

			if (tables[i]->length > ACPI_MAX_TABLE_LENGTH || SumBytes((uint8_t *) tables[i], tables[i]->length)) {
				KernelPanic("ACPI::Initialise - ACPI table %d with signature %s was invalid or unsupported.\n", i, 4, &tables[i]->signature);
			}
		}
	} else {
		KernelPanic("ACPI::Initialise - Could not find a valid or supported system descriptor table.\nACPI support is required.\n");
	}

#ifdef ARCH_X86_64
	{ 
		// Set up the APIC.
		
		ACPIDescriptorTable *header;
		MultipleAPICDescriptionTable *madt = (MultipleAPICDescriptionTable *) FindTable(SIGNATURE_MADT, &header);

		if (!madt) {
			KernelPanic("ACPI::Initialise - Could not find the MADT table.\nThis is required to use the APIC.\n");
		}

		uintptr_t length = header->length - ACPI_DESCRIPTOR_TABLE_HEADER_LENGTH - sizeof(MultipleAPICDescriptionTable);
		uintptr_t startLength = length;
		uint8_t *data = (uint8_t *) (madt + 1);

		lapic.address = (uint32_t volatile *) (LOW_MEMORY_MAP_START + madt->lapicAddress);

		while (length && length <= startLength) {
			uint8_t entryType = data[0];
			uint8_t entryLength = data[1];

			switch (entryType) {
				case 0: {
					// A processor and its LAPIC.
					if ((data[4] & 1) == 0) goto nextEntry;
					ACPIProcessor *processor = processors + processorCount;
					processor->processorID = data[2];
					processor->apicID = data[3];
					processorCount++;
				} break;

				case 1: {
					// An I/O APIC.
					ioApics[ioapicCount].id = data[2];
					ioApics[ioapicCount].address = ((uint32_t *) data)[1];
					ioApics[ioapicCount].gsiBase = ((uint32_t *) data)[2];
					ioapicCount++;
				} break;

				case 2: {
					// An interrupt source override structure.
					interruptOverrides[interruptOverrideCount].sourceIRQ = data[3];
					interruptOverrides[interruptOverrideCount].gsiNumber = ((uint32_t *) data)[1];
					interruptOverrides[interruptOverrideCount].activeLow = (data[8] & 2) ? true : false;
					interruptOverrides[interruptOverrideCount].levelTriggered = (data[8] & 8) ? true : false;
					interruptOverrideCount++;
				} break;

				case 4: {
					// A non-maskable interrupt.
					lapicNMIs[lapicNMICount].processor = data[2];
					lapicNMIs[lapicNMICount].lintIndex = data[5];
					lapicNMIs[lapicNMICount].activeLow = (data[3] & 2) ? true : false;
					lapicNMIs[lapicNMICount].levelTriggered = (data[3] & 8) ? true : false;
					lapicNMICount++;
				} break;

				default: {
					KernelLog(LOG_WARNING, "ACPI::Initialise - Found unknown entry of type %d in MADT\n", entryType);
				} break;
			}

			nextEntry:
			length -= entryLength;
			data += entryLength;
		}

		if (processorCount > 256 || ioapicCount > 16 || interruptOverrideCount > 256 || lapicNMICount > 32) {
			KernelPanic("ACPI::KernelPanic - Invalid number of processors (%d/%d), \n"
				    "                    I/O APICs (%d/%d), interrupt overrides (%d/%d)\n"
				    "                    and LAPIC NMIs (%d/%d)\n", 
				    processorCount, 256, ioapicCount, 16, interruptOverrideCount, 256, lapicNMICount, 32);
		}

		uint8_t bootstrapLapicID = lapic.ReadRegister(0x20 >> 2) >> 24;

		for (uintptr_t i = 0; i < processorCount; i++) {
			if (processors[i].apicID == bootstrapLapicID) {
				// That's us!
				bootstrapProcessor = processors + i;
				bootstrapProcessor->started 
					= bootstrapProcessor->bootstrapProcessor 
					= true;
			}
		}

		if (!bootstrapProcessor) {
			KernelPanic("ACPI::Initialise - Could not find the bootstrap processor\n");
		}

#define AP_TRAMPOLINE 0x10000

		// Put the trampoline code in memory.
		CopyMemory((void *) (LOW_MEMORY_MAP_START + AP_TRAMPOLINE),
				(void *) ProcessorAPStartup,
				0x1000); // Assume that the AP trampoline code is 4KB.

		// Put the paging table location at AP_TRAMPOLINE + 0xFF0.
		uint64_t cr3 = ProcessorReadCR3();
		CopyMemory((void *) (LOW_MEMORY_MAP_START + AP_TRAMPOLINE + 0xFF0),
				&cr3, sizeof(cr3));

		// Put the 64-bit GDTR at AP_TRAMPOLINE + 0xFE0.
		CopyMemory((void *) (LOW_MEMORY_MAP_START + AP_TRAMPOLINE + 0xFE0),
				(void *) processorGDTR, 0x10);

		// Put the GDT at AP_TRAMPOLINE + 0x1000.
		CopyMemory((void *) (LOW_MEMORY_MAP_START + AP_TRAMPOLINE + 0x1000),
				(void *) gdt_data, 0x1000);

		// Put the startup flag at AP_TRAMPOLINE + 0xFC0
		uint8_t volatile *startupFlag = (uint8_t *) (LOW_MEMORY_MAP_START + AP_TRAMPOLINE + 0xFC0);

		// Temporarily identity map a 2 pages in at 0x10000.
		{
			kernelVMM.virtualAddressSpace.lock.Acquire();
			kernelVMM.virtualAddressSpace.Map(0x10000, 0x10000, false);
			kernelVMM.virtualAddressSpace.Map(0x11000, 0x11000, false);
			kernelVMM.virtualAddressSpace.lock.Release();
		} 

		scheduler.processors = 1;

		for (uintptr_t i = 0; i < processorCount; i++) {
			ACPIProcessor *processor = processors + i;

			if (processor->bootstrapProcessor == false) {
				// Clear the startup flag.
				*startupFlag = 0;

				// Put the stack at AP_TRAMPOLINE + 0xFD0.
				void *stack = (void *) ((uintptr_t) kernelVMM.Allocate("StartupStack", 0x4000) + 0x4000);
				CopyMemory((void *) (LOW_MEMORY_MAP_START + AP_TRAMPOLINE + 0xFD0),
						&stack, sizeof(void *));
				// KernelLog(LOG_VERBOSE, "Trampoline stack: %x->%x\n", stack, (uintptr_t) stack + 0x4000);

				// Send an INIT IPI.
				lapic.WriteRegister(0x310 >> 2, processor->apicID << 24);
				lapic.WriteRegister(0x300 >> 2, 0x4500);
				for (uintptr_t i = 0; i < 10; i++) Delay1MS();

				// Send a startup IPI.
				lapic.WriteRegister(0x310 >> 2, processor->apicID << 24);
				lapic.WriteRegister(0x300 >> 2, 0x4600 | (AP_TRAMPOLINE >> PAGE_BITS));
				for (uintptr_t i = 0; i < 10; i++) Delay1MS();

				if (startupFlag) {
					// The processor started correctly.
					processor->started = true;
				} else {
					// Send a startup IPI, again.
					lapic.WriteRegister(0x310 >> 2, processor->apicID << 24);
					lapic.WriteRegister(0x300 >> 2, 0x4600 | (AP_TRAMPOLINE >> PAGE_BITS));
					for (uintptr_t i = 0; i < 1000; i++) Delay1MS(); // Wait longer this time.

					if (startupFlag) {
						// The processor started correctly.
						processor->started = true;
					} else {
						// The processor could not be started.
						KernelLog(LOG_WARNING, "ACPI::Initialise - Could not start processor %d\n", processor->processorID);
						continue;
					}
				}

				for (uintptr_t i = 0; i < 1000 && *startupFlag != 2; i++) Delay1MS();

				if (*startupFlag == 2) {
					// The processor started!
					scheduler.processors++;
				} else {
					// The processor did not report it completed initilisation, worringly.
					// Don't let it continue.

					processor->started = false;
					KernelLog(LOG_WARNING, "ACPI::Initialise - Could not initialise processor %d\n", processor->processorID);

					// TODO Send IPI to stop the processor?
				}
			}
		}

		SetupProcessor2();
		
		// Remove the identity pages needed for the trampoline code.
		{
			kernelVMM.virtualAddressSpace.lock.Acquire();
			kernelVMM.virtualAddressSpace.Remove(0x10000, 2);
			kernelVMM.virtualAddressSpace.lock.Release();
		}

		// Set up the LAPIC's time
		ProcessorDisableInterrupts();
		acpi.lapic.WriteRegister(0x380 >> 2, (uint32_t) -1); 
		for (int i = 0; i < 8; i++) Delay1MS(); // Average over 8ms
		acpi.lapic.ticksPerMs = ((uint32_t) -1 - acpi.lapic.ReadRegister(0x390 >> 2)) >> 4;
		osRandomByteSeed ^= acpi.lapic.ReadRegister(0x390 >> 2);
		ProcessorEnableInterrupts();
	}
#endif
}

void *ACPI::FindTable(uint32_t tableSignature, ACPIDescriptorTable **header) {
	for (uintptr_t i = 0; i < tablesCount; i++) {
		if (tables[i]->signature == tableSignature) {
			if (header) {
				*header = tables[i];
			}

			return (uint8_t *) tables[i] + ACPI_DESCRIPTOR_TABLE_HEADER_LENGTH;
		}
	}

	// The system does not have the ACPI table.
	return nullptr;
}

#endif
