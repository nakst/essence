#include "../api/os.h"

extern "C" void ProgramEntry() {
	OSObject window = OSCreateWindow((char *) "Calculator", 10, 320, 200, 0);

#if 0
	OSObject contentPane = OSGetWindowContentPane(window);
	OSConfigurePane(contentPane, 3, 0, OS_CONFIGURE_PANE_HORIZONTAL);
	OSConfigurePane(OSGetPane(contentPane, 0), 3, 0, OS_CONFIGURE_PANE_VERTICAL | OS_CONFIGURE_PANE_NO_INDENT);
	OSConfigurePane(OSGetPane(contentPane, 1), 3, 0, OS_CONFIGURE_PANE_VERTICAL | OS_CONFIGURE_PANE_NO_INDENT);
	OSConfigurePane(OSGetPane(contentPane, 2), 3, 0, OS_CONFIGURE_PANE_VERTICAL | OS_CONFIGURE_PANE_NO_INDENT);

	for (int i = 0; i < 9; i++) {
		char buffer[1]; size_t bufferBytes = OSFormatString(buffer, 1, "%d", i + 1);

		OSObject control = OSCreateControl(OS_CONTROL_BUTTON, buffer, bufferBytes, 0);
		OSSetPaneObject(OSGetPane(OSGetPane(contentPane, i % 3), i / 3), control, OS_OBJECT_CONTROL);
	}
#else
	OSObject contentPane = OSGetWindowContentPane(window);
	OSConfigurePane(contentPane, 3, 0, OS_CONFIGURE_PANE_VERTICAL);

	char buffer[16];

	{
		size_t bufferBytes = OSFormatString(buffer, 16, "Button 1");
		OSObject control = OSCreateControl(OS_CONTROL_BUTTON, buffer, bufferBytes, 0);
		OSSetPaneObject(OSGetPane(contentPane, 0), control, OS_OBJECT_CONTROL);
	}

	{
		size_t bufferBytes = OSFormatString(buffer, 16, "Hello");
		OSObject control = OSCreateControl(OS_CONTROL_TEXTBOX, buffer, bufferBytes, 0);
		OSSetPaneObject(OSGetPane(contentPane, 1), control, OS_OBJECT_CONTROL);
	}

	{
		size_t bufferBytes = OSFormatString(buffer, 16, "Button 2");
		OSObject control = OSCreateControl(OS_CONTROL_BUTTON, buffer, bufferBytes, 0);
		OSSetPaneObject(OSGetPane(contentPane, 2), control, OS_OBJECT_CONTROL);
	}
#endif

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
