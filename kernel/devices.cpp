#ifndef IMPLEMENTATION

enum DeviceType {
	DEVICE_TYPE_INVALID,
	DEVICE_TYPE_BLOCK,
	DEVICE_TYPE_ATA_CONTROLLER,
};

typedef bool (*DriveAccessFunction)(uintptr_t drive, uint64_t sector, size_t count, int operation, uint8_t *buffer); // Returns true on success.

enum BlockDeviceDriver {
	BLOCK_DEVICE_DRIVER_INVALID,
	BLOCK_DEVICE_DRIVER_ATA,
};

struct BlockDevice {
	bool Access(uint64_t sector, size_t count, int operation, uint8_t *buffer);

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

#endif

#ifdef IMPLEMENTATION

DeviceManager deviceManager;

bool BlockDevice::Access(uint64_t sector, size_t count, int operation, uint8_t *buffer) {
	if (sector > sectorCount || (sector + count) > sectorCount) {
		KernelPanic("BlockDevice::Access - Sector %d/%d out of bounds on drive %d with count %d.\n", sector, sectorCount, driveID, count);
	}

	sector += sectorOffset;

	switch (driver) {
		case BLOCK_DEVICE_DRIVER_ATA: {
			return ata.Access(driveID, sector, count, operation, buffer);
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
		uint8_t *information = (uint8_t *) kernelVMM.Allocate(device->block.sectorSize * 4);
		Defer(kernelVMM.Free(information));

		// Load the first 4 sectors of the drive to identify its filesystem.
		// Why don't we do this in vfs.cpp? Because C++ headers are driving me insane!!
		bool success = device->block.Access(0, 4, DRIVE_ACCESS_READ, information);

		if (!success) {
			// We could not access the block device.
			return nullptr;
		}

		if (information[1080] == 0x53 && information[1081] == 0xEF) {
			Ext2FS *filesystem = (Ext2FS *) kernelVMM.Allocate(sizeof(Ext2FS));
			File *root = filesystem->Initialise(device);
			if (root) {
				vfs.RegisterFilesystem(root, FILESYSTEM_EXT2, filesystem);
			} else {
				KernelLog(LOG_WARNING, "DeviceManager::Register - Block device %d contains invalid ext2 filesystem.\n", device->id);
				kernelVMM.Free(filesystem);
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
