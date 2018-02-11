#define UI_IMAGE(_image) _image.region, _image.border

struct UIImage {
	OSRectangle region;
	OSRectangle border;
};

UIImage activeWindowBorder;

struct Control : APIObject {
	unsigned layout;
	OSString text;
	OSRectangle bounds;
};

struct Grid : APIObject {
	unsigned columns, rows;
	OSObject *objects;
	OSRectangle bounds;
};

struct Window : APIObject {
	OSHandle window, surface;
	unsigned width, height;
	Grid *root;
	bool destroyed;
};

static OSCallbackResponse ProcessControlMessage(OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Control *control = (Control *) message->context;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			control->bounds = OSRectangle(
					message->layout.left, message->layout.right,
					message->layout.top, message->layout.bottom);
		} break;

		case OS_MESSAGE_PAINT: {
			OSDrawString(message->paint.surface, control->bounds, &control->text,
				OS_DRAW_STRING_HALIGN_CENTER | OS_DRAW_STRING_VALIGN_CENTER, 0, -1);
		} break;

		case OS_MESSAGE_DESTROY: {
			// TODO Free all the memory associated with the control.
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

OSObject OSCreateLabel(char *text, size_t textBytes) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;

	control->text.buffer = (char *) OSHeapAllocate(textBytes, false);
	control->text.bytes = textBytes;
	OSCopyMemory(control->text.buffer, text, textBytes);

	OSSetCallback(control, OSCallback(ProcessControlMessage, control));

	return control;
}

void OSAddControl(OSObject _grid, OSRectangle cells, OSObject _control, unsigned layout) {
	Grid *grid = (Grid *) _grid;

	if (cells.right > grid->columns || cells.bottom > grid->rows 
			|| cells.top < 0 || cells.left < 0 
			|| cells.top >= cells.bottom || cells.left >= cells.right) {
		OSCrashProcess(OS_FATAL_ERROR_OUT_OF_GRID_BOUNDS);
	}

	Control *control = (Control *) _control;
	control->layout = layout;

	for (int y = cells.top; y < cells.bottom; y++) {
		for (int x = cells.left; x < cells.right; x++) {
			OSObject *object = grid->objects + (y * grid->columns + x);
			if (*object) OSCrashProcess(OS_FATAL_ERROR_OVERWRITE_GRID_OBJECT);
			*object = control;
		}
	}
}

static OSCallbackResponse ProcessGridMessage(OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Grid *grid = (Grid *) message->context;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			OSSendMessage(grid->objects[0], message);
		} break;

		case OS_MESSAGE_PAINT: {
			OSSendMessage(grid->objects[0], message);
		} break;

		case OS_MESSAGE_DESTROY: {
			// TODO Free all the memory associated with the grid.
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

OSObject OSCreateGrid(unsigned columns, unsigned rows) {
	Grid *grid = (Grid *) OSHeapAllocate(sizeof(Grid), true);

	grid->columns = columns;
	grid->rows = rows;
	grid->objects = (OSObject *) OSHeapAllocate(sizeof(OSObject) * columns * rows, true);

	OSSetCallback(grid, OSCallback(ProcessGridMessage, grid));

	return grid;
}

static OSCallbackResponse ProcessWindowMessage(OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Window *window = (Window *) message->context;

	if (window->destroyed && message->type != OS_MESSAGE_WINDOW_DESTROYED) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	switch (message->type) {
		case OS_MESSAGE_WINDOW_CREATED: {
			OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, window->width, 0, window->height), UI_IMAGE(activeWindowBorder), OS_DRAW_MODE_REPEAT_FIRST);

			{
				OSMessage message;
				message.type = OS_MESSAGE_LAYOUT;
				message.layout.left = 0;
				message.layout.top = 0;
				message.layout.right = window->width;
				message.layout.bottom = window->height;
				OSSendMessage(window->root, &message);
			}

			{
				OSMessage message;
				message.type = OS_MESSAGE_PAINT;
				message.paint.surface = window->surface;
				OSSendMessage(window->root, &message);
			}

			OSSyscall(OS_SYSCALL_UPDATE_WINDOW, window->window, 0, 0, 0);
		} break;

		case OS_MESSAGE_WINDOW_DESTROYED: {
			OSHeapFree(window);
		} break;

		case OS_MESSAGE_DESTROY: {
			// TODO Free all the memory associated with the window.
			OSCloseHandle(window->surface);
			OSCloseHandle(window->window);
			window->destroyed = true;
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

OSObject OSCreateWindow(unsigned width, unsigned height, unsigned flags, char *title, size_t titleBytes) {
	Window *window = (Window *) OSHeapAllocate(sizeof(Window), true);
	window->type = API_OBJECT_WINDOW;

	OSRectangle bounds;
	bounds.left = 0;
	bounds.right = width;
	bounds.top = 0;
	bounds.bottom = height;

	OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) &window->window, (uintptr_t) &bounds, (uintptr_t) window, 0);
	OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, window->window, (uintptr_t) &bounds, 0, 0);

	window->width = bounds.right - bounds.left;
	window->height = bounds.bottom - bounds.top;

	OSSetCallback(window, OSCallback(ProcessWindowMessage, window));

	window->root = (Grid *) OSCreateGrid(1, 1);
	OSObject label = OSCreateLabel(OSLiteral("Hello, world!"));
	OSAddControl(window->root, OSRectangle(0, 1, 0, 1), label, 0);

	return window;
}

void OSInitialiseGUI() {
	activeWindowBorder = { {96, 105, 42, 77}, {96 + 3, 96 + 5, 42 + 29, 42 + 31} };
}
