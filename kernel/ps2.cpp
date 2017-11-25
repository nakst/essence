// TODO Double-clicking doesn't work in Qemu?
// TODO Scrolling
// TODO Register the devices

#ifndef IMPLEMENTATION

struct PS2Update {
	union {
		struct {
			volatile int xMovement, yMovement;
			volatile unsigned buttons;
		};

		struct {
			volatile unsigned scancode;
		};
	};
};

struct PS2 {
	void Initialise();
	void EnableDevices(unsigned which);
	void DisableDevices(unsigned which);
	void FlushOutputBuffer();
	void SendCommand(uint8_t command);
	uint8_t ReadByte(Timer &timeout);
	void WriteByte(Timer &timeout, uint8_t value);
	bool SetupKeyboard(Timer &timeout);
	bool SetupMouse(Timer &timeout);
	bool PollRead(uint8_t *value, bool forMouse);

	uint8_t mouseType;
	size_t channels;

	volatile uintptr_t lastUpdatesIndex;
	PS2Update lastUpdates[16];
	Spinlock lastUpdatesLock;
};

PS2 ps2;

#else

// Status register.
#define PS2_OUTPUT_FULL 	(1 << 0)
#define PS2_INPUT_FULL 		(1 << 1)
#define PS2_MOUSE_BYTE		(1 << 5)

// Mouse types.
#define PS2_MOUSE_NORMAL	(0)
#define PS2_MOUSE_SCROLL	(3)
#define PS2_MOUSE_5_BUTTON	(4)

// Controller commands.
#define PS2_DISABLE_FIRST	(0xAD)
#define PS2_ENABLE_FIRST	(0xAE)
#define PS2_TEST_FIRST		(0xAB)
#define PS2_DISABLE_SECOND	(0xA7)
#define PS2_ENABLE_SECOND	(0xA8)
#define PS2_WRITE_SECOND	(0xD4)
#define PS2_TEST_SECOND		(0xA9)
#define PS2_TEST_CONTROLER	(0xAA)
#define PS2_READ_CONFIG		(0x20)
#define PS2_WRITE_CONFIG	(0x60)

// Controller configuration.
#define PS2_FIRST_IRQ_MASK 	(1 << 0)
#define PS2_SECOND_IRQ_MASK	(1 << 1)
#define PS2_SECOND_CLOCK	(1 << 5)
#define PS2_TRANSLATION		(1 << 6)

// IRQs.
#define PS2_FIRST_IRQ 		(1)
#define PS2_SECOND_IRQ		(12)

// Ports.
#define PS2_PORT_DATA		(0x60)
#define PS2_PORT_STATUS		(0x64)
#define PS2_PORT_COMMAND	(0x64)

// Keyboard commands.
#define PS2_KEYBOARD_RESET	(0xFF)
#define PS2_KEYBOARD_ENABLE	(0xF4)
#define PS2_KEYBOARD_DISABLE	(0xF5)
#define PS2_KEYBOARD_REPEAT	(0xF3)
#define PS2_KEYBOARD_SET_LEDS	(0xED)

// Mouse commands.
#define PS2_MOUSE_RESET		(0xFF)
#define PS2_MOUSE_ENABLE	(0xF4)
#define PS2_MOUSE_DISABLE	(0xF5)
#define PS2_MOUSE_SAMPLE_RATE	(0xF3)
#define PS2_MOUSE_READ		(0xEB)
#define PS2_MOUSE_RESOLUTION	(0xE8)

void PS2MouseUpdated(void *_update) {
	PS2Update *update = (PS2Update *) _update;

	windowManager.UpdateCursor(update->xMovement, update->yMovement, update->buttons);
#if 0
	if (update->xMovement || update->yMovement)
		windowManager.MoveCursor(update->xMovement, update->yMovement);
	windowManager.ClickCursor(update->buttons);
#endif
}

void PS2KeyboardUpdated(void *_update) {
	PS2Update *update = (PS2Update *) _update;
	windowManager.PressKey(update->scancode);
}

bool PS2::PollRead(uint8_t *value, bool forMouse) {
	uint8_t status = ProcessorIn8(PS2_PORT_STATUS);
	if (status & PS2_MOUSE_BYTE && !forMouse) return false;
	if (!(status & PS2_MOUSE_BYTE) && forMouse) return false;

	if (status & PS2_OUTPUT_FULL) {
		*value = ProcessorIn8(PS2_PORT_DATA);
		osRandomByteSeed ^= *value;
		return true;
	} else {
		return false;
	}
}

bool PS2IRQHandler(uintptr_t interruptIndex) {
	if (!ps2.channels) return false;
	if (ps2.channels == 1 && interruptIndex == 12) return false;

	if (interruptIndex == 12) {
		static uint8_t firstByte = 0, secondByte = 0, thirdByte = 0;
		static size_t bytesFound = 0;

		if (bytesFound == 0) {
			if (!ps2.PollRead(&firstByte, true)) return false;
			if (!(firstByte & 8)) return false;
			bytesFound++;
			return true;
		} else if (bytesFound == 1) {
			if (!ps2.PollRead(&secondByte, true)) return false;
			bytesFound++;
			return true;
		} else if (bytesFound == 2) {
			if (!ps2.PollRead(&thirdByte, true)) return false;
			bytesFound++;
		}

		// KernelLog(LOG_VERBOSE, "Mouse data: %X%X%X\n", firstByte, secondByte, thirdByte);

		ps2.lastUpdatesLock.Acquire();
		PS2Update *update = ps2.lastUpdates + ps2.lastUpdatesIndex;
		ps2.lastUpdatesIndex = (ps2.lastUpdatesIndex + 1) % 16;
		ps2.lastUpdatesLock.Release();

		update->xMovement = secondByte - ((firstByte << 4) & 0x100);
		update->yMovement = -(thirdByte - ((firstByte << 3) & 0x100));
		update->buttons = ((firstByte & (1 << 0)) ? LEFT_BUTTON : 0)
			      	| ((firstByte & (1 << 1)) ? RIGHT_BUTTON : 0)
				| ((firstByte & (1 << 2)) ? MIDDLE_BUTTON : 0);

		scheduler.lock.Acquire();
		RegisterAsyncTask(PS2MouseUpdated, update, kernelProcess, false);
		scheduler.lock.Release();

		firstByte = 0;
		secondByte = 0;
		thirdByte = 0;
		bytesFound = 0;
	} else if (interruptIndex == 1) {
		static uint8_t firstByte = 0, secondByte = 0, thirdByte = 0;
		static size_t bytesFound = 0;

		if (bytesFound == 0) {
			if (!ps2.PollRead(&firstByte, false)) return false;
			bytesFound++;
			if (firstByte != 0xE0 && firstByte != 0xF0) {
				goto sequenceFinished;
			} else return true;
		} else if (bytesFound == 1) {
			if (!ps2.PollRead(&secondByte, false)) return false;
			bytesFound++;
			if (secondByte != 0xF0) {
				goto sequenceFinished;
			} else return true;
		} else if (bytesFound == 2) {
			if (!ps2.PollRead(&thirdByte, false)) return false;
			bytesFound++;
			goto sequenceFinished;
		}

		sequenceFinished:
		// KernelLog(LOG_VERBOSE, "Keyboard data: %X%X%X\n", firstByte, secondByte, thirdByte);

		ps2.lastUpdatesLock.Acquire();
		PS2Update *update = ps2.lastUpdates + ps2.lastUpdatesIndex;
		ps2.lastUpdatesIndex = (ps2.lastUpdatesIndex + 1) % 16;
		ps2.lastUpdatesLock.Release();

		if (firstByte == 0xE0) {
			if (secondByte == 0xF0) {
				update->scancode = SCANCODE_KEY_RELEASED | (1 << 8) | thirdByte;
			} else {
				update->scancode = SCANCODE_KEY_PRESSED | (1 << 8) | secondByte;
			}
		} else {
			if (firstByte == 0xF0) {
				update->scancode = SCANCODE_KEY_RELEASED | (0 << 8) | secondByte;
			} else {
				update->scancode = SCANCODE_KEY_PRESSED | (0 << 8) | firstByte;
			}
		}

		scheduler.lock.Acquire();
		RegisterAsyncTask(PS2KeyboardUpdated, update, kernelProcess, false);
		scheduler.lock.Release();

		firstByte = 0;
		secondByte = 0;
		thirdByte = 0;
		bytesFound = 0;
	} else {
		KernelPanic("PS2IRQHandler - Incorrect interrupt index.\n", interruptIndex);
	}

	return true;
}

void PS2::DisableDevices(unsigned which) {
	if (which & 1) ProcessorOut8(PS2_PORT_COMMAND, PS2_DISABLE_FIRST);
	if (which & 2) ProcessorOut8(PS2_PORT_COMMAND, PS2_DISABLE_SECOND);
}

void PS2::EnableDevices(unsigned which) {
	if (which & 1) ProcessorOut8(PS2_PORT_COMMAND, PS2_ENABLE_FIRST);
	if (which & 2) ProcessorOut8(PS2_PORT_COMMAND, PS2_ENABLE_SECOND);
}

void PS2::FlushOutputBuffer() {
	while (ProcessorIn8(PS2_PORT_STATUS) & PS2_OUTPUT_FULL) {
		ProcessorIn8(PS2_PORT_DATA);
	}
}

void PS2::SendCommand(uint8_t command) {
	ProcessorOut8(PS2_PORT_COMMAND, command);
}

uint8_t PS2::ReadByte(Timer &timeout) {
	while (!(ProcessorIn8(PS2_PORT_STATUS) & PS2_OUTPUT_FULL) && !timeout.event.Poll());
	return ProcessorIn8(PS2_PORT_DATA);
}

void PS2::WriteByte(Timer &timeout, uint8_t value) {
	while ((ProcessorIn8(PS2_PORT_STATUS) & PS2_INPUT_FULL) && !timeout.event.Poll());
	ProcessorOut8(PS2_PORT_DATA, value);
}

bool PS2::SetupKeyboard(Timer &timeout) {
	ProcessorOut8(PS2_PORT_DATA, PS2_KEYBOARD_ENABLE);
	if (ReadByte(timeout) != 0xFA) return false;
	ProcessorOut8(PS2_PORT_DATA, PS2_KEYBOARD_REPEAT);
	if (ReadByte(timeout) != 0xFA) return false;
	ProcessorOut8(PS2_PORT_DATA, 0);
	if (ReadByte(timeout) != 0xFA) return false;

	return true;
}

bool PS2::SetupMouse(Timer &timeout) {
	// TODO Mouse with scroll wheel detection.

	ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_SECOND);
	ProcessorOut8(PS2_PORT_DATA, PS2_MOUSE_RESET);
	if (ReadByte(timeout) != 0xFA) return false;
	if (ReadByte(timeout) != 0xAA) return false;
	if (ReadByte(timeout) != 0x00) return false;
	ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_SECOND);
	ProcessorOut8(PS2_PORT_DATA, PS2_MOUSE_SAMPLE_RATE);
	if (ReadByte(timeout) != 0xFA) return false;
	ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_SECOND);
	ProcessorOut8(PS2_PORT_DATA, 100);
	if (ReadByte(timeout) != 0xFA) return false;
	ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_SECOND);
	ProcessorOut8(PS2_PORT_DATA, PS2_MOUSE_RESOLUTION);
	if (ReadByte(timeout) != 0xFA) return false;
	ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_SECOND);
	ProcessorOut8(PS2_PORT_DATA, 3);
	if (ReadByte(timeout) != 0xFA) return false;
	ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_SECOND);
	ProcessorOut8(PS2_PORT_DATA, PS2_MOUSE_ENABLE);
	if (ReadByte(timeout) != 0xFA) return false;

	return true;
}

void PS2::Initialise() {
	Timer timeout = {};
	timeout.Set(100, false);
	Defer(timeout.Remove());

	FlushOutputBuffer();

	RegisterIRQHandler(PS2_FIRST_IRQ, PS2IRQHandler);
	RegisterIRQHandler(PS2_SECOND_IRQ, PS2IRQHandler);

	// TODO USB initilisation.
	// TODO PS/2 detection with ACPI.

	DisableDevices(1 | 2);
	FlushOutputBuffer();

	ProcessorOut8(PS2_PORT_COMMAND, PS2_READ_CONFIG);
	uint8_t configurationByte = ReadByte(timeout);
	ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_CONFIG);
	WriteByte(timeout, configurationByte & ~(PS2_FIRST_IRQ_MASK | PS2_SECOND_IRQ_MASK | PS2_TRANSLATION));

	SendCommand(PS2_TEST_CONTROLER);
	if (ReadByte(timeout) != 0x55) return;

	bool hasMouse = false;
	if (configurationByte & PS2_SECOND_CLOCK) {
		EnableDevices(2);
		ProcessorOut8(PS2_PORT_COMMAND, PS2_READ_CONFIG);
		configurationByte = ReadByte(timeout);
		if (!(configurationByte & PS2_SECOND_CLOCK)) {
			hasMouse = true;
			DisableDevices(2);
		}
	}

	{
		ProcessorOut8(PS2_PORT_COMMAND, PS2_TEST_FIRST);
		if (ReadByte(timeout)) return;
		if (timeout.event.Poll()) return;
		channels = 1;
	}

	if (hasMouse) {
		ProcessorOut8(PS2_PORT_COMMAND, PS2_TEST_SECOND);
		if (!ReadByte(timeout) && !timeout.event.Poll()) channels = 2;
	}

	EnableDevices(1 | 2);

	if (!SetupKeyboard(timeout) || timeout.event.Poll()) {
		channels = 0;
		return;
	}

	if (!SetupMouse(timeout) || timeout.event.Poll()) {
		channels = 1;
	}

	{
		ProcessorOut8(PS2_PORT_COMMAND, PS2_READ_CONFIG);
		uint8_t configurationByte = ReadByte(timeout);
		ProcessorOut8(PS2_PORT_COMMAND, PS2_WRITE_CONFIG);
		WriteByte(timeout, configurationByte | PS2_FIRST_IRQ_MASK | PS2_SECOND_IRQ_MASK);
	}

	KernelLog(LOG_VERBOSE, "Setup PS/2 controller%z.\n", channels == 2 ? ", with a mouse" : "");
}

#endif
