#ifndef IMPLEMENTATION

struct PS2 {
	void Initialise();
	uint8_t ReadFromDataRegister();
	void WriteToDataRegister(uint8_t value);
	void SendCommand(uint8_t command);
};

PS2 ps2;

#else

#define PS2_DATA (0x60)
#define PS2_COMMAND (0x64)
#define PS2_STATUS (0x64)

// TODO Timeout.

uint8_t PS2::ReadFromDataRegister() {
	while (!(ProcessorIn8(PS2_STATUS) & 1));
	return ProcessorIn8(PS2_DATA);
}

void PS2::WriteToDataRegister(uint8_t value) {
	while (ProcessorIn8(PS2_STATUS) & 2);
	ProcessorOut8(PS2_DATA, value);
}

void PS2::SendCommand(uint8_t command) {
	ProcessorOut8(PS2_COMMAND, command);
}

bool PS2InterruptHandler(uintptr_t interruptIndex) {
	if (interruptIndex == 1) {
		if (ProcessorIn8(PS2_STATUS) & 1) {
			uint8_t keyboardByte = ps2.ReadFromDataRegister();
			KernelLog(LOG_VERBOSE, "Received PS/2 keyboard byte: 0x%X\n", keyboardByte);
			return true;
		} else {
			return false;
		}
	} else {
		// TODO Mouse driver.
		return false;
	}
}

void PS2::Initialise() {
	// Initialise the PS/2 controller.

	SendCommand(0xAD);
	SendCommand(0xA7);

	while (ProcessorIn8(PS2_STATUS) & 1) {
		ProcessorIn8(PS2_DATA);
	}

	SendCommand(0x20);
	uint8_t configByte = ReadFromDataRegister();
	configByte &= ~((1 << 0) | (1 << 1) | (1 << 6));
	SendCommand(0x60);
	WriteToDataRegister(configByte);

	SendCommand(0xAA);
	if (ReadFromDataRegister() != 0x55) {
		KernelLog(LOG_WARNING, "PS/2 controller replied with unexpected value to 0xAA command.\n");
	}

	SendCommand(0xAB);
	if (ReadFromDataRegister()) {
		KernelLog(LOG_WARNING, "PS/2 controller replied with unexpected value to 0xAB command.\n");
	}

	SendCommand(0xA9);
	if (ReadFromDataRegister()) {
		KernelLog(LOG_WARNING, "PS/2 controller replied with unexpected value to 0xA9 command.\n");
	}

	SendCommand(0xAE);
	SendCommand(0xA8);

	SendCommand(0x20);
	configByte = ReadFromDataRegister();
	configByte |= (1 << 0) | (1 << 1);
	SendCommand(0x60);
	WriteToDataRegister(configByte);

	if (!RegisterIRQHandler(1, PS2InterruptHandler)) {
		KernelLog(LOG_WARNING, "Could not register PS/2 controller IRQ 1.\n");
	}

	if (!RegisterIRQHandler(12, PS2InterruptHandler)) {
		KernelLog(LOG_WARNING, "Could not register PS/2 controller IRQ 12.\n");
	}
}

#endif
