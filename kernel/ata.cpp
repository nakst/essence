// TODO Asynchronous timeout.

#ifdef IMPLEMENTATION

#define ATA_BUSES 2
#define ATA_DRIVES (ATA_BUSES * 2)
#define ATA_SECTOR_SIZE (512)
#define ATA_TIMEOUT (10000)

#define ATA_REGISTER(_bus, _reg) (_reg != -1 ? ((_bus ? 0x170 : 0x1F0) + _reg) : (_bus ? 0x376 : 0x3F6))
#define ATA_IRQ(_bus) (_bus ? 15 : 14)
#define ATA_DATA 0
#define ATA_SECTOR_COUNT 2
#define ATA_LBA1 3
#define ATA_LBA2 4
#define ATA_LBA3 5
#define ATA_DRIVE_SELECT 6
#define ATA_STATUS 7
#define ATA_COMMAND 7
#define ATA_DCR -1
#define ATA_IDENTIFY 0xEC
#define ATA_IDENTIFY_PACKET 0xA1
#define ATA_READ_PIO 0x20
#define ATA_READ_PIO_48 0x24
#define ATA_READ_DMA 0xC8
#define ATA_READ_DMA_48 0x25
#define ATA_WRITE_PIO 0x30
#define ATA_WRITE_PIO_48 0x34
#define ATA_WRITE_DMA 0xCA
#define ATA_WRITE_DMA_48 0x35

#define DMA_REGISTER(_bus, _reg) 4, ((_bus ? (_reg + 8) : _reg))
#define DMA_COMMAND 0
#define DMA_STATUS 2
#define DMA_PRDT 4

struct PRD {
	volatile uint32_t base;
	volatile uint16_t size;
	volatile uint16_t end;
};

struct ATAOperation {
	void *buffer;
	uintptr_t offsetIntoSector, readIndex;
	size_t countBytes, sectorsNeededToLoad;
	uint8_t operation, readingData, bus, slave;
	IOPacket *packet;
};

struct ATABlockedOperation {
	IOPacket *packet;
	uintptr_t drive;
	uint64_t offset;
	size_t countBytes;
	int operation;
	uint8_t *_buffer;

	LinkedItem<ATABlockedOperation> item;
};

struct ATADriver {
	void Initialise();
	bool Access(IOPacket *packet, uintptr_t drive, uint64_t sector, size_t count, int operation, uint8_t *buffer); // Returns true on success.
	bool AccessStart(int bus, int slave, uint64_t sector, uintptr_t offsetIntoSector, size_t sectorsNeededToLoad, size_t countBytes, int operation, uint8_t *buffer);
	bool AccessEnd(int bus, int slave);

	void SetDrive(int bus, int slave, int extra = 0);
	void Unblock();
	bool foundController;

	uint64_t sectorCount[ATA_DRIVES];
	bool isATAPI[ATA_DRIVES];
	Device *devmanDevices[ATA_DRIVES];

	Semaphore semaphore; 

	PRD *prdts[ATA_BUSES];
	void *buffers[ATA_BUSES];
	Event irqs[ATA_BUSES];
	Timer timeouts[ATA_BUSES];

	PCIDevice *device;

	uint16_t identifyData[ATA_SECTOR_SIZE / 2];

	volatile ATAOperation op;
	
	LinkedList<ATABlockedOperation> blockedPackets;
	Mutex blockedPacketsMutex;
};

ATADriver ata;

void ATADriver::SetDrive(int bus, int slave, int extra) {
	ProcessorOut8(ATA_REGISTER(bus, ATA_DRIVE_SELECT), extra | 0xA0 | (slave << 4));
	for (int i = 0; i < 4; i++) ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS));
}

bool ATADriver::AccessStart(int bus, int slave, uint64_t sector, uintptr_t offsetIntoSector, size_t sectorsNeededToLoad, size_t countBytes, int operation, uint8_t *_buffer) {
	uint16_t *buffer = (uint16_t *) _buffer;

	bool s48 = false;

	// Start a timeout.
	Timer *timeout = timeouts + bus;
	timeout->Set(1000, false);
	Defer(timeout->Remove());

	while ((ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) & 0x80) && !timeout->event.Poll());

	if (timeout->event.Poll()) return false;

	if (sector >= 0x10000000) {
		s48 = true;

		SetDrive(bus, slave, 0x40);

		ProcessorOut8(ATA_REGISTER(bus, ATA_SECTOR_COUNT), 0);
		ProcessorOut8(ATA_REGISTER(bus, ATA_SECTOR_COUNT), sectorsNeededToLoad);

		// Set the sector to access.
		// The drive will keep track of the previous and current values of these registers,
		// allowing it to construct a 48-bit sector number.
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA3), sector >> 40);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA2), sector >> 32);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA1), sector >> 24);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA3), sector >> 16);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA2), sector >>  8);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA1), sector >>  0);
	} else {
		SetDrive(bus, slave, 0x40 | (sector >> 24));
		ProcessorOut8(ATA_REGISTER(bus, ATA_SECTOR_COUNT), sectorsNeededToLoad);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA3), sector >> 16);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA2), sector >>  8);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA1), sector >>  0);
	}

	Event *event = irqs + bus;
	event->autoReset = false;
	event->Reset();

	// Save the operation information.
	op.buffer = buffer;
	op.offsetIntoSector = offsetIntoSector;
	op.countBytes = countBytes;
	op.operation = operation;
	op.readingData = false;
	op.readIndex = 0;
	op.sectorsNeededToLoad = sectorsNeededToLoad;

	{
		// Make sure the previous request has completed.
		ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS));
		device->ReadBAR8(DMA_REGISTER(bus, DMA_STATUS));

		// Prepare the PRDT and buffer
		prdts[bus]->size = sectorsNeededToLoad * ATA_SECTOR_SIZE;
		if (operation == DRIVE_ACCESS_WRITE) CopyMemory((uint8_t *) buffers[bus] + offsetIntoSector, buffer, countBytes);

		// Set the mode.
		device->WriteBAR8(DMA_REGISTER(bus, DMA_COMMAND), operation == DRIVE_ACCESS_WRITE ? 0 : 8);
		device->WriteBAR8(DMA_REGISTER(bus, DMA_STATUS), 6);

		// Wait for the RDY bit to set.
		while (!(ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) & (1 << 6)) && !timeout->event.Poll());
		if (timeout->event.Poll()) return false;

		// Issue the command.
		if (s48) 	ProcessorOut8(ATA_REGISTER(bus, ATA_COMMAND), operation == DRIVE_ACCESS_READ ? ATA_READ_DMA_48 : ATA_WRITE_DMA_48);
		else		ProcessorOut8(ATA_REGISTER(bus, ATA_COMMAND), operation == DRIVE_ACCESS_READ ? ATA_READ_DMA :    ATA_WRITE_DMA   );

		// Wait for the DRQ bit to set.
		while (!(ProcessorIn8(ATA_REGISTER(bus, ATA_DCR)) & (1 << 3)) && !timeout->event.Poll());

		if (timeout->event.Poll()) return false;

		// Start the transfer.
		// TODO Properly investigate the order of starting the DMA transfer and sending the ATA command.
		// 	Bochs doesn't work if you issue the ATA command first.
		// 	...and VirtualBox doesn't work if you start the DMA transfer first (except it sometimes does work??).
		device->WriteBAR8(DMA_REGISTER(bus, DMA_COMMAND), operation == DRIVE_ACCESS_WRITE ? 1 : 9);
		if (device->ReadBAR8(DMA_REGISTER(bus, DMA_STATUS)) & 2) return false;
		if (ProcessorIn8(ATA_REGISTER(bus, ATA_DCR)) & 33) return false;
	}

	return true;
}

bool ATADriver::AccessEnd(int bus, int slave) {
	(void) slave;
	Event *event = irqs + bus;

	{
		// Wait for the command to complete.
		event->Wait(ATA_TIMEOUT);

		// Check for error.
		if (ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) & 33) return false;
		if (device->ReadBAR8(DMA_REGISTER(bus, DMA_STATUS)) & 3) return false;

		// Check if the command has completed.
		if (!event->Poll()) {
			return false;
		}

		return true;
	}
}

void ATADriver::Unblock() {
	ATABlockedOperation *operation = nullptr;

	blockedPacketsMutex.Acquire();
	semaphore.Return(1);

	if (blockedPackets.firstItem) {
		operation = blockedPackets.firstItem->thisItem;
		blockedPackets.Remove(blockedPackets.firstItem);
		operation->packet->driverTemp = nullptr;
	}

	blockedPacketsMutex.Release();

	if (operation && !Access(operation->packet, operation->drive, operation->offset, operation->countBytes, operation->operation, operation->_buffer)) {
		operation->packet->request->Cancel(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
	}

	OSHeapFree(operation);
}

bool ATADriver::Access(IOPacket *packet, uintptr_t drive, uint64_t offset, size_t countBytes, int operation, uint8_t *_buffer) {
	uint64_t sector = offset / 512;
	uint64_t offsetIntoSector = offset % 512;
	uint64_t sectorsNeededToLoad = (countBytes + offsetIntoSector + 511) / 512;
	uintptr_t bus = drive >> 1;
	uintptr_t slave = drive & 1;

	if (drive >= ATA_DRIVES) KernelPanic("ATADriver::Access - Drive %d exceedes the maximum number of ATA driver (%d).\n", drive, ATA_DRIVES);
	if (isATAPI[drive]) KernelPanic("ATADriver::Access - Drive %d is an ATAPI drive. ATAPI read/write operations are currently not supported.\n", drive);
	if (!sectorCount[drive]) KernelPanic("ATADriver::Access - Drive %d is invalid.\n", drive);
	if (sector > sectorCount[drive] || (sector + sectorsNeededToLoad) > sectorCount[drive]) KernelPanic("ATADriver::Access - Attempt to access sector %d when drive only has %d sectors.\n", sector, sectorCount[drive]);
	if (sectorsNeededToLoad > 64) KernelPanic("ATADriver::Access - Attempt to read more than 64 consecutive sectors in 1 function call.\n");

	// Lock the driver.
	if (packet) {
		packet->driverRunning = true;
		blockedPacketsMutex.Acquire();
		if (semaphore.units == 0) {
			ATABlockedOperation *op = (ATABlockedOperation *) OSHeapAllocate(sizeof(ATABlockedOperation), true);
			packet->driverTemp = op;
			op->packet = packet;
			op->drive = drive;
			op->offset = offset;
			op->countBytes = countBytes;
			op->operation = operation;
			op->_buffer = _buffer;
			op->item.thisItem = op;
			blockedPackets.InsertEnd(&op->item);
			blockedPacketsMutex.Release();
			return true;
		} else {
			semaphore.Take(1);
		}
		blockedPacketsMutex.Release();
	} else {
		while (true) {
			semaphore.available.Wait(OS_WAIT_NO_TIMEOUT);
			blockedPacketsMutex.Acquire();
			if (semaphore.units) {
				semaphore.Take(1);
				break;
			}
			blockedPacketsMutex.Release();
		}
		blockedPacketsMutex.Release();
	}

	op.packet = packet;
	op.bus = bus;
	op.slave = slave;

	if (!AccessStart(bus, slave, sector, offsetIntoSector, sectorsNeededToLoad, countBytes, operation, _buffer)) {
		semaphore.Return(1);
		return false;
	}

#if 0
	bool result = AccessEnd(bus, slave);
	semaphore.Return(1);
	if (result && packet) packet->Complete(OS_SUCCESS);
	return result;
#else
	if (!packet) {
		bool result = AccessEnd(bus, slave);
		Unblock();
		return result;
	} else {
		return true; // The command has been successfully queued.
	}
#endif
}

void ATAIRQHandler2(void *argument) {
	(void) argument;

	ATAOperation *op = (ATAOperation *) &ata.op;
	bool cancelled = false;

	if (op->packet) {
		op->packet->request->mutex.Acquire();
		cancelled = op->packet->cancelled;
	}

	ProcessorIn8(ATA_REGISTER(op->bus, ATA_STATUS));
	ata.device->ReadBAR8(DMA_REGISTER(op->bus, DMA_STATUS));

	{
		if (!(ata.device->ReadBAR8(DMA_REGISTER(op->bus, DMA_STATUS)) & 4)) {
			// The interrupt bit was not set, so the IRQ must have been generated by a different device.
		} else {
			Event *event = ata.irqs + op->bus;

			if (!event->state) {
				// Copy the data that we read.
				if (op->buffer && op->operation == DRIVE_ACCESS_READ && !cancelled) {
					CopyMemory((void *) op->buffer, (uint8_t *) ata.buffers[op->bus] + op->offsetIntoSector, op->countBytes);
				}

				// Stop the transfer.
				ata.device->WriteBAR8(DMA_REGISTER(op->bus, DMA_COMMAND), 0);

				event->Set();
				goto requestDone;
			} else {
				KernelLog(LOG_WARNING, "ATAIRQHandler - Received more interrupts than expected.\n");
			}
		}
	}

	goto done;
	requestDone:;

	if (op->packet) {
		IOPacket *packet = op->packet;
		IORequest *request = packet->request;

		bool result = ata.AccessEnd(op->bus, op->slave);
		ata.Unblock();
		packet->driverRunning = false;

		if (!result) {
			request->Cancel(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
			// `packet` has been deallocated.
		} else {
			packet->Complete(result ? OS_SUCCESS : OS_ERROR_UNKNOWN_OPERATION_FAILURE);
		}

		request->mutex.Release();

		return;
	}

	done:;

	if (op->packet) {
		op->packet->request->mutex.Release();
	}
}

bool ATAIRQHandler(uintptr_t interruptIndex) {
	int bus = interruptIndex - ATA_IRQ(0);

	// Acknowledge the interrupt.
	ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS));
	ata.device->ReadBAR8(DMA_REGISTER(bus, DMA_STATUS));

	if (!ata.op.buffer) {
		return false;
	}

#ifdef ARCH_X86_64
	if (ata.op.buffer < (void *) 0xFFFF800000000000) {
		KernelPanic("ATAIRQHandler - Copy buffer (%x) not in kernel address space.\n", ata.op.buffer);
	}
#endif

	if (ata.op.packet) {
		scheduler.lock.Acquire();
		RegisterAsyncTask(ATAIRQHandler2, nullptr, nullptr, true);
		scheduler.lock.Release();
	} else {
		// If we're using synchronous IO, then *don't* queue an asynchronous task.
		// First of all, we don't need to (it's slower),
		// and secondly, we need to Sync() nodes we're closing during process handle table termination,
		// which takes place in the asynchronous task thread. (Meaning we'd get deadlock).
		// TODO Is there a better way to do this, preventing similar bugs in the future?
		ATAIRQHandler2(nullptr);
	}

	GetLocalStorage()->irqSwitchThread = true; 

	return true;
}

void ATADriver::Initialise() {
	semaphore.Return(1);

	Device *controller;

	{
		Device device = {};
		device.type = DEVICE_TYPE_ATA_CONTROLLER;
		device.parent = DEVICE_PARENT_ROOT;
		controller = deviceManager.Register(&device);
	}

	KernelLog(LOG_VERBOSE, "ATADriver::Initialise - Found an ATA controller.\n");

	for (uintptr_t bus = 0; bus < ATA_BUSES; bus++) {
		// If the status is 0xFF, then the bus does not exist.
		if (ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) == 0xFF) {
			continue;
		}

		// Check that the LBA registers are RW.
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA1), 0xAB);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA2), 0xCD);
		ProcessorOut8(ATA_REGISTER(bus, ATA_LBA3), 0xEF);

		// Otherwise, the bus doesn't exist.
		if (ProcessorIn8(ATA_REGISTER(bus, ATA_LBA1) != 0xAB)) continue;
		if (ProcessorIn8(ATA_REGISTER(bus, ATA_LBA2) != 0xCD)) continue;
		if (ProcessorIn8(ATA_REGISTER(bus, ATA_LBA3) != 0xEF)) continue;

		// Clear the device command register.
		ProcessorOut8(ATA_REGISTER(bus, ATA_DCR), 0);

		int dmaDrivesOnBus = 0;
		int drivesOnBus = 0;
		uint8_t status;

		size_t drivesPerBus = 2;

		for (uintptr_t slave = 0; slave < drivesPerBus; slave++) {
			// Issue the IDENTIFY command to the drive.
			SetDrive(bus, slave);
			ProcessorOut8(ATA_REGISTER(bus, ATA_LBA2), 0);
			ProcessorOut8(ATA_REGISTER(bus, ATA_LBA3), 0);
			ProcessorOut8(ATA_REGISTER(bus, ATA_COMMAND), ATA_IDENTIFY);

			// Start a timeout.
			Timer *timeout = timeouts + bus;
			timeout->Set(100, false);
			Defer(timeout->Remove());

			// Check for error.
			if (ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) & 32) continue;
			if (ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) & 1) {
				uint8_t a = ProcessorIn8(ATA_REGISTER(bus, ATA_LBA2));
				uint8_t b = ProcessorIn8(ATA_REGISTER(bus, ATA_LBA3));
				if (a == 0x14 && b == 0xEB) {
					isATAPI[bus * 2 + slave] = true;
					ProcessorOut8(ATA_REGISTER(bus, ATA_COMMAND), ATA_IDENTIFY_PACKET);
				} else {
					continue;
				}
			}

			// Wait for the drive to be ready for the data transfer.
			while ((ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) & 0x80) && !timeout->event.Poll());
			if (timeout->event.Poll()) continue;
			while ((!(status = ProcessorIn8(ATA_REGISTER(bus, ATA_STATUS)) & 9)) && !timeout->event.Poll());
			if (timeout->event.Poll()) continue;
			if (status & 33) continue;

			// Transfer the data.
			for (uintptr_t i = 0; i < 256; i++) {
				identifyData[i] = ProcessorIn16(ATA_REGISTER(bus, ATA_DATA));
			}

			// Check if the device supports LBA/DMA.
			if (!(identifyData[49] & 0x200)) continue;
			if (!(identifyData[49] & 0x100)) continue;
			dmaDrivesOnBus |= 1;
			drivesOnBus |= 1;

			// Work out the number of sectors in the drive.
			uint32_t lba28Sectors = ((uint32_t) identifyData[60] << 0) + ((uint32_t) identifyData[61] << 16);
			uint64_t lba48Sectors = ((uint64_t) identifyData[100] << 0) + ((uint64_t) identifyData[101] << 16) +
				((uint64_t) identifyData[102] << 32) + ((uint64_t) identifyData[103] << 48);
			bool supportsLBA48 = lba48Sectors && (identifyData[83] & 0x40);
			uint64_t sectors = supportsLBA48 ? lba48Sectors : lba28Sectors;
			sectorCount[slave + bus * 2] = sectors;

			KernelLog(LOG_INFO, "ATADriver::Initialise - Found ATA%z drive: %d/%d\n", isATAPI[slave + bus * 2] ? "PI" : "" ,bus, slave);
			KernelLog(LOG_INFO, "ATADriver::Initialise - Sectors: %x\n", sectors);
		}

		if (dmaDrivesOnBus) {
			uintptr_t dataPhysical = pmm.AllocateContiguous128KB();

			if (!dataPhysical) {
				KernelLog(LOG_WARNING, "ATADriver::Initialise - Could not allocate memory for DMA on bus %d.\n", bus);
				sectorCount[bus * 2 + 0] = sectorCount[bus * 2 + 1] = 0;
				drivesOnBus = 0;
			} else {
				uintptr_t dataVirtual = (uintptr_t) kernelVMM.Allocate("ATADMA", 131072, VMM_MAP_ALL, VMM_REGION_PHYSICAL, dataPhysical, VMM_REGION_FLAG_NOT_CACHABLE /*do not cache reads/writes*/);

				PRD *prdt = (PRD *) dataVirtual;
				prdt->end = 0x8000;
				prdt->base = dataPhysical + 65536;
				prdts[bus] = prdt;

				void *buffer = (void *) (dataVirtual + 65536);
				buffers[bus] = buffer;

				device->WriteBAR32(DMA_REGISTER(bus, DMA_PRDT), dataPhysical);
			}
		}

		if (drivesOnBus) {
			if (!RegisterIRQHandler(ATA_IRQ(bus), ATAIRQHandler)) {
				KernelLog(LOG_WARNING, "ATADriver::Initialise - Could not register IRQ for bus %d.\n", bus);

				// Disable the drives on this bus.
				sectorCount[bus * 2 + 0] = 0;
				sectorCount[bus * 2 + 1] = 0;
			}
		}

		for (uintptr_t slave = 0; slave < 2; slave++) {
			if (sectorCount[slave + bus * 2]) {
				bool success;
					
				success = Access(nullptr, bus * 2 + slave, 0, 512, DRIVE_ACCESS_READ, (uint8_t *) identifyData);

				if (!success) {
					KernelLog(LOG_WARNING, "ATADriver::Initialise - Could not perform test read on drive.\n");
				} else {
					Access(nullptr, bus * 2 + slave, 0, 512, DRIVE_ACCESS_READ, (uint8_t *) identifyData);
					success = Access(nullptr, bus * 2 + slave, 0, 512, DRIVE_ACCESS_WRITE, (uint8_t *) identifyData);

					if (!success) {
						KernelLog(LOG_WARNING, "ATADriver::Initialise - Could not perform test write to drive.\n");
					}
				}
			}
		}
	}

	for (uintptr_t i = 0; i < ATA_DRIVES; i++) {
		if (sectorCount[i]) {
			// Register the drive.
			Device device = {};
			device.parent = controller;
			device.type = DEVICE_TYPE_BLOCK;
			device.block.driveID = i;
			device.block.sectorSize = ATA_SECTOR_SIZE;
			device.block.sectorCount = sectorCount[i];
			device.block.driver = BLOCK_DEVICE_DRIVER_ATA;
			device.block.maxAccessSectorCount = 64;
			devmanDevices[i] = deviceManager.Register(&device);
			if (!devmanDevices[i]) sectorCount[i] = 0;
		}
	}
}

void ATARegisterController(PCIDevice *device) {
	if (ata.foundController) {
		KernelLog(LOG_WARNING, "ATARegisterController - Attempt to register multiple ATA controllers; ignored.\n");
	} else {
		ata.foundController = true;
		ata.device = device;
		ata.Initialise();
	}
}

#endif
