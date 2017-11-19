#include "../api/os.h"

void RunTests() {
#if 0
	{
		OSHandle event = OSCreateEvent(false);
		OSError error = OSSetEvent(event);
		uintptr_t result = OSWait(&event, 1, 100);
		OSPrint("%d/%d, wait result = %d\n", event, error, result);
	}
#endif
		
	OSNodeInformation file1;
	OSError file1Error = OSOpenNode((char *) "/os/test.txt", 12, OS_OPEN_NODE_ACCESS_READ | OS_OPEN_NODE_EXCLUSIVE_READ, &file1);

	OSPrint("file1.size = %d, file1Error = %d\n", file1.fileSize, file1Error);

	uint8_t buffer[32];
	size_t bytesRead = OSReadFileSync(file1.handle, 0, 32, buffer);

	// OSPrint("buffer contents = %s, bytesRead = %d\n", bytesRead - 1, buffer, bytesRead);

	OSCloseHandle(file1.handle);

	OSNodeInformation file2;
	OSError file2Error = OSOpenNode((char *) "/os/test.txt", 12, OS_OPEN_NODE_ACCESS_READ | OS_OPEN_NODE_ACCESS_WRITE, &file2);

	OSPrint("file2.size = %d, file2Error = %d\n", file2.fileSize, file2Error);

	buffer[0] = 'M';
	size_t bytesWritten = OSWriteFileSync(file2.handle, 0, 1, buffer);
	// OSCloseHandle(file2.handle);

	OSPrint("bytesWritten = %d\n", bytesWritten);

	OSZeroMemory(buffer, 32);

	OSNodeInformation file3;
	OSOpenNode((char *) "/os/test.txt", 12, OS_OPEN_NODE_ACCESS_READ 
			| OS_OPEN_NODE_ACCESS_RESIZE 
			| OS_OPEN_NODE_ACCESS_WRITE, &file3);
	bytesRead = OSReadFileSync(file3.handle, 0, 32, buffer);
	(void) bytesRead;
	// OSPrint("buffer contents = %s, bytesRead = %d\n", bytesRead - 1, buffer, bytesRead);

	// OSResizeFile(file3.handle, 10000);
	OSCloseHandle(file3.handle);
}

intptr_t entryValue = 0;
OSControl *textOutput;
char textOutputBuffer[1024];

void NumberButtonPressed(OSControl *generator, void *argument, OSCallbackData *data) {
	(void) generator;
	(void) data;

	intptr_t buttonValue = (intptr_t) argument;
	entryValue = entryValue * 10 + buttonValue;

	size_t length = OSFormatString(textOutputBuffer, 1024, "%d", entryValue);
	OSSetControlText(textOutput, textOutputBuffer, length, false);
}

extern "C" void ProgramEntry() {
	RunTests();

	OSWindow *window = OSCreateWindow(200, 150);

#if 1
	char *text = (char *) OSHeapAllocate(32, false);
	OSCopyMemory(text, (void *) "Hey!", 4);
	OSControl *textbox = OSCreateControl(OS_CONTROL_TEXTBOX, text, 4, true);
	textbox->textAllocated = 32;
	OSAddControl(window, textbox, 16, 16);
#else
	textOutput = OSCreateControl(OS_CONTROL_STATIC, (char *) "0", 1, false);
	textOutput->bounds.right = 200 - 32;
	textOutput->textBounds.right = 200 - 32;
	OSAddControl(window, textOutput, 16, 14);

	OSControl *groupBox = OSCreateControl(OS_CONTROL_GROUP, nullptr, 0, false);
	groupBox->bounds.right = 200 - 24;
	groupBox->bounds.bottom = 25;
	OSAddControl(window, groupBox, 12, 10);

	for (int i = 0; i <= 9; i++) {
		int y = ((i + 2) / 3) * -25 + 150 - 14 - 21;
		int x = ((i + 2) % 3) * 40 + 42;

		char label = '0' + i;
		OSControl *button = OSCreateControl(OS_CONTROL_BUTTON, &label, 1, true);
		button->bounds.right = 36;
		button->textBounds.right = 36;
		button->action.callback = NumberButtonPressed;
		button->action.argument = (void *) (intptr_t) i;
		OSAddControl(window, button, x, y);
	}
#endif

	while (true) {
		OSMessage message;
		OSWaitMessage(OS_WAIT_NO_TIMEOUT);

		if (OSGetMessage(&message) == OS_SUCCESS) {
			if (OS_SUCCESS == OSProcessGUIMessage(&message)) {
				continue;
#if 0
			} else if (message.type == OS_MESSAGE_KEYBOARD) {
				if (!(message.keyboard.scancode & OS_SCANCODE_KEY_RELEASED)) {
					switch (message.keyboard.scancode) {
						case OS_SCANCODE_0: NumberButtonPressed(nullptr, (void *) 0, nullptr); break;
						case OS_SCANCODE_1: NumberButtonPressed(nullptr, (void *) 1, nullptr); break;
						case OS_SCANCODE_2: NumberButtonPressed(nullptr, (void *) 2, nullptr); break;
						case OS_SCANCODE_3: NumberButtonPressed(nullptr, (void *) 3, nullptr); break;
						case OS_SCANCODE_4: NumberButtonPressed(nullptr, (void *) 4, nullptr); break;
						case OS_SCANCODE_5: NumberButtonPressed(nullptr, (void *) 5, nullptr); break;
						case OS_SCANCODE_6: NumberButtonPressed(nullptr, (void *) 6, nullptr); break;
						case OS_SCANCODE_7: NumberButtonPressed(nullptr, (void *) 7, nullptr); break;
						case OS_SCANCODE_8: NumberButtonPressed(nullptr, (void *) 8, nullptr); break;
						case OS_SCANCODE_9: NumberButtonPressed(nullptr, (void *) 9, nullptr); break;
					}

					OSUpdateWindow(window);
				}
#endif
			} else {
				// The message was not handled by the GUI.
				// We should do something with it.
				OSPrint("Calculator received unhandled message of type %d\n", message.type);
			}
		}
	}
}
