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

extern DeviceManager deviceManager;
void ATARegisterController(struct PCIDevice *device);
void AHCIRegisterController(struct PCIDevice *device);

#endif

#ifdef IMPLEMENTATION

DeviceManager deviceManager;

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
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 0);
	osRandomByteSeed += ProcessorIn8(0x71) << 0;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 2);
	osRandomByteSeed += ProcessorIn8(0x71) << 1;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 4);
	osRandomByteSeed += ProcessorIn8(0x71) << 2;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 6);
	osRandomByteSeed += ProcessorIn8(0x71) << 3;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 7);
	osRandomByteSeed += ProcessorIn8(0x71) << 4;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 8);
	osRandomByteSeed += ProcessorIn8(0x71) << 5;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 9);
	osRandomByteSeed += ProcessorIn8(0x71) << 6;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 10);
	osRandomByteSeed += ProcessorIn8(0x71) << 7;
	for (int i = 0; i < 10; i++) ProcessorOut8(0x70, 11);
	osRandomByteSeed += ProcessorIn8(0x71) << 8;

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
		uint8_t *information = (uint8_t *) OSHeapAllocate(device->block.sectorSize * 32, false);
		Defer(OSHeapFree(information));

		// Load the first 16KB of the drive to identify its filesystem.
		// Why don't we do this in vfs.cpp? Because C++ headers are driving me insane!!
		bool success = device->block.Access(0, 16384, DRIVE_ACCESS_READ, information);

		if (!success) {
			// We could not access the block device.
			return nullptr;
		}

		if (0) {

#if 0
		} else if (information[1080] == 0x53 && information[1081] == 0xEF) {
			UniqueIdentifier installationID = {}; // You can't boot from a Ext2 filesystem.
			Ext2FS *filesystem = (Ext2FS *) OSHeapAllocate(sizeof(Ext2FS), true);
			File *root = filesystem->Initialise(device);
			if (root) {
				filesystem->filesystem = vfs.RegisterFilesystem(root, FILESYSTEM_EXT2, filesystem, installationID);
			} else {
				KernelLog(LOG_WARNING, "DeviceManager::Register - Block device %d contains invalid ext2 filesystem.\n", device->id);
				OSHeapFree(filesystem);
			}
#endif
		} else if (((uint32_t *) information)[2048] == 0x65737345) {
			EsFSVolume *volume = (EsFSVolume *) OSHeapAllocate(sizeof(EsFSVolume), true);
			Node *root = volume->Initialise(device);
			if (root) {
				volume->filesystem = vfs.RegisterFilesystem(root, FILESYSTEM_ESFS, volume, volume->superblock.osInstallation);
			} else {
				KernelLog(LOG_WARNING, "DeviceManager::Register - Block device %d contains invalid EssenceFS volume.\n", device->id);
				OSHeapFree(volume);
			}
		} else if (information[510] == 0x55 && information[511] == 0xAA && !device->block.sectorOffset /*Must be at start of drive*/) {
			// Check each partition in the table.
			for (uintptr_t i = 0; i < 4; i++) {
				for (uintptr_t j = 0; j < 0x10; j++) {
					if (information[j + 0x1BE + i * 0x10]) {
						goto partitionExists;
					}
				}

				continue;
				partitionExists:

				uint32_t offset = ((uint32_t) information[0x1BE + i * 0x10 + 8 ] << 0 )
					+ ((uint32_t) information[0x1BE + i * 0x10 + 9 ] << 8 )
					+ ((uint32_t) information[0x1BE + i * 0x10 + 10] << 16)
					+ ((uint32_t) information[0x1BE + i * 0x10 + 11] << 24);
				uint32_t count  = ((uint32_t) information[0x1BE + i * 0x10 + 12] << 0 )
					+ ((uint32_t) information[0x1BE + i * 0x10 + 13] << 8 )
					+ ((uint32_t) information[0x1BE + i * 0x10 + 14] << 16)
					+ ((uint32_t) information[0x1BE + i * 0x10 + 15] << 24);

				if (offset + count > device->block.sectorCount) {
					// The MBR is invalid.
					goto unknownFilesystem;
				}

				Device child;
				CopyMemory(&child, device, sizeof(Device));
				child.item = {};
				child.parent = device;
				child.block.sectorOffset += offset;
				child.block.sectorCount = count;
				Register(&child);
			}
		} else {
			unknownFilesystem:
			KernelLog(LOG_WARNING, "DeviceManager::Register - Could not detect filesystem or partition table on block device %d.\n", device->id);
		}
	}

	return device;
}

#endif
