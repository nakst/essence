#include "../api/os.h"

extern "C" void ProgramEntry() {
	OSObject window = OSCreateWindow((char *) "Calculator", 10, 320, 200, 0);

	OSObject contentPane = OSGetWindowContentPane(window);
	OSSetPaneColumns(contentPane, 3, 0, 0);

	for (int i = 0; i < 3; i++) {
		char buffer[16];
		size_t bufferBytes = OSFormatString(buffer, 16, "Button %d", i);

		OSObject control = OSCreateControl(OS_CONTROL_BUTTON, buffer, bufferBytes, 0);
		OSSetPaneObject(OSGetPane(contentPane, i), control, OS_OBJECT_CONTROL);
	}

	OSLayoutPane(contentPane);

	while (true) {
		OSMessage message;
		OSWaitMessage(OS_WAIT_NO_TIMEOUT);

		if (OSGetMessage(&message) == OS_SUCCESS) {
			if (OS_SUCCESS == OSProcessGUIMessage(&message)) {
				continue;
			} else {
				// Message not handled.
			}
		}
	}
}
