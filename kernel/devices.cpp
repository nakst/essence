#ifndef IMPLEMENTATION

enum DeviceType {
	DEVICE_TYPE_INVALID,
	DEVICE_TYPE_BLOCK,
	DEVICE_TYPE_ATA_CONTROLLER,
	DEVICE_TYPE_AHCI_CONTROLLER,
};

typedef bool (*DriveAccessFunction)(uintptr_t drive, uint64_t offset, size_t countBytes, int operation, uint8_t *buffer); // Returns true on success.

enum BlockDeviceDriver {
	BLOCK_DEVICE_DRIVER_INVALID,
	BLOCK_DEVICE_DRIVER_ATA,
	BLOCK_DEVICE_DRIVER_AHCI,
};

struct BlockDevice {
	bool Access(uint64_t sector, size_t count, int operation, uint8_t *buffer, bool alreadyInCorrectPartition = false);

	uintptr_t driveID;
	size_t sectorSize;
	size_t maxAccessSectorCount;
	uint64_t sectorOffset;
	uint64_t sectorCount;
	BlockDeviceDriver driver;
};

struct Device {
	DeviceType type;
#define DEVICE_PARENT_ROOT (nullptr)
	Device *parent;
	LinkedItem item;
	uintptr_t id;

	union {
		BlockDevice block;
	};
};

struct DeviceManager {
	Device *Register(Device *device);
	void Initialise();

	Mutex lock;

	uintptr_t nextDeviceID;
	LinkedList deviceList;
	Pool devicePool;
};

struct IOPacket {
	void Execute();
	LinkedItem item;
};

struct IOManager {
	void Initialise();
	void Work();
	void AddPacket(IOPacket *packet);

	LinkedList packets;
	Mutex mutex;
	Event packetsAvailable;
};

extern DeviceManager deviceManager;
extern IOManager ioManager;
void ATARegisterController(struct PCIDevice *device);
void AHCIRegisterController(struct PCIDevice *device);

#endif

#ifdef IMPLEMENTATION

DeviceManager deviceManager;
IOManager ioManager;

Mutex tempMutex;

bool BlockDevice::Access(uint64_t offset, size_t countBytes, int operation, uint8_t *buffer, bool alreadyInCorrectPartition) {
	if (!alreadyInCorrectPartition) {
		if (offset / sectorSize > sectorCount || (offset + countBytes) / sectorSize > sectorCount || countBytes / sectorSize > maxAccessSectorCount) {
			KernelPanic("BlockDevice::Access - Access out of bounds on drive %d.\n", driveID);
		}

		offset += sectorOffset * sectorSize;

		if (operation == DRIVE_ACCESS_WRITE) {
			Print("Write to drive.....\n");
		}
	}

	if (operation == DRIVE_ACCESS_WRITE && ((offset % sectorSize) || (countBytes % sectorSize))) {
		uint64_t currentSector = offset / sectorSize;
		uint64_t sectorCount = (offset + countBytes) / sectorSize - currentSector;

		uint8_t *temp = (uint8_t *) OSHeapAllocate(sectorSize, false);
		Defer(OSHeapFree(temp));

		if (!Access(currentSector * sectorSize, sectorSize, DRIVE_ACCESS_READ, temp, true)) return false;
		size_t size = sectorSize - (offset - currentSector * sectorSize);
		CopyMemory(temp + (offset - currentSector * sectorSize), buffer, size > countBytes ? countBytes : size);
		if (!Access(currentSector * sectorSize, sectorSize, DRIVE_ACCESS_WRITE, temp, true)) return false;
		buffer += sectorSize - (offset - currentSector * sectorSize);
		countBytes -= sectorSize - (offset - currentSector * sectorSize);
		currentSector++;

		if (sectorCount > 1) {
			if (!Access(currentSector * sectorSize, sectorCount - 1, DRIVE_ACCESS_WRITE, buffer, true)) return false;
			buffer += sectorSize * (sectorCount - 1);
			currentSector += sectorCount - 1;
			countBytes -= sectorSize * (sectorCount - 1);
		}

		if (sectorCount) {
			if (!Access(currentSector * sectorSize, sectorSize, DRIVE_ACCESS_READ, temp, true)) return false;
			size_t size = sectorSize - (offset - currentSector * sectorSize);
			CopyMemory(temp + (offset - currentSector * sectorSize), buffer, size > countBytes ? countBytes : size);
			if (!Access(currentSector * sectorSize, sectorSize, DRIVE_ACCESS_WRITE, temp, true)) return false;
		}

		return true;
	}

	switch (driver) {
		case BLOCK_DEVICE_DRIVER_ATA: {
			return ata.Access(driveID, offset, countBytes, operation, buffer);
		} break;

		case BLOCK_DEVICE_DRIVER_AHCI: {
			return ahci.Access(driveID, offset, countBytes, operation, buffer);
		} break;

		default: {
			KernelPanic("BlockDevice::Access - Invalid BlockDeviceDriver %d\n", driver);
			return false;
		} break;
	}
}

void DeviceManager::Initialise() {
	devicePool.Initialise(sizeof(Device));

#ifdef ARCH_X86_64
	InitialiseRandomSeed();
	pci.Enumerate();
	ps2.Initialise();
#endif

	// Once we have initialised the device manager we should have found the drive from which we booted.
	if (!vfs.foundBootFilesystem) {
		KernelPanic("DeviceManager::Initialise - Could not find the boot filesystem.\n");
	}
}

Device *DeviceManager::Register(Device *deviceSpec) {
	lock.Acquire();
	Device *device = deviceSpec;
	device->id = nextDeviceID++;
	device = (Device *) devicePool.Add();
	CopyMemory(device, deviceSpec, sizeof(Device));
	device->item.thisItem = device;
	deviceList.InsertEnd(&device->item);
	lock.Release();

	if (device->type == DEVICE_TYPE_BLOCK) {
		DetectFilesystem(device);
	}

	return device;
}

void IOPacket::Execute() {
	// TODO
}

void IOManager::AddPacket(IOPacket *packet) {
	mutex.Acquire();

	if (packets.count == 0) {
		packetsAvailable.Set();
	}

	packet->item.thisItem = packet;
	packets.InsertEnd(&packet->item);

	mutex.Release();
}

void IOManager::Work() {
	while (true) {
		packetsAvailable.Wait(OS_WAIT_NO_TIMEOUT);

		IOPacket *packet = nullptr;

		mutex.Acquire();

		if (packets.count) {
			LinkedItem *packetItem = packets.firstItem;
			packets.Remove(packetItem);
			packet = (IOPacket *) packetItem->thisItem;
		}

		if (!packets.count) {
			packetsAvailable.Reset();
		}

		mutex.Release();

		if (packet) {
			packet->Execute();
			OSHeapFree(packet); // Deallocate the packet.
		}
	}
}

void _IOManagerWorkerThread() {
	ioManager.Work();
}

void IOManager::Initialise() {
	packetsAvailable.autoReset = false;
	scheduler.SpawnThread((uintptr_t) _IOManagerWorkerThread, 0, kernelProcess, false);
}

#endif
