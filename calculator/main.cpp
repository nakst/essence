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
	char *path = (char *) "/os/new_dir/test2.txt";
	OSNodeInformation node;
	OSError error = OSOpenNode(path, OSCStringLength(path), 
			OS_OPEN_NODE_ACCESS_READ | OS_OPEN_NODE_ACCESS_RESIZE | OS_OPEN_NODE_ACCESS_WRITE | OS_OPEN_NODE_CREATE_DIRECTORIES, &node);
	OSPrint(":: error = %d\n", error);
	error = OSResizeFile(node.handle, 8);
	OSPrint(":: error = %d\n", error);
	char buffer[8];
	buffer[0] = 'a';
	buffer[1] = 'b';
	OSWriteFileSync(node.handle, 0, 1, buffer);
	buffer[0] = 'b';
	buffer[1] = 'c';
	size_t bytesRead = OSReadFileSync(node.handle, 0, 8, buffer);
	if (buffer[0] != 'a' || buffer[1] != 0) OSCrashProcess(101);
	OSPrint(":: bytesRead = %d\n", bytesRead);
	OSPrint(":: buffer = %s\n", 8, buffer);
	OSRefreshNodeInformation(&node);
	OSPrint(":: length = %d\n", node.fileSize);
	OSPrint(":: TID = %d\n", OSGetThreadID(OS_CURRENT_THREAD));

	{
		OSBatchCall calls[] = {
			{ OS_SYSCALL_PRINT, /*Return immediately on error?*/ true, /*Argument 0 and return value*/ (uintptr_t) "Hello ",    /*Argument 1*/ 6, /*Argument 2*/ 0, /*Argument 3*/ 0, },
			{ OS_SYSCALL_PRINT, /*Return immediately on error?*/ true, /*Argument 0 and return value*/ (uintptr_t) "batched",   /*Argument 1*/ 7, /*Argument 2*/ 0, /*Argument 3*/ 0, },
			{ OS_SYSCALL_PRINT, /*Return immediately on error?*/ true, /*Argument 0 and return value*/ (uintptr_t) " world!\n", /*Argument 1*/ 8, /*Argument 2*/ 0, /*Argument 3*/ 0, },
		};

		OSBatch(calls, sizeof(calls) / sizeof(OSBatchCall));
	}

	{
		char *path = (char *) "/os/sample_images";
		OSNodeInformation node;
		OSError error = OSOpenNode(path, OSCStringLength(path), OS_OPEN_NODE_DIRECTORY, &node);
		OSDirectoryChild buffer[16];

		if (error == OS_SUCCESS) {
			error = OSEnumerateDirectoryChildren(node.handle, buffer, 16);
		}

		OSPrint("error = %d\n", error);
		if (error != OS_SUCCESS) OSCrashProcess(100);
	}

	{
		char *path = (char *) "/os/test.txt";
		OSNodeInformation node;
		OSError error = OSOpenNode(path, OSCStringLength(path), 
				OS_OPEN_NODE_ACCESS_READ | OS_OPEN_NODE_ACCESS_RESIZE | OS_OPEN_NODE_ACCESS_WRITE,
				&node);
		if (error != OS_SUCCESS) OSCrashProcess(102);
		error = OSResizeFile(node.handle, 2048);
		if (error != OS_SUCCESS) OSCrashProcess(103);
		uint16_t buffer[1024];
		for (int i = 0; i < 1024; i++) buffer[i] = i;
		OSPrint("!!!!Starting tests!!!!!\n");
		OSWriteFileSync(node.handle, 0, 2048, buffer);
		for (int i = 0; i < 1024; i++) buffer[i] = i + 1024;
		OSHandle handle = OSWriteFileAsync(node.handle, 256, 2048 - 256 - 256, buffer + 128);
		// OSCancelIORequest(handle);
		OSWaitSingle(handle);
		OSIORequestProgress progress;
		OSGetIORequestProgress(handle, &progress);
		OSCloseHandle(handle);
		for (int i = 0; i < 1024; i++) buffer[i] = i + 2048;
		OSReadFileSync(node.handle, 0, 2048, buffer);
		for (int i = 0; i < 128; i++) if (buffer[i] != i) OSCrashProcess(107);
		for (int i = 1024 - 128; i < 1024; i++) if (buffer[i] != i) OSCrashProcess(108);
		for (int i = 128; i < 1024 - 128; i++) if (buffer[i] == i) OSCrashProcess(110); else if (buffer[i] != i + 1024) OSCrashProcess(109);
		OSPrint("Tests were a success!\n");

		OSCreateWindow((char *) "Test Program", 12, 320, 200, 0);
	}

#if 0
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
#endif

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
