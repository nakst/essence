#include "../api/os.h"

intptr_t entryValue = 0;
OSControl *textOutput;
char textOutputBuffer[1024];

void NumberButtonPressed(OSControl *generator, void *argument, OSEvent *event) {
	(void) generator;
	(void) event;

	intptr_t buttonValue = (intptr_t) argument;
	entryValue = entryValue * 10 + buttonValue;

	size_t length = OSFormatString(textOutputBuffer, 1024, "%d", entryValue);
	OSSetControlLabel(textOutput, textOutputBuffer, length, false);
}

extern "C" void ProgramEntry() {
	OSWindow *window = OSCreateWindow(200, 150);
	textOutput = OSCreateControl(OS_CONTROL_STATIC, (char *) "0", 1, false);
	textOutput->bounds.right = 200 - 32;
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
		button->action.callback = NumberButtonPressed;
		button->action.argument = (void *) (intptr_t) i;
		OSAddControl(window, button, x, y);
	}

	while (true) {
		OSMessage message;
		OSWaitMessage(OS_WAIT_NO_TIMEOUT);

		if (OSGetMessage(&message) == OS_SUCCESS) {
			if (OS_SUCCESS == OSProcessGUIMessage(&message)) {
				continue;
			}

			// The message was not handled by the GUI.
			// We should do something with it.
			OSPrint("Calculator received unhandled message of type %d\n", message.type);
		}
	}
}
