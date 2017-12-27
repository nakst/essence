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
	bool Access(IOPacket *packet, uint64_t offset, size_t count, int operation, uint8_t *buffer, 
			bool alreadyInCorrectPartition = false, bool freeBuffer = false);

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
	LinkedItem<Device> item;
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
	LinkedList<Device> deviceList;
	Pool devicePool;
};

enum IORequestType {
	IO_REQUEST_READ,
	IO_REQUEST_WRITE,
};

enum IOPacketType {
	IO_PACKET_NODE,
	IO_PACKET_ESFS,
	IO_PACKET_BLOCK_DEVICE_PARTIAL_WRITE,
	IO_PACKET_BLOCK_DEVICE_FREE_BUFFER,
	IO_PACKET_AHCI,
	IO_PACKET_ATA,
};

struct IOPacket {
	void Complete(OSError error);
	void QueuedChildren();

	bool cancelled;
	IOPacketType type;
	struct IORequest *request;

	IOPacket *parent;
	LinkedItem<IOPacket> treeItem, driverItem;
	LinkedList<IOPacket> children;
	size_t remaining;

	void *object;
	void *buffer;
	uint64_t offset, count; 
	void *parameter1, *parameter2;
};

struct IORequest {
	void Start();
	void Cancel(OSError error);
	void Complete();
	bool CloseHandle();
	void PrintTree();
	IOPacket *AddPacket(IOPacket *parent);

	IORequestType type;
	OSError error;
	Event complete;
	IOPacket *root;

	Node *node;
	void *buffer;
	uint64_t offset, count; 

	Mutex mutex;
	size_t handles;
};

extern DeviceManager deviceManager;

void ATARegisterController(struct PCIDevice *device);
void AHCIRegisterController(struct PCIDevice *device);

#define REQUEST_INCOMPLETE (0)
#define REQUEST_BLOCKED (1)

#endif

#ifdef IMPLEMENTATION

DeviceManager deviceManager;

bool BlockDevice::Access(IOPacket *packet, uint64_t offset, size_t countBytes, int operation, uint8_t *buffer, bool alreadyInCorrectPartition, bool freeBuffer) {
	if (!packet && freeBuffer) {
		KernelPanic("BlockDevice::Access - `freeBuffer` set but `packet` was nullptr.\n");
	}

	if (!alreadyInCorrectPartition) {
		if (offset / sectorSize > sectorCount || (offset + countBytes) / sectorSize > sectorCount || countBytes / sectorSize > maxAccessSectorCount) {
			KernelPanic("BlockDevice::Access - Access out of bounds on drive %d.\n", driveID);
		}

		offset += sectorOffset * sectorSize;
	}

	if (operation == DRIVE_ACCESS_WRITE && ((offset % sectorSize) || (countBytes % sectorSize))) {
		uint64_t currentSector = offset / sectorSize;

		uint8_t *temp1 = (uint8_t *) OSHeapAllocate(sectorSize, false);
		Defer(if (!packet) OSHeapFree(temp1));
		uint8_t *temp2 = (uint8_t *) OSHeapAllocate(sectorSize, false);
		Defer(if (!packet) OSHeapFree(temp2));

		{
			size_t size = sectorSize - (offset - currentSector * sectorSize);
			size = size > countBytes ? countBytes : size;

			if (packet) {
				IOPacket *continuePacket = packet->request->AddPacket(packet);
				continuePacket->buffer = temp1;
				continuePacket->offset = currentSector * sectorSize;
				continuePacket->count = size;
				continuePacket->object = this;
				continuePacket->type = IO_PACKET_BLOCK_DEVICE_PARTIAL_WRITE;
				continuePacket->parameter1 = buffer;
				continuePacket->parameter2 = temp1 + (offset - currentSector * sectorSize);
				Access(continuePacket, currentSector * sectorSize, sectorSize, DRIVE_ACCESS_READ, temp1, true);
				continuePacket->QueuedChildren();
			} else {
				if (!Access(packet, currentSector * sectorSize, sectorSize, DRIVE_ACCESS_READ, temp1, true)) return false;
				CopyMemory(temp1 + (offset - currentSector * sectorSize), buffer, size);
				if (!Access(packet, currentSector * sectorSize, sectorSize, DRIVE_ACCESS_WRITE, temp1, true)) return false;
			}

			buffer += size;
			countBytes -= size;
			currentSector++;
		}

		if (countBytes >= sectorSize) {
			size_t fullSectors = countBytes / sectorSize;

			if (!Access(packet, currentSector * sectorSize, fullSectors * sectorSize, DRIVE_ACCESS_WRITE, buffer, true)) return false;

			buffer += sectorSize * fullSectors;
			countBytes -= sectorSize * fullSectors;
			currentSector += fullSectors;
		}

		if (countBytes) {
			size_t size = countBytes;

			if (packet) {
				IOPacket *continuePacket = packet->request->AddPacket(packet);
				continuePacket->buffer = temp2;
				continuePacket->offset = currentSector * sectorSize;
				continuePacket->count = size;
				continuePacket->object = this;
				continuePacket->type = IO_PACKET_BLOCK_DEVICE_PARTIAL_WRITE;
				continuePacket->parameter1 = buffer;
				continuePacket->parameter2 = temp2;
				Access(continuePacket, currentSector * sectorSize, sectorSize, DRIVE_ACCESS_READ, temp2, true);
				continuePacket->QueuedChildren();
			} else {
				if (!Access(packet, currentSector * sectorSize, sectorSize, DRIVE_ACCESS_READ, temp2, true)) return false;
				CopyMemory(temp2, buffer, size);
				if (!Access(packet, currentSector * sectorSize, sectorSize, DRIVE_ACCESS_WRITE, temp2, true)) return false;
			}
		}

		return true;
	}

	IOPacket *driverPacket = nullptr;
	
	if (packet) {
		if (freeBuffer) {
			packet = packet->request->AddPacket(packet);
			packet->buffer = buffer;
			packet->type = IO_PACKET_BLOCK_DEVICE_FREE_BUFFER;
		}

		// Make a new packet for the driver, if we're using the asynchronous API.
		driverPacket = packet->request->AddPacket(packet);
		driverPacket->buffer = buffer;
		driverPacket->offset = offset;
		driverPacket->count = countBytes;
		driverPacket->object = (void *) driveID;

		if (freeBuffer) {
			packet->QueuedChildren();
		}
	}

	bool result;

	switch (driver) {
		case BLOCK_DEVICE_DRIVER_ATA: {
			if (driverPacket) driverPacket->type = IO_PACKET_ATA;
			result = ata.Access(driverPacket, driveID, offset, countBytes, operation, buffer);

			if (driverPacket) {
				if (result) {
					driverPacket->Complete(OS_SUCCESS);
				} else {
					driverPacket->request->Cancel(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
				}
			}
		} break;

		case BLOCK_DEVICE_DRIVER_AHCI: {
			if (driverPacket) driverPacket->type = IO_PACKET_AHCI;
			result = ahci.Access(/*driverPacket, */driveID, offset, countBytes, operation, buffer);

			if (driverPacket) {
				if (result) {
					driverPacket->Complete(OS_SUCCESS);
				} else {
					driverPacket->request->Cancel(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
				}
			}
		} break;

		default: {
			KernelPanic("BlockDevice::Access - Invalid BlockDeviceDriver %d\n", driver);
			result = false;
		} break;
	}

	return result;
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

IOPacket *IORequest::AddPacket(IOPacket *parent) {
	mutex.AssertLocked();
	handles++;

	IOPacket *packet = (IOPacket *) OSHeapAllocate(sizeof(IOPacket), true);
	packet->parent = parent;
	packet->request = this;
	packet->treeItem.thisItem = packet;
	packet->driverItem.thisItem = packet;
	packet->remaining = 1; // First event is removed when all children have been queued.

	if (parent) {
		parent->children.InsertEnd(&packet->treeItem);
		parent->remaining++;
	}

	return packet;
}

void PrintIOPacket(IOPacket *packet) {
	Print("{ %x %d %d ", packet, packet->type, packet->remaining);
	LinkedItem<IOPacket> *child = packet->children.firstItem;
	while (child) {
		IOPacket *packet = (IOPacket *) child->thisItem;
		child = child->nextItem;
		PrintIOPacket(packet);
	}
	Print(" }");
}

void IORequest::PrintTree() {
	if (root) PrintIOPacket(root);
	else Print("{}");
	Print("\n");
}

void IORequest::Start() {
	mutex.Acquire();
	Defer(mutex.Release());

	buffer = kernelVMM.Allocate("IOCopy", count, vmmMapAll, vmmRegionCopy, (uintptr_t) buffer);

	if (handles < 1) {
		KernelPanic("IORequest::Start - Invalid handle count.\n");
	}

	root = AddPacket(nullptr);
	root->type = IO_PACKET_NODE;

	switch (type) {
		case IO_REQUEST_READ: {
			node->Read(root);
		} break;

		case IO_REQUEST_WRITE: {
			node->Write(root);
		} break;
	}

	root->QueuedChildren();
}

void IOPacket::Complete(OSError error) {
	request->mutex.AssertLocked();

	if (!cancelled) {
		bool success = error == OS_SUCCESS;
		if (!success) cancelled = true;

		switch (type) {
			case IO_PACKET_NODE: {
				request->node->Complete(this);
				if (success) request->Complete();
			} break;

			case IO_PACKET_ESFS: {
			} break;

			case IO_PACKET_BLOCK_DEVICE_PARTIAL_WRITE: {
				BlockDevice *device = (BlockDevice *) object;
				CopyMemory(parameter2, parameter1, count);
				device->Access(parent, offset, device->sectorSize, DRIVE_ACCESS_WRITE, (uint8_t *) buffer, true, true);
			} break;

			case IO_PACKET_BLOCK_DEVICE_FREE_BUFFER: {
				OSHeapFree(buffer);
			} break;

			case IO_PACKET_AHCI: {
			} break;

			case IO_PACKET_ATA: {
			} break;
		}

		if (success && parent) {
			parent->remaining--;
			parent->children.Remove(&treeItem);

			if (parent->remaining == 0) {
				parent->Complete(error);
			}
		}

		if (!success) {
			LinkedItem<IOPacket> *child = children.firstItem;

			while (child) {
				IOPacket *packet = (IOPacket *) child->thisItem;
				child = child->nextItem;
				packet->Complete(error);
			}
		}
	}

	if (request->CloseHandle()) {
		OSHeapFree(request, sizeof(IORequest));
	}

	OSHeapFree(this, sizeof(IOPacket));
}

void IOPacket::QueuedChildren() {
	request->mutex.AssertLocked();
	remaining--;

	if (!remaining) {
		Complete(OS_SUCCESS);
	}
}

void IORequest::Cancel(OSError _error) {
	mutex.AssertLocked();
	error = _error;
	root->Complete(error);
	Complete();
}

void IORequest::Complete() {
	mutex.AssertLocked();
	kernelVMM.Free(buffer);
	complete.Set();
}

bool IORequest::CloseHandle() {
	mutex.AssertLocked();
	return --handles == 0;
}

#endif
