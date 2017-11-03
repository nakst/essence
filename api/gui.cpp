#define BORDER_OFFSET_X (5)
#define BORDER_OFFSET_Y (29)

#define BORDER_SIZE_X (8)
#define BORDER_SIZE_Y (34)

static void SendCallback(OSControl *from, OSEventCallback &callback, OSEvent &event) {
	if (callback.callback) {
		callback.callback(from, callback.argument, &event);
	} else {
		switch (event.type) {
			case OS_EVENT_INVALID: {
				Panic();
			} break;

			case OS_EVENT_ACTION: {
				// We can't really do anything if the program doesn't want to handle the action.
			} break;

			case OS_EVENT_GET_LABEL: {
				// The program wants us to store the text.
				// Therefore, use the text we have stored in the control.
				event.getLabel.label = from->label;
				event.getLabel.labelLength = from->labelLength;
				event.getLabel.freeLabel = false;
			} break;
		}
	}
}

static void OSDrawControl(OSWindow *window, OSControl *control) {
	if (!window) return;

	int imageWidth = control->image.right - control->image.left;
	int imageHeight = control->image.bottom - control->image.top;

	bool isHoverControl = control == window->hoverControl;
	bool isPressedControl = control == window->pressedControl;
	intptr_t styleX = (control->disabled ? 3
			: ((isPressedControl && isHoverControl) ? 2 
			: ((isPressedControl || isHoverControl) ? 0 : 1)));
	styleX = (imageWidth + 1) * styleX + control->image.left;

	OSEvent labelEvent = {};
	labelEvent.type = OS_EVENT_GET_LABEL;
	SendCallback(control, control->getLabel, labelEvent);

	if (control->imageType == OS_CONTROL_IMAGE_FILL) {
		if (control->imageBorder.left) {
			OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
					control->bounds,
					control->image,
					control->imageBorder,
					OS_DRAW_MODE_REPEAT_FIRST);
		} else {
			OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
					control->bounds,
					OSRectangle(styleX, styleX + imageWidth, control->image.top, control->image.bottom),
					OSRectangle(styleX + 3, styleX + 5, control->image.top + 10, control->image.top + 11),
					OS_DRAW_MODE_REPEAT_FIRST);
		}

		OSDrawString(window->surface, control->bounds, 
				labelEvent.getLabel.label, labelEvent.getLabel.labelLength,
				OS_DRAW_STRING_HALIGN_CENTER | OS_DRAW_STRING_VALIGN_CENTER,
				control->disabled ? 0x808080 : 0x000000, -1);
	} else if (control->imageType == OS_CONTROL_IMAGE_CENTER_LEFT) {
		OSFillRectangle(window->surface, control->bounds, OSColor(0xF0, 0xF0, 0xF5));
		OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
				OSRectangle(control->bounds.left, control->bounds.left + control->fillWidth, 
					control->bounds.top, control->bounds.top + imageHeight),
				OSRectangle(styleX, styleX + imageWidth, control->image.top, control->image.bottom),
				OSRectangle(styleX + 3, styleX + 5, control->image.top + 10, control->image.top + 11),
				OS_DRAW_MODE_REPEAT_FIRST);
		OSDrawString(window->surface, 
				OSRectangle(control->bounds.left + control->fillWidth + 4, 
					control->bounds.right, control->bounds.top, control->bounds.bottom), 
				labelEvent.getLabel.label, labelEvent.getLabel.labelLength,
				OS_DRAW_STRING_HALIGN_LEFT | OS_DRAW_STRING_VALIGN_CENTER,
				control->disabled ? 0x808080 : 0x000000, 0xF0F0F5);
	} else if (control->imageType == OS_CONTROL_IMAGE_NONE) {
		OSFillRectangle(window->surface, control->bounds, OSColor(0xF0, 0xF0, 0xF5));
		OSDrawString(window->surface, control->bounds, 
				labelEvent.getLabel.label, labelEvent.getLabel.labelLength,
				OS_DRAW_STRING_HALIGN_CENTER | OS_DRAW_STRING_VALIGN_CENTER,
				control->disabled ? 0x808080 : 0x000000, 0xF0F0F5);
	}

	if (control->checked) {
		int checkY = 92 + 13 * control->checked - 13; 
		OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
				OSRectangle(control->bounds.left, control->bounds.left + control->fillWidth, 
					control->bounds.top, control->bounds.top + imageHeight),
				OSRectangle(96, 96 + 13, checkY, checkY + 13),
				OSRectangle(96 + 1, 96 + 2, checkY + 1, checkY + 2),
				OS_DRAW_MODE_REPEAT_FIRST);
	}


	if (labelEvent.getLabel.freeLabel) {
		OSHeapFree(labelEvent.getLabel.label);
	}

	window->dirty = true;
}

OSError OSSetControlLabel(OSControl *control, char *label, size_t labelLength, bool clone) {
	if (control->freeLabel) {
		OSHeapFree(control->label);
	}

	if (clone) {
		char *temp = (char *) OSHeapAllocate(labelLength, false);
		if (!temp) return OS_ERROR_COULD_NOT_ALLOCATE_MEMORY;
		OSCopyMemory(temp, label, labelLength);
		label = temp;
	}

	control->freeLabel = clone;
	control->label = label;
	control->labelLength = labelLength;
	control->getLabel.callback = nullptr;

	OSDrawControl(control->parent, control);

	return OS_SUCCESS;
}

OSError OSInvalidateControl(OSControl *control) {
	OSDrawControl(control->parent, control);
	return OS_SUCCESS;
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

void OSCheckControl(OSControl *control, int checked) {
	control->checked = checked;

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

OSControl *OSCreateControl(OSControlType type, char *label, size_t labelLength, bool cloneLabel) {
	OSControl *control = (OSControl *) OSHeapAllocate(sizeof(OSControl), true);
	if (!control) return nullptr;

	control->type = type;

	switch (type) {
		case OS_CONTROL_BUTTON: {
			control->bounds.right = 80;
			control->bounds.bottom = 21;
			control->imageType = OS_CONTROL_IMAGE_FILL;
			control->image = OSRectangle(42, 42 + 8, 88, 88 + 21);
		} break;

		case OS_CONTROL_GROUP: {
			control->bounds.right = 100;
			control->bounds.bottom = 100;
			control->imageType = OS_CONTROL_IMAGE_FILL;
			control->image = OSRectangle(20, 20 + 6, 85, 85 + 6);
			control->imageBorder = OSRectangle(22, 23, 87, 88);
		} break;

		case OS_CONTROL_CHECKBOX: {
			control->bounds.right = 21 + MeasureStringWidth(label, labelLength, GetGUIFontScale());
			control->bounds.bottom = 13;
			control->imageType = OS_CONTROL_IMAGE_CENTER_LEFT;
			control->fillWidth = 13;
			control->image = OSRectangle(42, 42 + 8, 110, 110 + 13);
		} break;

		case OS_CONTROL_RADIOBOX: {
			control->bounds.right = 21 + MeasureStringWidth(label, labelLength, GetGUIFontScale());
			control->bounds.bottom = 14;
			control->imageType = OS_CONTROL_IMAGE_CENTER_LEFT;
			control->fillWidth = 14;
			control->image = OSRectangle(116, 116 + 14, 42, 42 + 14);
		} break;

		case OS_CONTROL_STATIC: {
			control->bounds.right = 4 + MeasureStringWidth(label, labelLength, GetGUIFontScale());
			control->bounds.bottom = 14;
			control->imageType = OS_CONTROL_IMAGE_NONE;
		} break;
	}

	OSSetControlLabel(control, label, labelLength, cloneLabel);

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
	OSUpdateWindow(window);

	return window;
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

				if (previousPressedControl->type == OS_CONTROL_CHECKBOX) {
					// If this is a checkbox, then toggle the check.
					previousPressedControl->checked = previousPressedControl->checked ? OS_CONTROL_NO_CHECK : OS_CONTROL_CHECKED;
				} else if (previousPressedControl->type == OS_CONTROL_RADIOBOX) {
					// TODO If this is a radiobox,
					// 	clear the check on all other controls in the container,

					previousPressedControl->checked = OS_CONTROL_RADIO_CHECK;
				}

				OSDrawControl(window, previousPressedControl);

				if (window->hoverControl == previousPressedControl) { // TODO Still trigger the control if the mouse only just left its bounds?
					OSEvent event = {};
					event.type = OS_EVENT_ACTION;
					SendCallback(previousPressedControl, previousPressedControl->action, event);
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
