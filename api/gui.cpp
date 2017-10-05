#define BORDER_OFFSET_X (5)
#define BORDER_OFFSET_Y (29)

#define BORDER_SIZE_X (8)
#define BORDER_SIZE_Y (34)

static void OSDrawControl(OSWindow *window, OSControl *control) {
	bool isHoverControl = control == window->hoverControl;
	intptr_t styleX = isHoverControl ? 42 : 51;

	OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
			OSRectangle(control->x, control->x + control->width,
				    control->y, control->y + control->height),
			OSRectangle(styleX, styleX + 8, 88, 88 + 21),
			OSRectangle(styleX + 3, styleX + 5, 88 + 10, 88 + 11),
			OS_DRAW_MODE_REPEAT_FIRST);

	OSUpdateWindow(window);
}

static bool OSControlHitTest(OSControl *control, int x, int y) {
	if (x >= control->x && x < control->x + control->width 
			&& y >= control->y && y < control->y + control->height) {
		return true;
	} else {
		return false;
	}
}

OSError OSAddControl(OSWindow *window, OSControl *control, int x, int y) {
	control->x = x + BORDER_OFFSET_X;
	control->y = y + BORDER_OFFSET_Y;
	window->controls[window->controlsCount++] = control;
	OSDrawControl(window, control);

	return OS_SUCCESS;
}

OSControl *OSCreateControl() {
	OSControl *control = (OSControl *) OSHeapAllocate(sizeof(OSControl));
	if (!control) return nullptr;

	control->width = 80;
	control->height = 21;

	return control;
}

OSWindow *OSCreateWindow(size_t width, size_t height) {
	OSWindow *window = (OSWindow *) OSHeapAllocate(sizeof(OSWindow));
	OSZeroMemory(window, sizeof(OSWindow));

	// Add the size of the border.
	width += BORDER_SIZE_X;
	height += BORDER_SIZE_Y;

	OSError result = OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) window, width, height, 0);

	if (result != OS_SUCCESS) {
		OSHeapFree(window);
		return nullptr;
	}

	// Draw the window background and border.
	OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, width, 0, height), 
			OSRectangle(96, 105, 42, 77), OSRectangle(96 + 3, 96 + 5, 42 + 29, 42 + 31), OS_DRAW_MODE_REPEAT_FIRST);
	OSUpdateWindow(window);

	return window;
}

OSError OSProcessGUIMessage(OSMessage *message) {
	// TODO Message security. 
	// 	How should we tell who sent the message?

	OSWindow *window = message->targetWindow;
	bool updateWindow = false;

	switch (message->type) {
		case OS_MESSAGE_MOUSE_MOVED: {
			OSControl *previousHoverControl = window->hoverControl;

			if (previousHoverControl) {
				if (!OSControlHitTest(previousHoverControl, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
					window->hoverControl = nullptr;
					OSDrawControl(window, previousHoverControl);
					updateWindow = true;
				}
			}

			for (uintptr_t i = 0; !window->hoverControl && i < window->controlsCount; i++) {
				OSControl *control = window->controls[i];

				if (OSControlHitTest(control, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
					window->hoverControl = control;
					OSDrawControl(window, window->hoverControl);
					updateWindow = true;
				}
			}
		} break;

		default: {
			return OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI;
		}
	}

	if (updateWindow) {
		OSUpdateWindow(window);
	}

	return OS_SUCCESS;
}
