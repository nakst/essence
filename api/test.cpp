#include "../api/os.h"

#define OS_MANIFEST_DEFINITIONS
#include "../bin/os/test.manifest.h"

#include "../freetype/ft2build.h"
#include FT_FREETYPE_H

OSObject progressBar, window, scrollbar;

int x = 5;

struct y {
	int a, b;
};

y y2 = {1, 2};

int vp = 100;

OSCallbackResponse ScrollbarMoved(OSObject object, OSMessage *message) {
	(void) object;

	if (message->valueChanged.newValue == 300) {
		// OSSetScrollbarPosition(object, 200, false);
		// vp = 800;
		// OSSetScrollbarMeasurements(object, 400, 800);
	}

	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse Crash(OSObject object, OSMessage *message) {
	(void) object;
	(void) message;

	// OSPrint("object = %x\n", object);
	OSCreateMenu(osMenuTextboxContext, object, OS_CREATE_MENU_AT_SOURCE, OS_FLAGS_DEFAULT);

	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse ProcessActionOK(OSObject object, OSMessage *message) {
	(void) object;
	(void) message;

	static int progress = 0;
	progress++;
	if (progress <= 5) {
		// OSSetProgressBarValue(progressBar, progress);
	} else {
		OSMessage message;
		message.type = OS_MESSAGE_DESTROY;
		OSSendMessage(window, &message);
	}

	return OS_CALLBACK_HANDLED;
}

#if 0
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
#endif

jmp_buf buf;
int jmpState = 0;

void Function2() {
	if (jmpState++ != 2) OSCrashProcess(232);
	longjmp(buf, 0);
	OSCrashProcess(233);
}

int CompareIntegers(const void *a, const void *b) {
	uintptr_t *c = (uintptr_t *) a;
	uintptr_t *d = (uintptr_t *) b;
	return *c - *d;
}

OSCallbackResponse ToggleEnabled(OSObject object, OSMessage *message) {
	(void) object;
	OSPrint("context = %x\n", message->context);
	OSEnableControl(message->context, message->command.checked);
	OSSetScrollbarMeasurements(scrollbar, 10, 20);
	return OS_CALLBACK_HANDLED;
}

int z = 1;

extern "C" OSObject CreateMenuItem(OSMenuItem item);

extern "C" void ProgramEntry() {
	if (x != 5) OSCrashProcess(600);
	if (y2.a != 1) OSCrashProcess(601);
	if (y2.b != 2) OSCrashProcess(602);
	if (++z != 2) OSCrashProcess(603); // Is the data segment writable?

	{
		void *a = malloc(0x100000);
		void *b = realloc(a, 0x1000);
		void *c = realloc(b, 0x100000);
		free(c);
	}

	if (strcasecmp("abc", "AbC")) {
		OSCrashProcess(250);
	}

	if (strspn("123abc", "123456789") != 3) {
		OSCrashProcess(251);
	}

	if (strcspn("abc123", "123456789") != 3) {
		OSCrashProcess(252);
	}

	{
		const char *s = "abcabc";
		if (strchr(s, 'b') != s + 1) {
			OSCrashProcess(254);
		}
		if (strrchr(s, 'b') != s + 4) {
			OSCrashProcess(254);
		}
	}

	{
		char b[] = "abcdef";

		if (strlen(b) != 6 || strnlen(b, 3) != 3 || strnlen(b, 10) != 6) {
			OSCrashProcess(267);
		}
	}

	if (strchr("", 'a') || strrchr("", 'a')) {
		OSCrashProcess(255);
	}

	{
		char b[32];
		strcpy(b, "he");
		strcat(b, "llo");
		if (strcmp(b, "hello")) OSCrashProcess(253);
	}

	{
		char text[] = "hello,world:test";
		const char *delim = ":,";
		char *n = text;
		(void) n;
		(void) delim;
		// OSPrint("%z\n", strsep(&n, delim));
		// OSPrint("%z\n", strsep(&n, delim));
		// OSPrint("%z\n", strsep(&n, delim));
		// OSPrint("%x\n", strsep(&n, delim));
	}

	jmpState = 1;
	if (setjmp(buf) == 0) {
		if (jmpState++ != 1) OSCrashProcess(230);
		Function2();
		OSCrashProcess(231);
	} else {
		if (jmpState++ != 3) OSCrashProcess(234);
	}

	if (jmpState++ != 4) OSCrashProcess(235);
	if (strtol("\n\f\n -0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaAAAAAAAaaaaaa", nullptr, 0) != LONG_MIN) OSCrashProcess(236);
	char *x = (char *) "+03", *y;
	if (strtol(x, &y, 4) != 3 || y != x + 3) OSCrashProcess(237);

	{
		char *a = (char *) "hello";
		if (strstr(a, "hello") != a) OSCrashProcess(238);
		if (strstr(a, "hell") != a) OSCrashProcess(239);
		if (strstr(a, "ello") != a + 1) OSCrashProcess(240);
		if (strstr(a, "hello!")) OSCrashProcess(241);
		if (strstr(a, "l") != a + 2) OSCrashProcess(242);
	}

	{
		char *a = (char *) "hello";

		if (strcmp(a, "hdllo") <= 0) OSCrashProcess(243);
		if (strcmp(a, "hello") != 0) OSCrashProcess(244);
		if (strcmp(a, "hfllo") >= 0) OSCrashProcess(245);
		if (strcmp(a, "h") <= 0) OSCrashProcess(246);
		if (strcmp(a, "helloo") >= 0) OSCrashProcess(247);
	}

	{
		uintptr_t array[1000];
		for (uintptr_t i = 0; i < 1000; i++) array[i] = OSGetRandomByte();
		qsort(array, 1000, sizeof(uintptr_t), CompareIntegers);
		for (uintptr_t i = 1; i < 1000; i++) if (array[i] < array[i - 1]) OSCrashProcess(248);
	}

	{
		OSNodeInformation node = {};
		OSOpenNode(OSLiteral("/MapFile.txt"), 
				  OS_OPEN_NODE_RESIZE_EXCLUSIVE 
				| OS_OPEN_NODE_WRITE_ACCESS 
				| OS_OPEN_NODE_READ_ACCESS, &node);
		OSResizeFile(node.handle, 1048576);
		uint32_t *buffer = (uint32_t *) OSHeapAllocate(1048576, false);
		for (int i = 0; i < 262144; i++) buffer[i] = i;
		OSWriteFileSync(node.handle, 0, 1048576, buffer);
		OSReadFileSync(node.handle, 0, 1048576, buffer);
		for (uintptr_t i = 0; i < 262144; i++) if (buffer[i] != i) OSCrashProcess(201);
		uint32_t *pointer = (uint32_t *) OSMapObject(node.handle, 0, OS_MAP_OBJECT_ALL, OS_MAP_OBJECT_READ_WRITE);
		uint32_t *pointer2 = (uint32_t *) OSMapObject(node.handle, 0, OS_MAP_OBJECT_ALL, OS_MAP_OBJECT_READ_WRITE);
		for (uintptr_t i = 4096; i < 262144; i++) if (pointer[i] != buffer[i]) OSCrashProcess(200);
		for (uintptr_t i = 4096; i < 262144; i++) if (pointer2[i] != buffer[i]) OSCrashProcess(200);
		// pointer[0]++;
		OSFree(pointer);
		OSCloseHandle(node.handle);
		OSFree(pointer2);
	}

	{
		char m[256];
		char n[256];
		n[255] = 0;
		for (int i = 0; i < 256; i++) m[i] = i;
		OSCopyMemory(n, m, 255);
		m[255] = 0;
		for (int i = 0; i < 256; i++) if (m[i] != n[i]) OSCrashProcess(202);
	}

	char *path = (char *) "/os/new_dir/test2.txt";
	OSNodeInformation node;
	OSError error = OSOpenNode(path, OSCStringLength(path), 
			OS_OPEN_NODE_READ_ACCESS | OS_OPEN_NODE_RESIZE_ACCESS | OS_OPEN_NODE_WRITE_ACCESS | OS_OPEN_NODE_CREATE_DIRECTORIES, &node);
	if (error != OS_SUCCESS) OSCrashProcess(150);
	error = OSResizeFile(node.handle, 8);
	if (error != OS_SUCCESS) OSCrashProcess(151);
	char buffer[8];
	buffer[0] = 'a';
	buffer[1] = 'b';
	OSWriteFileSync(node.handle, 0, 1, buffer);
	buffer[0] = 'b';
	buffer[1] = 'c';
	size_t bytesRead = OSReadFileSync(node.handle, 0, 8, buffer);
	if (bytesRead != 8) OSCrashProcess(152);
	if (buffer[0] != 'a' || buffer[1] != 0) OSCrashProcess(101);
	OSRefreshNodeInformation(&node);

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

		if (error != OS_SUCCESS) OSCrashProcess(100);
	}

	{
		char *path = (char *) "/os/act.txt";

		OSNodeInformation node1, node2, node3;

		OSError error1 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_ACCESS, &node1);
		if (error1 != OS_SUCCESS) OSCrashProcess(120);

		OSError error2 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_ACCESS, &node2);
		if (error2 != OS_SUCCESS) OSCrashProcess(121);

		OSError error3 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_BLOCK, &node3);
		if (error3 != OS_ERROR_FILE_CANNOT_GET_EXCLUSIVE_USE) OSCrashProcess(122);

		OSCloseHandle(node1.handle);
		OSCloseHandle(node2.handle);

		OSError error6 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_BLOCK, &node1);
		if (error6 != OS_SUCCESS) OSCrashProcess(125);

		OSError error7 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_BLOCK, &node2);
		if (error7 != OS_SUCCESS) OSCrashProcess(126);

		OSCloseHandle(node1.handle);
		OSCloseHandle(node2.handle);

		OSError error8 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_EXCLUSIVE, &node1);
		if (error8 != OS_SUCCESS) OSCrashProcess(127);

		OSError error9 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_BLOCK, &node2);
		if (error9 != OS_ERROR_FILE_CANNOT_GET_EXCLUSIVE_USE) OSCrashProcess(128);

		OSError error10 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_EXCLUSIVE, &node2);
		if (error10 != OS_ERROR_FILE_CANNOT_GET_EXCLUSIVE_USE) OSCrashProcess(129);

		OSError error11 = OSOpenNode(path, OSCStringLength(path),
				OS_OPEN_NODE_READ_ACCESS, &node2);
		if (error11 != OS_ERROR_FILE_IN_EXCLUSIVE_USE) OSCrashProcess(130);
	}

	{
		char *path = (char *) "/os/test.txt";
		OSNodeInformation node;
		OSError error = OSOpenNode(path, OSCStringLength(path), 
				OS_OPEN_NODE_READ_ACCESS | OS_OPEN_NODE_RESIZE_ACCESS | OS_OPEN_NODE_WRITE_ACCESS,
				&node);
		if (error != OS_SUCCESS) OSCrashProcess(102);
		// error = OSResizeFile(node.handle, 2048);
		if (error != OS_SUCCESS) OSCrashProcess(103);
		uint16_t buffer[1024];
		for (int i = 0; i < 1024; i++) buffer[i] = i;
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

		// OSCreateWindow((char *) "Test Program", 12, 320, 200, 0);
	}

	{
		OSHandle region = OSOpenSharedMemory(512 * 1024 * 1024, nullptr, 0, 0);
		void *pointer = OSMapObject(region, 0, 0, OS_MAP_OBJECT_READ_WRITE);
		// Commented out because of the memory requirements.
#if 0
		OSResizeSharedMemory(region, 300 * 1024 * 1024); // Big -> big
		OSResizeSharedMemory(region, 200 * 1024 * 1024); // Big -> small
		OSResizeSharedMemory(region, 100 * 1024 * 1024); // Small -> small
		OSResizeSharedMemory(region, 400 * 1024 * 1024); // Small -> big
#endif
		OSCloseHandle(region);
		OSFree(pointer);
	}

#if 1
	{
		OSHandle region = OSOpenSharedMemory(512 * 1024 * 1024, nullptr, 0, 0);
		void *pointer = OSMapObject(region, 0, 0, OS_MAP_OBJECT_READ_WRITE);
		// OSZeroMemory(pointer, 512 * 1024 * 1024);
		OSCloseHandle(region);
		OSFree(pointer);
	}
#endif

	for (int i = 0; i < 3; i++) {
		OSHandle handle = OSOpenSharedMemory(1024, (char *) "Test", 4, OS_OPEN_SHARED_MEMORY_FAIL_IF_FOUND);
		OSHandle handle3 = OSOpenSharedMemory(0, (char *) "Test", 4, OS_OPEN_SHARED_MEMORY_FAIL_IF_NOT_FOUND);
		if (handle3 == OS_INVALID_HANDLE) OSCrashProcess(141);
		OSCloseHandle(handle);
		OSCloseHandle(handle3);
		OSHandle handle2 = OSOpenSharedMemory(0, (char *) "Test", 4, OS_OPEN_NODE_FAIL_IF_NOT_FOUND);
		if (handle2 != OS_INVALID_HANDLE) OSCrashProcess(140);
	}

	OSPrint("All tests completed successfully.\n");
	// OSCrashProcess(OS_FATAL_ERROR_INVALID_BUFFER);
	
	OSWindowSpecification ws = {};
	ws.width = 600;
	ws.height = 400;
	ws.minimumWidth = 100;
	ws.minimumHeight = 100;
	ws.title = (char *) "Hello, world!";
	ws.titleBytes = OSCStringLength(ws.title);
	ws.menubar = myMenuBar;
	window = OSCreateWindow(&ws);

	OSObject b;
	OSObject content = OSCreateGrid(4, 5, 0);
	OSObject scrollPane = OSCreateScrollPane(content, OS_CREATE_SCROLL_PANE_VERTICAL | OS_CREATE_SCROLL_PANE_HORIZONTAL);
	OSSetRootGrid(window, scrollPane);
	OSAddControl(content, 1, 1, b = OSCreateButton(actionOK), 0);

	OSAddControl(content, 1, 0, OSCreateLine(OS_ORIENTATION_HORIZONTAL), OS_CELL_H_EXPAND);

	OSAddControl(content, 0, 2, b = OSCreateTextbox(0), 0);

	OSSetCommandNotificationCallback(window, actionToggleEnabled, OS_MAKE_CALLBACK(ToggleEnabled, b));
	OSAddControl(content, 0, 1, OSCreateButton(actionToggleEnabled), 0);

	OSAddControl(content, 1, 2, OSCreateTextbox(0), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);

	OSAddControl(content, 0, 0, OSCreateIndeterminateProgressBar(), 0);
	OSAddControl(content, 0, 3, OSCreateButton(commandDeleteEverything), 0);

	{
		OSObject grid = OSCreateGrid(2, 2, OS_CREATE_GRID_DRAW_BOX);
		OSAddGrid(content, 1, 4, grid, OS_CELL_H_RIGHT);
		OSAddControl(grid, 0, 0, OSCreateTextbox(0), OS_CELL_H_LEFT);
		OSAddControl(grid, 0, 1, OSCreateButton(actionToggleEnabled), OS_CELL_H_LEFT);
	}

#if 1
#if 0
	scrollbar = OSCreateScrollbar(OS_ORIENTATION_HORIZONTAL);
	OSAddControl(content, 0, 4, scrollbar, OS_CELL_H_PUSH | OS_CELL_H_EXPAND | OS_CELL_V_CENTER);
#else
	scrollbar = OSCreateScrollbar(OS_ORIENTATION_VERTICAL);
	OSAddControl(content, 0, 4, scrollbar, OS_CELL_V_PUSH | OS_CELL_V_EXPAND | OS_CELL_H_CENTER);
#endif
	OSSetScrollbarMeasurements(scrollbar, 400, 100);
	OSSetObjectNotificationCallback(scrollbar, OS_MAKE_CALLBACK(ScrollbarMoved, nullptr));
#endif

	OSDisableCommand(window, actionToggleEnabled, false);
	OSDisableCommand(window, actionOK, false);


	OSProcessMessages();
}
