#include "../api/os.h"

OSObject contentPane;

void Test(OSObject generator, void *argument, OSCallbackData *data) {
	(void) argument;
	char buffer[64];
	size_t bytes = OSFormatString(buffer, 64, "Generator = %x, type = %d", generator, data->type);
	OSSetControlText(OSGetControl(contentPane, 0, 1), buffer, bytes);
	OSPrint("%s\n", bytes, buffer);
}

extern "C" void ProgramEntry() {
	OSObject window = OSCreateWindow((char *) "Test Program", 12, 640, 400, OS_CREATE_WINDOW_WITH_MENU_BAR);
	contentPane = OSGetWindowContentPane(window);
	OSObject menuBar = OSGetWindowMenuBarPane(window);
	OSObject button1 = OSCreateControl(OS_CONTROL_BUTTON, (char *) "Button 1", 8, 0);
	OSObject button2 = OSCreateControl(OS_CONTROL_BUTTON, (char *) "Button 2", 8, 0);
	OSObject button3 = OSCreateControl(OS_CONTROL_BUTTON, (char *) "Button 3", 8, 0);
	OSObject textbox1 = OSCreateControl(OS_CONTROL_TEXTBOX, (char *) "Textbox 1", 9, 0);
	OSObject textbox2 = OSCreateControl(OS_CONTROL_TEXTBOX, (char *) "Textbox 2", 9, 0);
	OSObject textbox3 = OSCreateControl(OS_CONTROL_TEXTBOX, (char *) "Textbox 3", 9, 0);
	OSObject menu1 = OSCreateControl(OS_CONTROL_MENU, (char *) "Menu 1", 6, 0);
	OSObject menu2 = OSCreateControl(OS_CONTROL_MENU, (char *) "Menu 2", 6, 0);
	OSObject menu3 = OSCreateControl(OS_CONTROL_MENU, (char *) "Menu 3", 6, 0);
	OSConfigurePane(contentPane, 2, 3, 0);
	OSConfigurePane(menuBar, 3, 1, OS_CONFIGURE_PANE_NO_INDENT_V);
	OSSetPaneObject(OSGetPane(contentPane, 0, 0), button1, OS_SET_PANE_OBJECT_HORIZONTAL_LEFT | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(contentPane, 1, 1), button2, OS_SET_PANE_OBJECT_HORIZONTAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(contentPane, 0, 2), button3, OS_SET_PANE_OBJECT_HORIZONTAL_CENTER | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(contentPane, 1, 0), textbox1, OS_SET_PANE_OBJECT_VERTICAL_PUSH);
	OSSetPaneObject(OSGetPane(contentPane, 0, 1), textbox2, 0);
	OSSetPaneObject(OSGetPane(contentPane, 1, 2), textbox3, OS_SET_PANE_OBJECT_VERTICAL_PUSH);
	OSSetPaneObject(OSGetPane(menuBar, 0, 0), menu1, OS_SET_PANE_OBJECT_VERTICAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(menuBar, 1, 0), menu2, OS_SET_PANE_OBJECT_VERTICAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(menuBar, 2, 0), menu3, OS_SET_PANE_OBJECT_VERTICAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSLayoutPane(contentPane);
	OSLayoutPane(menuBar);
	OSSetObjectCallback(button1, OS_OBJECT_CONTROL, OS_CALLBACK_ACTION, Test, nullptr);

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
