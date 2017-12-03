#include "../api/os.h"

extern "C" void ProgramEntry() {
	OSObject window = OSCreateWindow((char *) "Calculator", 10, 320, 200, 0);
	(void) window;

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
