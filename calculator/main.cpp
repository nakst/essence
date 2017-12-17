#include "../api/os.h"

OSObject contentPane;

void Test(OSObject generator, void *argument, OSCallbackData *data) {
	(void) argument;
	char buffer[64];
	size_t bytes = OSFormatString(buffer, 64, "Generator = %x, type = %d", generator, data->type);
	OSSetControlText(OSGetControl(contentPane, 0, 1), buffer, bytes);
	OSPrint("%s\n", bytes, buffer);
}

enum MenuID {
	MENU_FILE,
	MENU_EDIT,
	MENU_RECENT,
};

void PopulateMenu(OSObject generator, void *argument, OSCallbackData *data) {
	(void) generator;
	(void) data;

	OSObject menu = data->populateMenu.popupMenu;

	switch ((uintptr_t) argument) {
		case MENU_FILE: {
			OSObject openItem = OSCreateControl(OS_CONTROL_MENU, (char *) "Open", 4, 0);
			OSObject saveItem = OSCreateControl(OS_CONTROL_MENU, (char *) "Save", 4, 0);
			OSObject recentItem = OSCreateControl(OS_CONTROL_MENU, (char *) "Recent", 6, OS_CONTROL_MENU_HAS_CHILDREN);

			OSSetMenuItems(menu, 3);
			OSSetMenuItem(menu, 0, openItem);
			OSSetMenuItem(menu, 1, saveItem);
			OSSetMenuItem(menu, 2, recentItem);

			OSSetObjectCallback(recentItem, OS_OBJECT_CONTROL, OS_CALLBACK_POPULATE_MENU, PopulateMenu, (void *) MENU_RECENT);
		} break;

		case MENU_RECENT:
		case MENU_EDIT: {
			OSObject copyItem = OSCreateControl(OS_CONTROL_MENU, (char *) "Copy", 4, 0);
			OSObject pasteItem = OSCreateControl(OS_CONTROL_MENU, (char *) "Paste", 5, 0);
			OSObject recurseItem = OSCreateControl(OS_CONTROL_MENU, (char *) "Recurse", 7, OS_CONTROL_MENU_HAS_CHILDREN);

			OSSetMenuItems(menu, 3);
			OSSetMenuItem(menu, 0, copyItem);
			OSSetMenuItem(menu, 1, pasteItem);
			OSSetMenuItem(menu, 2, recurseItem);

			OSSetObjectCallback(recurseItem, OS_OBJECT_CONTROL, OS_CALLBACK_POPULATE_MENU, PopulateMenu, (void *) MENU_FILE);
		} break;
	}
}

extern "C" void ProgramEntry() {
	OSObject window = OSCreateWindow((char *) "Test Program", 12, 320, 200, 
			OS_CREATE_WINDOW_WITH_MENU_BAR);

	OSObject menuBar = OSGetWindowMenuBar(window);
	OSSetMenuBarMenus(menuBar, 2);

	OSObject fileMenu = OSCreateControl(OS_CONTROL_MENU, (char *) "File", 4, OS_CONTROL_MENU_HAS_CHILDREN | OS_CONTROL_MENU_STYLE_BAR);
	OSObject editMenu = OSCreateControl(OS_CONTROL_MENU, (char *) "Edit", 4, OS_CONTROL_MENU_HAS_CHILDREN | OS_CONTROL_MENU_STYLE_BAR);

	OSSetMenuBarMenu(menuBar, 0, fileMenu);
	OSSetMenuBarMenu(menuBar, 1, editMenu);

	OSSetObjectCallback(fileMenu, OS_OBJECT_CONTROL, OS_CALLBACK_POPULATE_MENU, PopulateMenu, (void *) MENU_FILE);
	OSSetObjectCallback(editMenu, OS_OBJECT_CONTROL, OS_CALLBACK_POPULATE_MENU, PopulateMenu, (void *) MENU_EDIT);

#if 0
	OSObject window = OSCreateWindow((char *) "Test Program", 12, 640, 400, OS_CREATE_WINDOW_WITH_MENU_BAR);
	contentPane = OSGetWindowContentPane(window);
	OSObject menuBar = OSGetWindowMenuBar(window);
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
	OSSetMenuBarMenus(menuBar, 3);
	OSSetPaneObject(OSGetPane(contentPane, 0, 0), button1, OS_SET_PANE_OBJECT_HORIZONTAL_LEFT | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(contentPane, 1, 1), button2, OS_SET_PANE_OBJECT_HORIZONTAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(contentPane, 0, 2), button3, OS_SET_PANE_OBJECT_HORIZONTAL_CENTER | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
	OSSetPaneObject(OSGetPane(contentPane, 1, 0), textbox1, OS_SET_PANE_OBJECT_VERTICAL_PUSH);
	OSSetPaneObject(OSGetPane(contentPane, 0, 1), textbox2, 0);
	OSSetPaneObject(OSGetPane(contentPane, 1, 2), textbox3, OS_SET_PANE_OBJECT_VERTICAL_PUSH);
	OSSetMenuBarMenu(menuBar, 0, menu1);
	OSSetMenuBarMenu(menuBar, 1, menu2);
	OSSetMenuBarMenu(menuBar, 2, menu3);
	OSLayoutPane(contentPane);
	OSLayoutPane(menuBar);
	OSSetObjectCallback(button1, OS_OBJECT_CONTROL, OS_CALLBACK_ACTION, Test, nullptr);
#endif

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
