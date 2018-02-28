#ifndef IMPLEMENTATION

#define AHCI_TIMEOUT (1000)
#define AHCI_SECTOR_SIZE (512)
#define AHCI_COMMAND_COUNT (32)
#define AHCI_DRIVE_COUNT (32)
#define AHCI_IDENTIFY (-1)

enum AHCIPacketType {
	AHCI_PACKET_TYPE_HOST_TO_DEVICE = 0x27,
	AHCI_PACKET_TYPE_DEVICE_TO_HOST = 0x34,
	AHCI_PACKET_TYPE_DMA_ACTIVATE = 0x39,
	AHCI_PACKET_TYPE_DMA_SETUP = 0x41,
	AHCI_PACKET_TYPE_DATA = 0x46,
	AHCI_PACKET_TYPE_BIST = 0x58,
	AHCI_PACKET_TYPE_PIO_SETUP = 0x5F,
	AHCI_PACKET_TYPE_SET_DEVICE_BITS = 0xA1,
};

struct AHCIPacketHostToDevice {
	uint8_t packetType;
	uint8_t portMultiplier:4;
	uint8_t _reserved0:3;
	uint8_t controlOrCommand:1;
	uint8_t command;
	uint8_t featureLow;
	
	uint8_t lba0;	
	uint8_t lba1;	
	uint8_t lba2;	
	uint8_t device;
	
	uint8_t lba3;	
	uint8_t lba4;	
	uint8_t lba5;	
	uint8_t featureHigh;
	
	uint8_t countLow;
	uint8_t countHigh;
	uint8_t icc;
	uint8_t control;
	
	uint8_t _reserved1[4];
};

struct AHCIPacketDeviceToHost {
	uint8_t packetType;
	uint8_t portMultiplier:4;
	uint8_t _reserved0:2;
	uint8_t interrupt:1;
	uint8_t _reserved1:1;
	uint8_t status;
	uint8_t error;
	
	uint8_t lba0;	
	uint8_t lba1;	
	uint8_t lba2;	
	uint8_t device;
	
	uint8_t lba3;	
	uint8_t lba4;	
	uint8_t lba5;	
	uint8_t _reserved2;
	
	uint8_t countLow;
	uint8_t countHigh;
	uint8_t _reserved3[2];
	
	uint8_t _reserved4[4];
};

struct AHCIPacketData {
	uint8_t packetType;
	uint8_t portMultiplier:4;
	uint8_t _reserved0:4;
	uint8_t _reserved1[2];

	// Data follows.
};

struct AHCIPacketPIOSetup {
	uint8_t packetType;
	uint8_t portMultiplier:4;
	uint8_t _reserved0:1;
	uint8_t transferDirection:1;
	uint8_t interrupt:1;
	uint8_t _reserved1:1;
	uint8_t status;
	uint8_t error;
	
	uint8_t lba0;	
	uint8_t lba1;	
	uint8_t lba2;	
	uint8_t device;
	
	uint8_t lba3;	
	uint8_t lba4;	
	uint8_t lba5;	
	uint8_t _reserved2;
	
	uint8_t countLow;
	uint8_t countHigh;
	uint8_t _reserved3;
	uint8_t newStatus;
	
	uint16_t transferCount;
	uint8_t _reserved4[2];
};

struct AHCIPacketDMASetup {
	uint8_t packetType;
	uint8_t portMultiplier:4;
	uint8_t _reserved0:1;
	uint8_t transferDirection:1;
	uint8_t interrupt:1;
	uint8_t autoActivate:1;
	uint8_t _reserved1[2];
	
	uint32_t dmaBufferIDLow;
	uint32_t dmaBufferIDHigh;
	uint32_t _reserved2;
	uint32_t dmaBufferOffset;
	uint32_t transferCount;
	uint32_t _reserved3;
};

struct AHCIPort {
	uint32_t commandListBaseAddressLow;
	uint32_t commandListBaseAddressHigh;
	uint32_t fisBaseAddressLow;
	uint32_t fisBaseAddressHigh;
	uint32_t interruptStatus;
	uint32_t interruptEnable;

	union {
		uint32_t command;
		uint32_t status;
	};

	uint32_t _reserved0;
	uint32_t taskFileData;
	uint32_t signature;
	uint32_t sataStatus;
	uint32_t sataControl;
	uint32_t sataError;
	uint32_t sataActive;
	uint32_t commandIssue;
	uint32_t sataNotification;
	uint32_t fisSwitchControl;
	uint32_t _reserved1[11];
	uint32_t _reserved2[4];
};

struct AHCIReceivedPacket {
	AHCIPacketDMASetup dmaSetup;
	uint8_t _pad0[4];
	
	AHCIPacketPIOSetup pioSetup;
	uint8_t _pad1[12];
	
	AHCIPacketDeviceToHost deviceToHost;
	uint8_t _pad2[4];
	
	uint8_t sdbfis[8];
	uint8_t ufis[64];
	uint8_t _reserved[0x100 - 0xA0];
};

struct AHCICommandHeader {
	uint8_t commandLength:5;
	uint8_t atapi:1;
	uint8_t write:1;
	uint8_t prefetchable:1;
	uint8_t reset:1;
	uint8_t bist:1;
	uint8_t clearBusy:1;
	uint8_t _reserved0:1;
	uint8_t portMultiplier:4;
	uint16_t prdEntryCount;
	
	uint32_t prdByteCount;
	
	uint32_t commandTableDescriptorLow;
	uint32_t commandTableDescriptorHigh;
	
	uint32_t _reserved1[4];
};

struct AHCIPRDTEntry {
	uint32_t targetAddressLow;
	uint32_t targetAddressHigh;
	uint32_t _reserved0;
	uint32_t byteCount:22;
	uint32_t _reserved1:9;
	uint32_t interruptOnCompletion:1;
};

struct AHCICommandTable {
	uint8_t commandPacket[64];
	uint8_t atapiCommand[16];
	uint8_t _reserved0[48];
	AHCIPRDTEntry prdtEntries[8];
};

struct AHCIHBA {
	uint32_t capabilities;
	uint32_t globalHostControl;
	uint32_t interruptStatus;
	uint32_t portImplemented;
	uint32_t version;
	uint32_t commandCompletionCoalescingControl;
	uint32_t commandCompletionCoalescingPorts;
	uint32_t enclosureManagementLocation;
	uint32_t enclosureManagementControl;
	uint32_t capabilitiesExtended;
	uint32_t biosHandoffStatus;
	
	uint8_t _reserved0[0xA0 - 0x2C];
	uint8_t _reserved1[0x100 - 0xA0];
	
	AHCIPort ports[32];
};

struct AHCIDrive {
	bool present;

	volatile AHCICommandHeader *commandList;
	volatile AHCIReceivedPacket *receivedPacket;
	volatile AHCICommandTable *commandTable;

	Mutex mutex;

	Semaphore available; // The number of available commands.
	volatile uint32_t commandsInUse; // Bitset.

	struct Device *device;
	uint64_t sectorCount;

	uintptr_t physicalBuffers[AHCI_COMMAND_COUNT];
	volatile void *buffers[AHCI_COMMAND_COUNT];

	Event receivedIRQ[AHCI_COMMAND_COUNT];
};

struct AHCIController {
	void Initialise();
	void RemoveBlockingPacket(struct IOPacket *packet);
	bool Access(struct IOPacket *packet, uintptr_t drive, uint64_t offset, size_t count, int operation, uint8_t *buffer); // Returns true on success.

	bool present;
	struct PCIDevice *pciDevice;
	AHCIDrive drives[AHCI_DRIVE_COUNT];
	volatile AHCIHBA *r;

	// Acquire to check/change a packet's driverState or modify the blocked packets lists.
	Mutex blockedPacketsMutex;
};

AHCIController ahci;

#else

bool AHCIIRQHandler(uintptr_t interruptIndex) {
	(void) interruptIndex;

	uint32_t pendingInterrupts = ahci.r->interruptStatus;
	Defer(ahci.r->interruptStatus = pendingInterrupts);

	if (!pendingInterrupts) {
		return false;
	}

	for (uint32_t i = 0; i < 32; i++) {
		if (pendingInterrupts & (1 << i)) {
			AHCIDrive *drive = ahci.drives + i;
			volatile AHCIPort *port = ahci.r->ports + i;

			if (!drive->present) {
				KernelLog(LOG_WARNING, "AHCIIRQHandler - Received IRQ for non-present drive on port %d.\n", i);
				continue;
			}

			uint32_t interruptStatus = port->interruptStatus;
			Defer(port->interruptStatus = interruptStatus);

			for (uintptr_t i = 0; i < AHCI_COMMAND_COUNT; i++) {
				if ((drive->commandsInUse & (1 << i)) 
						&& !(port->commandIssue & (1 << i))) {
					if (drive->receivedIRQ[i].state) continue;

					drive->receivedIRQ[i].Set();
				}
			}
		}
	}

	return true;
}

void AHCIRegisterController(PCIDevice *pciDevice) {
	KernelLog(LOG_VERBOSE, "AHCIRegisterController - Found AHCI controller.\n");

	ahci.pciDevice = pciDevice;
	ahci.present = true;

	Device *controller;

	{
		// Register the device.
		Device device = {};
		device.type = DEVICE_TYPE_AHCI_CONTROLLER;
		device.parent = DEVICE_PARENT_ROOT;
		controller = deviceManager.Register(&device);
	}

	// Memory map the registers.
	ahci.r = (AHCIHBA *) kernelVMM.Allocate("AHCI", sizeof(AHCIHBA), VMM_MAP_LAZY, VMM_REGION_PHYSICAL, pciDevice->baseAddresses[5], VMM_REGION_FLAG_NOT_CACHABLE, nullptr);

	if (!ahci.r) {
		KernelLog(LOG_WARNING, "AHCIDriver::Initialise - Could not allocate memory for ABAR.\n");
		return;
	}

	// Enable AHCI and IRQs.
	ahci.r->globalHostControl |= (1 << 31) | (1 << 1);

	// Reset any pending interrupts.
	ahci.r->interruptStatus = ahci.r->interruptStatus;

	size_t drivesFound = 0;

	for (uintptr_t i = 0; i < AHCI_DRIVE_COUNT; i++) {
		AHCIDrive *drive = ahci.drives + i;
		volatile AHCIPort *port = ahci.r->ports + i;

		uint32_t driveStatus = port->sataStatus;

		if ((driveStatus & 0xF) == 3 && (driveStatus & 0xF0) && (driveStatus & 0xF00) == 0x100) {
			// The drive is present.
		} else {
			continue;
		}

		if (port->signature == 0x00000101) {
			// Normal SATA drive.
		} else {
			continue;
		}

		drive->present = true;
		drive->available.Return(AHCI_COMMAND_COUNT);
		drivesFound++;
	}

	if (!drivesFound) {
		// We didn't find any drives.
		return;
	}

	// Register our IRQ handler.
	RegisterIRQHandler(pciDevice->interruptLine, AHCIIRQHandler);

	uintptr_t commandListPage = 0;
	uintptr_t receivedPacketPage = 0;
	uint8_t *commandListPageVirtual = nullptr;
	uint8_t *receivedPacketPageVirtual = nullptr;

	// Setup present drives.
	for (uintptr_t i = 0; i < AHCI_DRIVE_COUNT; i++) {
		AHCIDrive *drive = ahci.drives + i;
		volatile AHCIPort *port = ahci.r->ports + i;

		if (!drive->present) {
			continue;
		}

		// Allocate necessary structures for the port.

		if ((i % (PAGE_SIZE / 1024)) == 0) {
			commandListPage = pmm.AllocatePage(true);
			commandListPageVirtual = (uint8_t *) kernelVMM.Allocate("AHCI", sizeof(PAGE_SIZE), VMM_MAP_LAZY, VMM_REGION_PHYSICAL, commandListPage, VMM_REGION_FLAG_NOT_CACHABLE, nullptr);
		}

		if ((i % (PAGE_SIZE / 256)) == 0) {
			receivedPacketPage = pmm.AllocatePage(true);
			receivedPacketPageVirtual = (uint8_t *) kernelVMM.Allocate("AHCI", sizeof(PAGE_SIZE), VMM_MAP_LAZY, VMM_REGION_PHYSICAL, receivedPacketPage, VMM_REGION_FLAG_NOT_CACHABLE, nullptr);
		}

		// Start a timeout.
		Timer timeout = {};
		timeout.Set(AHCI_TIMEOUT, false);
		Defer(timeout.Remove());

		// Stop the command engine.
		port->command &= ~0x11;
		while (port->status & 0xC000 && !timeout.event.Poll());
		if (timeout.event.Poll()) { drive->present = false; continue; }

		// Set the addresses...

		port->commandListBaseAddressLow = (uint32_t) (commandListPage >> 0);
		port->commandListBaseAddressHigh = (uint32_t) (commandListPage >> 32);
		drive->commandList = (AHCICommandHeader *) commandListPageVirtual;

		port->fisBaseAddressLow = (uint32_t) (receivedPacketPage >> 0);
		port->fisBaseAddressHigh = (uint32_t) (receivedPacketPage >> 32);
		drive->receivedPacket = (AHCIReceivedPacket *) receivedPacketPageVirtual;

		uintptr_t commandTablePage = pmm.AllocatePage(true);
		drive->commandTable = (AHCICommandTable *) kernelVMM.Allocate("AHCI", sizeof(PAGE_SIZE), VMM_MAP_LAZY, VMM_REGION_PHYSICAL, commandTablePage, VMM_REGION_FLAG_NOT_CACHABLE, nullptr);

		for (uintptr_t i = 0; i < AHCI_COMMAND_COUNT; i++) {
			uintptr_t physicalBuffer = pmm.AllocateContiguous64KB();
			void *buffer = kernelVMM.Allocate("AHCI", 65536, VMM_MAP_ALL, VMM_REGION_PHYSICAL, physicalBuffer, VMM_REGION_FLAG_NOT_CACHABLE, nullptr);

			drive->physicalBuffers[i] = physicalBuffer;
			drive->buffers[i] = buffer;

			volatile AHCICommandHeader *command = drive->commandList + i;
			command->commandTableDescriptorLow = (uint32_t) ((commandTablePage + 256 * i) >> 0); 
			command->commandTableDescriptorHigh = (uint32_t) ((commandTablePage + 256 * i) >> 32);
			command->prdEntryCount = 8; 
		}

		// Increment the position of these pages.
		commandListPage += 1024;
		receivedPacketPage += 256;
		commandListPageVirtual += 1024;
		receivedPacketPageVirtual += 256;

		// Restart the command engine.
		port->command |= 0x11;

		// Clear any remaining interrupts, and enable them.
		port->interruptStatus = port->interruptStatus; 
		port->interruptEnable |= 0xFD800001; 

		// Get the IDENTIFY data.
		uint16_t identifyData[256];
		bool success = ahci.Access(nullptr, i, 0, 512, AHCI_IDENTIFY, (uint8_t *) identifyData);
		if (!success) { drive->present = false; continue; }

		// Work out the number of sectors in the drive.
		uint32_t lba28Sectors = ((uint32_t) identifyData[60] << 0) + ((uint32_t) identifyData[61] << 16);
		uint64_t lba48Sectors = ((uint64_t) identifyData[100] << 0) + ((uint64_t) identifyData[101] << 16) +
			((uint64_t) identifyData[102] << 32) + ((uint64_t) identifyData[103] << 48);
		bool supportsLBA48 = lba48Sectors && (identifyData[83] & 0x40);
		uint64_t sectors = supportsLBA48 ? lba48Sectors : lba28Sectors;
		drive->sectorCount = sectors;
		if (!sectors) { drive->present = false; continue; }

		// Register the drive!
		{
			Device device = {};
			device.parent = controller;
			device.type = DEVICE_TYPE_BLOCK;
			device.block.driveID = i;
			device.block.sectorSize = AHCI_SECTOR_SIZE;
			device.block.sectorCount = drive->sectorCount;
			device.block.driver = BLOCK_DEVICE_DRIVER_AHCI;
			device.block.maxAccessSectorCount = 128; // 64KB buffer / 512
			drive->device = deviceManager.Register(&device);
		}

		KernelLog(LOG_INFO, "AHCIRegisterController - Found drive %d (%dMB).\n", i, drive->sectorCount * AHCI_SECTOR_SIZE / 1048576);
	}
}

bool AHCIController::Access(IOPacket *ioPacket, uintptr_t _drive, uint64_t offset, size_t countBytes, int operation, uint8_t *userBuffer) {
	AHCIDrive *drive = drives + _drive;
	volatile AHCIPort *port = r->ports + _drive;

	uint64_t sector = offset / 512;
	uint64_t offsetIntoSector = offset % 512;
	uint64_t sectorsNeededToLoad = (countBytes + offsetIntoSector + 511) / 512;

	if (countBytes > 65536) {
		KernelPanic("AHCIController::Access - Attempt to access more than 64KiB in one command.\n");
	} else if (operation == DRIVE_ACCESS_WRITE && offsetIntoSector) {
		KernelPanic("AHCIController::Access - Attempt to partially write to a sector.\n");
	}

	uintptr_t commandIndex = AHCI_COMMAND_COUNT;

	// Wait for an available command.
	if (ioPacket) {
		// TODO.Packets.
		return false;
	} else {
		while (true) {
			drive->available.available.Wait(OS_WAIT_NO_TIMEOUT);
			blockedPacketsMutex.Acquire();

			if (drive->available.units) {
				break;
			}

			blockedPacketsMutex.Release();
		}
	}

	// Get a command.
	drive->available.Take(1);
	blockedPacketsMutex.Release();
	drive->mutex.Acquire();

	for (uintptr_t i = 0; i < AHCI_COMMAND_COUNT; i++) {
		if (drive->commandsInUse & (1 << i)) {
			continue;
		} else {
			commandIndex = i;
			break;
		}
	}

	if (commandIndex == AHCI_COMMAND_COUNT) {
		KernelPanic("AHCIController::Access - Could not find an available command even though available.units != 0.\n");
	}

	drive->receivedIRQ[commandIndex].Reset();

	volatile AHCICommandHeader *header = drive->commandList + commandIndex;
	volatile AHCICommandTable *table = drive->commandTable + commandIndex;

	uintptr_t physicalBuffer = drive->physicalBuffers[commandIndex];
	volatile void *buffer = drive->buffers[commandIndex];

	if (operation == DRIVE_ACCESS_WRITE) {
		// Copy the data to the input buffer.
		CopyMemory((uint8_t *) buffer + offsetIntoSector, userBuffer, countBytes);
	}

	// Prepare the PRDT.
	header->commandLength = sizeof(AHCIPacketDeviceToHost) / sizeof(uint32_t);
	header->write = operation == DRIVE_ACCESS_WRITE;
	header->prdEntryCount = 1;
	table->prdtEntries[0].targetAddressLow = (uint32_t) (physicalBuffer >> 0);
	table->prdtEntries[0].targetAddressHigh = (uint32_t) (physicalBuffer >> 32);
	table->prdtEntries[0].byteCount = countBytes + offsetIntoSector;
	table->prdtEntries[0].interruptOnCompletion = true;

	// Setup the ATA command.
	volatile AHCIPacketHostToDevice *packet = (volatile AHCIPacketHostToDevice *) table->commandPacket;
	packet->packetType = AHCI_PACKET_TYPE_HOST_TO_DEVICE;
	packet->controlOrCommand = 1;
	packet->device = 1 << 6;
	packet->countLow = (uint8_t) (sectorsNeededToLoad >> 0);
	packet->countHigh = (uint8_t) (sectorsNeededToLoad >> 8);
	packet->lba0 = (uint8_t) (sector >> 0);
	packet->lba1 = (uint8_t) (sector >> 8);
	packet->lba2 = (uint8_t) (sector >> 16);
	packet->lba3 = (uint8_t) (sector >> 24);
	packet->lba4 = (uint8_t) (sector >> 32);
	packet->lba5 = (uint8_t) (sector >> 40);

	switch (operation) {
		case AHCI_IDENTIFY: {
			packet->command = ATA_IDENTIFY;
		} break;

		case DRIVE_ACCESS_READ: {
			packet->command = ATA_READ_DMA_48;
		} break;

		case DRIVE_ACCESS_WRITE: {
			packet->command = ATA_WRITE_DMA_48;
		} break;

		default: {
			KernelPanic("AHCIController::Access - Unrecognised operation.\n");
		} break;
	}

	// Clear the received packet.
	ZeroMemory((void *) (drive->receivedPacket + commandIndex), sizeof(AHCIReceivedPacket));

	// Wait for the device to no longer be busy.
	{
		Timer timeout = {};
		timeout.Set(AHCI_TIMEOUT, false);
		Defer(timeout.Remove());

		while (port->taskFileData & ((1 << 7) | (1 << 3)) && !timeout.event.Poll());
		if (timeout.event.Poll()) return false;
	}

	// Issue the command.
	drive->commandsInUse |= 1 << commandIndex;
	port->commandIssue = 1 << commandIndex; 
	drive->mutex.Release();

	// Wait for the command to complete.
	drive->receivedIRQ[commandIndex].Wait(AHCI_TIMEOUT);

	bool success = false;

	if (port->commandIssue & (1 << commandIndex)) {
		KernelLog(LOG_WARNING, "AHCIDriver::Access - Could not read from drive (timeout).\n");
		goto finish;
	}

	if (port->interruptStatus & (1 << 30)) {
		KernelLog(LOG_WARNING, "AHCIDriver::Access - Could not read from drive (drive error).\n");
		goto finish;
	}

	// TODO Proper error handling.

	// Copy to the output buffer.
	if (operation != DRIVE_ACCESS_WRITE) {
		CopyMemory(userBuffer, (uint8_t *) buffer + offsetIntoSector, countBytes);
	}

	success = true;
	finish:;

	// Mark the command index as available.
	drive->mutex.Acquire();
	drive->commandsInUse &= ~(1 << commandIndex);
	drive->mutex.Release();
	ahci.blockedPacketsMutex.Acquire();
	drive->available.Return(1);
	ahci.blockedPacketsMutex.Release();
	
	return success;
}

void AHCIController::RemoveBlockingPacket(IOPacket *packet) {
	(void) packet;
}

#endif
