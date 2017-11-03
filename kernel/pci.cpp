#ifdef IMPLEMENTATION

#define PCI_CONFIG	0xCF8
#define PCI_DATA	0xCFC

enum PCIDeviceType {
	PCI_DEVICE_TYPE_UNKNOWN,
	PCI_DEVICE_TYPE_ATA,
	PCI_DEVICE_TYPE_AHCI,
	PCI_DEVICE_TYPE_FLOPPY,
	PCI_DEVICE_TYPE_ETHERNET,
	PCI_DEVICE_TYPE_VGA,
	PCI_DEVICE_TYPE_USB,
	PCI_DEVICE_TYPE_AUDIO,
	PCI_DEVICE_TYPE_PCI_BRIDGE,
};

struct PCIDevice {
	void WriteBAR32(uintptr_t index, uintptr_t offset, uint32_t value);
	uint32_t ReadBAR32(uintptr_t index, uintptr_t offset);
	void WriteBAR8(uintptr_t index, uintptr_t offset, uint8_t value);
	uint8_t ReadBAR8(uintptr_t index, uintptr_t offset);

	uint8_t classCode, subclassCode, progIF;
	uint8_t bus, device, function;

	uint8_t interruptPin, interruptLine;
	uint32_t baseAddresses[6];

	PCIDeviceType type;
};

uint8_t PCIDevice::ReadBAR8(uintptr_t index, uintptr_t offset) {
	uint32_t baseAddress = baseAddresses[index];

	if (baseAddress & 1) {
		uint8_t value = ProcessorIn8((baseAddress & ~3) + offset);
		return value;
#ifdef ARCH_X86_64
	} else if (!(baseAddress & 7)) {
		return *((volatile uint8_t *) (LOW_MEMORY_MAP_START + baseAddress + offset));
#endif
	} else {
		KernelPanic("PCIDevice::ReadBAR8 - Unimplemented.\n");
		return 0;
	}
}

void PCIDevice::WriteBAR8(uintptr_t index, uintptr_t offset, uint8_t value) {
	uint32_t baseAddress = baseAddresses[index];

	if (baseAddress & 1) {
		ProcessorOut8((baseAddress & ~3) + offset, value);
#ifdef ARCH_X86_64
	} else if (!(baseAddress & 7)) {
		*((volatile uint8_t *) (LOW_MEMORY_MAP_START + baseAddress + offset)) = value;
#endif
	} else {
		KernelPanic("PCIDevice::WriteBAR8 - Unimplemented.\n");
	}
}

uint32_t PCIDevice::ReadBAR32(uintptr_t index, uintptr_t offset) {
	uint32_t baseAddress = baseAddresses[index];

	if (baseAddress & 1) {
		return ProcessorIn32((baseAddress & ~3) + offset);
#ifdef ARCH_X86_64
	} else if (!(baseAddress & 7)) {
		return *((volatile uint32_t *) (LOW_MEMORY_MAP_START + baseAddress + offset));
#endif
	} else {
		KernelPanic("PCIDevice::ReadBAR32 - Unimplemented.\n");
		return 0;
	}
}

void PCIDevice::WriteBAR32(uintptr_t index, uintptr_t offset, uint32_t value) {
	uint32_t baseAddress = baseAddresses[index];

	if (baseAddress & 1) {
		ProcessorOut32((baseAddress & ~3) + offset, value);
#ifdef ARCH_X86_64
	} else if (!(baseAddress & 7)) {
		*((volatile uint32_t *) (LOW_MEMORY_MAP_START + baseAddress + offset)) = value;
#endif
	} else {
		KernelPanic("PCIDevice::WriteBAR32 - Unimplemented.\n");
	}
}

struct PCI {
	uint32_t ReadConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
	void WriteConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

	void ParseDeviceType(PCIDevice *pciDevice, uint8_t classCode, uint8_t subclassCode, uint8_t progIF);

#define PCI_BUS_DO_NOT_SCAN 0
#define PCI_BUS_SCAN_NEXT 1
#define PCI_BUS_SCANNED 2
	int busScanStates[256];

	void Enumerate();

	PCIDevice *devices;
	size_t devicesCount, devicesAllocated;
};

PCI pci;

uint32_t PCI::ReadConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
	if (offset & 3) KernelPanic("PCI::ReadConfig - offset is not 4-byte aligned.");
	ProcessorOut32(PCI_CONFIG, (uint32_t) (0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset));
	return ProcessorIn32(PCI_DATA);
}

void PCI::WriteConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
	if (offset & 3) KernelPanic("PCI::WriteConfig - offset is not 4-byte aligned.");
	ProcessorOut32(PCI_CONFIG, (uint32_t) (0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset));
	ProcessorOut32(PCI_DATA, value);
}

void PCI::ParseDeviceType(PCIDevice *pciDevice, uint8_t classCode, uint8_t subclassCode, uint8_t progIF) {
	pciDevice->type = PCI_DEVICE_TYPE_UNKNOWN;

	if (classCode == 0x01) {
		// Mass storage controller

		if (subclassCode == 0x01) {
			pciDevice->type = PCI_DEVICE_TYPE_ATA;
		} else if (subclassCode == 0x02) {
			pciDevice->type = PCI_DEVICE_TYPE_FLOPPY;
		} else if (subclassCode == 0x06 && progIF == 0x01) {
			pciDevice->type = PCI_DEVICE_TYPE_AHCI;
		}
	} else if (classCode == 0x02) {
		// Network controller

		if (subclassCode == 0x00) {
			pciDevice->type = PCI_DEVICE_TYPE_ETHERNET;
		}
	} else if (classCode == 0x03) {
		// Display controller

		if (subclassCode == 0x00) {
			pciDevice->type = PCI_DEVICE_TYPE_VGA;
		}
	} else if (classCode == 0x04) {
		// Multimedia Controller

		if (subclassCode == 0x01) {
			pciDevice->type = PCI_DEVICE_TYPE_AUDIO;
		}
	} else if (classCode == 0x06) {
		// Bus

		if (subclassCode == 0x04) {
			pciDevice->type = PCI_DEVICE_TYPE_PCI_BRIDGE;
		}
	} else if (classCode == 0x0C) {
		// Serial Bus Controllers

		if (subclassCode == 0x03) {
			pciDevice->type = PCI_DEVICE_TYPE_USB;
		}
	}
}

void PCI::Enumerate() {
	uint32_t baseHeaderType = ReadConfig(0, 0, 0, 0x0C);
	int baseBuses = (baseHeaderType & 0x80) ? 8 : 1;

	int busesToScan = 0;

	for (int baseBus = 0; baseBus < baseBuses; baseBus++) {
		uint32_t deviceID = ReadConfig(0, 0, baseBus, 0x00);
		if ((deviceID & 0xFFFF) == 0xFFFF) continue;
		busScanStates[baseBus] = PCI_BUS_SCAN_NEXT;
		busesToScan++;
	}

	if (!busesToScan) {
		KernelPanic("PCI::Enumerate - No buses found\n");
	}

	PCIDevice _device = {};

	while (busesToScan) {
		for (int bus = 0; bus < 256; bus++) {
			if (busScanStates[bus] == PCI_BUS_SCAN_NEXT) {
				busScanStates[bus] = PCI_BUS_SCANNED;
				busesToScan--;

				for (int device = 0; device < 32; device++) {
					uint32_t deviceID = ReadConfig(bus, device, 0, 0x00);
					if ((deviceID & 0xFFFF) == 0xFFFF) continue;

					uint32_t headerType = (ReadConfig(bus, device, 0, 0x0C) >> 16) & 0xFF;
					int functions = (headerType & 0x80) ? 8 : 1;

					for (int function = 0; function < functions; function++) {
						uint32_t deviceID = ReadConfig(bus, device, function, 0x00);
						if ((deviceID & 0xFFFF) == 0xFFFF) continue;

						PCIDevice *pciDevice = (PCIDevice *) ArrayAdd(pci.devices, _device);

						uint32_t deviceClass = ReadConfig(bus, device, function, 0x08);
						uint32_t interruptInformation = ReadConfig(bus, device, function, 0x3C);

						uint8_t classCode = (deviceClass >> 24) & 0xFF;
						uint8_t subclassCode = (deviceClass >> 16) & 0xFF;
						uint8_t progIF = (deviceClass >> 8) & 0xFF;

						pciDevice->classCode = classCode;
						pciDevice->subclassCode = subclassCode;
						pciDevice->progIF = progIF;

						pciDevice->bus = bus;
						pciDevice->device = device;
						pciDevice->function = function;
						
						pciDevice->interruptPin = (interruptInformation >> 8) & 0xFF;
						pciDevice->interruptLine = (interruptInformation >> 0) & 0xFF;

						for (int i = 0; i < 6; i++) {
							pciDevice->baseAddresses[i] = ReadConfig(bus, device, function, 0x10 + 4 * i);
						}

						ParseDeviceType(pciDevice, classCode, subclassCode, progIF);

						if (pciDevice->type == PCI_DEVICE_TYPE_PCI_BRIDGE) {
							uint8_t secondaryBus = (ReadConfig(bus, device, function, 0x18) >> 8) & 0xFF; 
							if (busScanStates[secondaryBus] == PCI_BUS_DO_NOT_SCAN) {
								busesToScan++;
								busScanStates[secondaryBus] = PCI_BUS_SCAN_NEXT;
							}
						}
					}
				}
			}
		}
	}

	for (uintptr_t i = 0; i < devicesCount; i++) {
		PCIDevice *device = devices + i;

		switch (device->type) {
			case PCI_DEVICE_TYPE_ATA: {
				// Enable busmastering DMA and interrupts.
				uint32_t previousCommand = ReadConfig(device->bus, device->device, device->function, 4);
				WriteConfig(device->bus, device->device, device->function, 4, ((1 << 2) | previousCommand) & ~(1 << 10));
				ATARegisterController(device);
			} break;

			case PCI_DEVICE_TYPE_AHCI: {
				// Enable busmastering DMA and interrupts.
				uint32_t previousCommand = ReadConfig(device->bus, device->device, device->function, 4);
				WriteConfig(device->bus, device->device, device->function, 4, ((1 << 2) | previousCommand) & ~(1 << 10));
				AHCIRegisterController(device);
			} break;

			default: {
			} break;
		}
	}
}

#endif
