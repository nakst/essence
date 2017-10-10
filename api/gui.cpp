#define BORDER_OFFSET_X (5)
#define BORDER_OFFSET_Y (29)

#define BORDER_SIZE_X (8)
#define BORDER_SIZE_Y (34)

static void OSDrawControl(OSWindow *window, OSControl *control) {
	bool isHoverControl = control == window->hoverControl;
	bool isPressedControl = control == window->pressedControl;
	intptr_t styleX = (control->disabled ? 69
			: ((isPressedControl && isHoverControl) ? 60 : ((isPressedControl || isHoverControl) ? 42 : 51)));

	OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
			control->bounds,
			OSRectangle(styleX, styleX + 8, 88, 88 + 21),
			OSRectangle(styleX + 3, styleX + 5, 88 + 10, 88 + 11),
			OS_DRAW_MODE_REPEAT_FIRST);

	window->dirty = true;
}

static bool OSControlHitTest(OSControl *control, int x, int y) {
	if (x >= control->bounds.left && x < control->bounds.right
			&& y >= control->bounds.top && y < control->bounds.bottom) {
		return true;
	} else {
		return false;
	}
}

void OSDisableControl(OSControl *control, bool disabled) {
	control->disabled = disabled;

	if (disabled) {
		if (control == control->parent->hoverControl)   control->parent->hoverControl = nullptr;
		if (control == control->parent->pressedControl) control->parent->pressedControl = nullptr;
	}

	OSDrawControl(control->parent, control);
}

OSError OSAddControl(OSWindow *window, OSControl *control, int x, int y) {
	control->bounds.left    = x + BORDER_OFFSET_X;
	control->bounds.top     = y + BORDER_OFFSET_Y;
	control->bounds.right  += x + BORDER_OFFSET_X;
	control->bounds.bottom += y + BORDER_OFFSET_Y;

	control->parent = window;
	window->dirty = true;

	window->controls[window->controlsCount++] = control;
	OSDrawControl(window, control);

	return OS_SUCCESS;
}

OSControl *OSCreateControl(OSControlType type) {
	OSControl *control = (OSControl *) OSHeapAllocate(sizeof(OSControl), true);
	if (!control) return nullptr;

	control->type = type;
	control->bounds.right = 80;
	control->bounds.bottom = 21;

	return control;
}

OSWindow *OSCreateWindow(size_t width, size_t height) {
	OSWindow *window = (OSWindow *) OSHeapAllocate(sizeof(OSWindow), true);

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
	OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(width - 4 - 95 + 28, width - 4, 5, 5 + 43 - 22), 
			OSRectangle(28, 95, 22, 43), OSRectangle(30, 31, 23, 24), OS_DRAW_MODE_REPEAT_FIRST);
	OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(width - 4 - 95 + 28, width - 4, 5, 5 + 43 - 22), 
			OSRectangle(28, 95, 44, 65), OSRectangle(30, 31, 45, 46), OS_DRAW_MODE_REPEAT_FIRST);
	OSUpdateWindow(window);

	return window;
}

static void SendCallback(OSControl *from, OSEventCallback *callback) {
	if (callback->callback) {
		callback->callback(from, callback->argument);
	}
}

OSError OSProcessGUIMessage(OSMessage *message) {
	// TODO Message security. 
	// 	How should we tell who sent the message?
	// 	(and that they gave us a valid window?)

	OSWindow *window = message->targetWindow;

	switch (message->type) {
		case OS_MESSAGE_MOUSE_MOVED: {
			OSControl *previousHoverControl = window->hoverControl;

			if (previousHoverControl) {
				if (!OSControlHitTest(previousHoverControl, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
					window->hoverControl = nullptr;
					OSDrawControl(window, previousHoverControl);
				}
			}

			for (uintptr_t i = 0; !window->hoverControl && i < window->controlsCount; i++) {
				OSControl *control = window->controls[i];

				if (OSControlHitTest(control, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
					window->hoverControl = control;
					OSDrawControl(window, window->hoverControl);
				}
			}
		} break;

		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			if (window->hoverControl) {
				window->pressedControl = window->hoverControl;
				OSDrawControl(window, window->pressedControl);
			}
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			if (window->pressedControl) {
				OSControl *previousPressedControl = window->pressedControl;
				window->pressedControl = nullptr;
				OSDrawControl(window, previousPressedControl);

				if (window->hoverControl == previousPressedControl) {
					SendCallback(previousPressedControl, &previousPressedControl->action);
				}
			}
		} break;

		case OS_MESSAGE_WINDOW_CREATED: {
			window->dirty = true;
		} break;

		default: {
			return OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI;
		}
	}

	if (window->dirty) {
		OSUpdateWindow(window);
		window->dirty = false;
	}

	return OS_SUCCESS;
}
