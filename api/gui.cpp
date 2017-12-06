// TODO Textboxes
//	- Clipboard and undo
//	- Scrolling (including drag selections)
//	- Actually use the OS_CALLBACK_GET_TEXT callback for everything.
//	- Heap corruption?

struct OSCallback {
	_OSCallback callback;
	void *argument;
};

struct Control {
	OSControlType type;
	struct Pane *pane;
	bool dirty;

	// Position and size:
	OSRectangle bounds;
	unsigned preferredWidth, preferredHeight;
	unsigned textBoundsBorder;

	// Properties:
#define OS_CONTROL_ENABLED  (false)
#define OS_CONTROL_DISABLED (true)
	bool disabled;
	struct Window *window;

	// Callbacks:
	OSCallback action;
	OSCallback getText;
	OSCallback insertText;
	OSCallback removeText;

	// Style:
	OSRectangle image;
	OSRectangle imageBorder;
	OSCursorStyle cursorStyle;
	OSControlImageType imageType;
	bool manualImage;
	int fillWidth;
	unsigned textAlign;
	bool canHaveFocus;
	int caretBlink;
	bool textShadow;
	int fontSize;

	// Misc:
	OSCaret wordSelectionAnchor, wordSelectionAnchor2;
	uint8_t resizeRegionIndex;

	// State:

	OSCaret caret, caret2;
	OSString text;
};

struct Pane {
	struct Window *window;
	struct Pane *parent;
	Control *control;
	OSRectangle bounds;
	unsigned preferredWidth, preferredHeight;

	struct Pane *grid;
	size_t gridWidth, gridHeight;

	unsigned flags;
	bool dirty;
};

struct Window {
	OSHandle handle;
	OSHandle surface;

	size_t width, height;

	Pane pane;

	Control *hoverControl, *previousHoverControl;
	Control *pressedControl;
	Control *focusedControl;

	bool dirty;
};

#define BORDER_OFFSET_X (5)
#define BORDER_OFFSET_Y (29)

#define BORDER_SIZE_X (8)
#define BORDER_SIZE_Y (34)

static void SendCallback(Control *from, OSCallback &callback, OSCallbackData &data) {
	if (callback.callback) {
		callback.callback(from, callback.argument, &data);
	} else {
		switch (data.type) {
			case OS_CALLBACK_ACTION: {
				// We can't really do anything if the program doesn't want to handle the action.
			} break;

			case OS_CALLBACK_GET_TEXT: {
				// The program wants us to store the text.
				// Therefore, use the text we have stored in the control.
				*data.getText.string = from->text;
				data.getText.freeText = false;
			} break;

			case OS_CALLBACK_INSERT_TEXT: {
				OSString *string = &from->text;
				OSString *insert = data.insertText.string;
				OSCaret *caret = data.insertText.caret;

				if (string->allocated <= string->bytes + insert->bytes) {
					string->buffer = (char *) realloc(string->buffer, (string->allocated = string->bytes + insert->bytes + 16));
				}

				char *text = string->buffer;
				text += caret->byte;

				memmove(text + insert->bytes, text, string->bytes - (text - string->buffer));
				OSCopyMemory(text, insert->buffer, insert->bytes);

				string->bytes += insert->bytes;
				string->characters += insert->characters;
			} break;

			case OS_CALLBACK_REMOVE_TEXT: {
				char *text = from->text.buffer + data.removeText.caretStart->byte;
				char *text2 = from->text.buffer + data.removeText.caretEnd->byte;

				size_t bytes = text2 - text;
				size_t characters = data.removeText.caretEnd->character - data.removeText.caretStart->character;

				memmove(text, text2, from->text.bytes - (text2 - from->text.buffer));

				from->text.bytes -= bytes;
				from->text.characters -= characters;
			} break;

			default: {
				Panic();
			} break;
		}
	}
}

static OSRectangle GetPaneBounds(Pane *pane) {
	OSRectangle rectangle = pane->bounds;

#if 0
	pane = pane->parent;

	while (pane) {
		rectangle.left   += pane->bounds.left;
		rectangle.right  += pane->bounds.left;
		rectangle.top    += pane->bounds.top;
		rectangle.bottom += pane->bounds.top;

		pane = pane->parent;
	}
#endif

	return rectangle;
}

static OSRectangle GetControlBounds(Control *control) {
	return control->bounds;
}

static OSRectangle GetControlTextBounds(Control *control) {
	OSRectangle textBounds = GetControlBounds(control);
	textBounds.left += control->textBoundsBorder;
	textBounds.right -= control->textBoundsBorder;
	textBounds.top += control->textBoundsBorder;
	textBounds.bottom -= control->textBoundsBorder;
	return textBounds;
}

static void DrawControl(Control *control) {
	Window *window = control->window;

	if (!window || control->imageType == OS_CONTROL_IMAGE_TRANSPARENT) return;

	int imageWidth = control->image.right - control->image.left;
	int imageHeight = control->image.bottom - control->image.top;

	bool isHoverControl = (control == window->hoverControl) || (control == window->focusedControl);
	bool isPressedControl = (control == window->pressedControl) || (control == window->focusedControl);

	if (control->manualImage) {
		isHoverControl = true;
		isPressedControl = false;
	}

	intptr_t styleX = (control->disabled ? 3
			: ((isPressedControl && isHoverControl) ? 2 
			: ((isPressedControl || isHoverControl) ? 0 : 1)));
	styleX = (imageWidth + 1) * styleX + control->image.left;

	OSCallbackData textEvent = {};
	OSString controlText;
	textEvent.type = OS_CALLBACK_GET_TEXT;
	textEvent.getText.string = &controlText;
	SendCallback(control, control->getText, textEvent);

	OSRectangle textBounds = GetControlTextBounds(control);
	OSRectangle bounds = GetControlBounds(control);

	if (control->imageType == OS_CONTROL_IMAGE_FILL) {
		if (control->imageBorder.left) {
			OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
					bounds,
					control->image,
					control->imageBorder,
					OS_DRAW_MODE_REPEAT_FIRST);
		} else {
			OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
					bounds,
					OSRectangle(styleX, styleX + imageWidth, control->image.top, control->image.bottom),
					OSRectangle(styleX + 3, styleX + 5, control->image.top + 10, control->image.top + 11),
					OS_DRAW_MODE_REPEAT_FIRST);
		}

		if (control->textShadow) {
#define SHADOW_OFFSET (1)
			DrawString(window->surface, OSRectangle(textBounds.left + SHADOW_OFFSET, textBounds.right + SHADOW_OFFSET,
								textBounds.top + SHADOW_OFFSET, textBounds.bottom + SHADOW_OFFSET), 
					&controlText,
					control->textAlign,
					control->disabled ? 0x808080 : 0x000000, -1, 0xFFAFC6EA,
					OSPoint(0, 0), nullptr, -1, 0, false, control->fontSize);
		}
		
		DrawString(window->surface, textBounds, 
				&controlText,
				control->textAlign,
				control->disabled ? 0x808080 : (control->textShadow ? 0xFFFFFF : 0x000000), -1, 0xFFAFC6EA,
				OSPoint(0, 0), nullptr, control == window->focusedControl && !control->disabled ? control->caret.character : -1, 
				control->caret2.character, control->caretBlink == 2, control->fontSize);
	} else if (control->imageType == OS_CONTROL_IMAGE_CENTER_LEFT) {
		OSFillRectangle(window->surface, bounds, OSColor(0xF0, 0xF0, 0xF5));
		OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
				OSRectangle(bounds.left, bounds.left + control->fillWidth, 
					bounds.top, bounds.top + imageHeight),
				OSRectangle(styleX, styleX + imageWidth, control->image.top, control->image.bottom),
				OSRectangle(styleX + 3, styleX + 5, control->image.top + 10, control->image.top + 11),
				OS_DRAW_MODE_REPEAT_FIRST);
		OSDrawString(window->surface, 
				textBounds, 
				&controlText,
				control->textAlign,
				control->disabled ? 0x808080 : 0x000000, 0xF0F0F5);
	} else if (control->imageType == OS_CONTROL_IMAGE_NONE) {
		OSFillRectangle(window->surface, bounds, OSColor(0xF0, 0xF0, 0xF5));
		OSDrawString(window->surface, textBounds, 
				&controlText,
				control->textAlign,
				control->disabled ? 0x808080 : 0x000000, 0xF0F0F5);
	}

	if (textEvent.getText.freeText) {
		OSHeapFree(controlText.buffer);
	}
}

static void DrawPane(Pane *pane, bool force) {
	if (pane->dirty == false && !force) {
		return;
	} else {
		pane->dirty = false;
	}

	if (pane->control && (pane->control->dirty || force)) {
		DrawControl(pane->control);
		pane->control->dirty = false;
	}

	for (uintptr_t i = 0; i < pane->gridWidth * pane->gridHeight; i++) {
		DrawPane(pane->grid + i, force);
	}
}

void OSSetControlText(Control *control, char *text, size_t textLength) {
	if (control->text.buffer) {
		OSHeapFree(control->text.buffer);
	}

	if (control->getText.callback) {
		Panic();
	}

	char *temp = (char *) OSHeapAllocate(textLength, false);
	if (!temp && textLength) OSCrashProcess(OS_FATAL_ERROR_COULD_NOT_ALLOCATE_MEMORY);
	OSCopyMemory(temp, text, textLength);
	text = temp;

	OSString *string = &control->text;
	string->buffer = text;
	string->bytes = textLength;
	string->allocated = textLength;
	string->characters = utf8_length(text, textLength);

	OSInvalidateControl(control);
}

void OSInvalidateControl(OSObject _control) {
	Control *control = (Control *) _control;
	control->dirty = true;

	Pane *pane = control->pane;
	while (pane) {
		pane->dirty = true;
		pane->window->dirty = true;
		pane = pane->parent;
	}
}

static bool ControlHitTest(Control *control, int x, int y) {
	if (!control) return false;

	OSRectangle bounds = GetControlBounds(control);

	if (control->window->pressedControl == control) {
#define EXTRA_PRESSED_BORDER (5)
		if (x >= bounds.left - EXTRA_PRESSED_BORDER && x < bounds.right + EXTRA_PRESSED_BORDER 
				&& y >= bounds.top - EXTRA_PRESSED_BORDER && y < bounds.bottom + EXTRA_PRESSED_BORDER) {
			return true;
		} else {
			return false;
		}
	} else {
		if (x >= bounds.left && x < bounds.right
				&& y >= bounds.top && y < bounds.bottom) {
			return true;
		} else {
			return false;
		}
	}
}

void OSDisableControl(OSObject object, bool disabled) {
	Control *control = (Control *) object;

	control->disabled = disabled;

	if (disabled) {
		if (control == control->window->hoverControl)   control->window->hoverControl = nullptr;
		if (control == control->window->pressedControl) control->window->pressedControl = nullptr;
	}

	OSInvalidateControl(control);
}

OSObject OSGetWindowContentPane(OSObject _window) {
	Window *window = (Window *) _window;
	return window->pane.grid;
}

OSObject OSGetPane(OSObject _pane, uintptr_t gridX, uintptr_t gridY) {
	Pane *pane = (Pane *) _pane;

	if (pane->gridWidth <= gridX || pane->gridHeight <= gridY) {
		OSCrashProcess(OS_FATAL_ERROR_INVALID_PANE_CHILD);
	}

	return pane->grid + gridX + gridY * pane->gridWidth;
}

void OSSetObjectCallback(OSObject object, OSObjectType objectType, OSCallbackType type, _OSCallback function, void *argument) {
	switch (objectType) {
		case OS_OBJECT_CONTROL: {
			Control *control = (Control *) object;

			switch (type) {
				case OS_CALLBACK_ACTION: {
					control->action.callback = function;
					control->action.argument = argument;
				} break;

				default: { goto unsupported; } break;
			}
		} break;

		default: {
			unsupported:;
			OSCrashProcess(OS_FATAL_ERROR_UNSUPPORTED_CALLBACK);
		} break;
	}
}

static void InitialisePane(Pane *pane, OSRectangle bounds, Window *window, Pane *parent, size_t gridWidth, size_t gridHeight, Control *control);

void OSConfigurePane(OSObject _pane, size_t gridWidth, size_t gridHeight, unsigned flags) {
	Pane *pane = (Pane *) _pane;
	pane->flags = flags;

	InitialisePane(pane, pane->bounds, pane->parent->window, pane->parent, gridWidth, gridHeight, nullptr);

	for (uintptr_t i = 0; i < gridWidth * gridHeight; i++) {
		InitialisePane(pane->grid + i, OSRectangle(0, 0, 0, 0), pane->parent->window, pane, 0, 0, nullptr);
	}
}

void OSSetPaneObject(OSObject _pane, OSObject object, unsigned flags) {
	Control *control = (Control *) object;
	
	Pane *pane = (Pane *) _pane;
	pane->flags = flags;

	InitialisePane(pane, pane->bounds, pane->parent->window, pane->parent, 0, 0, control);
}

static void MeasurePane(Pane *pane, unsigned &width, unsigned &height) {
	if (pane->control) {
		width = pane->control->preferredWidth;
		height = pane->control->preferredHeight;
		return;
	}

	unsigned columnWidths[pane->gridWidth];
	unsigned rowHeights[pane->gridHeight];

	OSZeroMemory(columnWidths, sizeof(columnWidths));
	OSZeroMemory(rowHeights, sizeof(rowHeights));

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		for (uintptr_t j = 0; j < pane->gridHeight; j++) {
			Pane *cell = (Pane *) OSGetPane(pane, i, j);
			unsigned width, height;
			MeasurePane(cell, width, height);

			if (columnWidths[i] < width) {
				columnWidths[i] = width;
			}

			if (rowHeights[j] < height) {
				rowHeights[j] = height;
			}
		}
	}

	width = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT) ? 0 : 4;
	height = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT) ? 0 : 4;

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		for (uintptr_t j = 0; j < pane->gridHeight; j++) {
			height += rowHeights[j] + 4;
		}

		width += columnWidths[i] + 4;
	}
}

static void LayoutPane(Pane *pane) {
	if (pane->control) {
		unsigned givenWidth = pane->bounds.right - pane->bounds.left;
		unsigned givenHeight = pane->bounds.bottom - pane->bounds.top;

		pane->control->bounds = pane->bounds;

		if (givenWidth > pane->control->preferredWidth) {
			if (pane->flags & OS_SET_PANE_OBJECT_HORIZONTAL_LEFT) {
				pane->control->bounds.right = pane->control->bounds.left + pane->control->preferredWidth;
			} else if (pane->flags & OS_SET_PANE_OBJECT_HORIZONTAL_RIGHT) {
				pane->control->bounds.left = pane->control->bounds.right - pane->control->preferredWidth;
			} else if (pane->flags & OS_SET_PANE_OBJECT_HORIZONTAL_CENTER) {
				pane->control->bounds.left = pane->control->bounds.left + givenWidth / 2 - pane->control->preferredWidth / 2;
				pane->control->bounds.right = pane->control->bounds.left + pane->control->preferredWidth;
			}
		}

		if (givenHeight > pane->control->preferredHeight) {
			if (pane->flags & OS_SET_PANE_OBJECT_VERTICAL_TOP) {
				pane->control->bounds.bottom = pane->control->bounds.top + pane->control->preferredHeight;
			} else if (pane->flags & OS_SET_PANE_OBJECT_VERTICAL_BOTTOM) {
				pane->control->bounds.top = pane->control->bounds.bottom - pane->control->preferredHeight;
			} else if (pane->flags & OS_SET_PANE_OBJECT_VERTICAL_CENTER) {
				pane->control->bounds.top = pane->control->bounds.top + givenHeight / 2 - pane->control->preferredHeight / 2;
				pane->control->bounds.bottom = pane->control->bounds.top + pane->control->preferredHeight;
			}
		}

		return;
	}

	unsigned columnWidths[pane->gridWidth];
	unsigned rowHeights[pane->gridHeight];

	OSZeroMemory(columnWidths, sizeof(columnWidths));
	OSZeroMemory(rowHeights, sizeof(rowHeights));

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		for (uintptr_t j = 0; j < pane->gridHeight; j++) {
			Pane *cell = (Pane *) OSGetPane(pane, i, j);
			unsigned width, height;
			MeasurePane(cell, width, height);

			if (columnWidths[i] < width) {
				columnWidths[i] = width;
			}

			if (rowHeights[j] < height) {
				rowHeights[j] = height;
			}
		}
	}

	unsigned positionX = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT) ? 0 : 4 + pane->bounds.left;

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		unsigned positionY = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT) ? 0 : 4 + pane->bounds.top;

		for (uintptr_t j = 0; j < pane->gridHeight; j++) {
			Pane *cell = (Pane *) OSGetPane(pane, i, j);
			cell->bounds = OSRectangle(positionX, positionX + columnWidths[i], positionY, positionY + rowHeights[j]);
			LayoutPane(cell);
			positionY += rowHeights[j] + 4;
		}

		positionX += columnWidths[i] + 4;
	}
}

static void InvalidatePane(Pane *pane) {
	if (pane->control) {
		pane->control->dirty = true;
	}

	pane->dirty = true;
	pane->window->dirty = true;

	for (uintptr_t i = 0; i < pane->gridWidth * pane->gridHeight; i++) {
		InvalidatePane(pane->grid + i);
	}
}

void OSLayoutPane(OSObject _pane) {
	Pane *pane = (Pane *) _pane;
	LayoutPane(pane);
	InvalidatePane(pane);
}

OSObject OSCreateControl(OSControlType type, char *text, size_t textLength, unsigned flags) {
	(void) flags;

	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	if (!control) return nullptr;

	control->type = type;
	control->textAlign = OS_DRAW_STRING_HALIGN_CENTER | OS_DRAW_STRING_VALIGN_CENTER;
	control->cursorStyle = OS_CURSOR_NORMAL;
	control->fontSize = FONT_SIZE;

	switch (type) {
		case OS_CONTROL_BUTTON: {
			control->preferredWidth = 80;
			control->preferredHeight = 21;
			control->imageType = OS_CONTROL_IMAGE_FILL;
			control->image = OSRectangle(42, 42 + 8, 88, 88 + 21);
		} break;

		case OS_CONTROL_STATIC: {
			control->preferredWidth = 4 + MeasureStringWidth(text, textLength, GetGUIFontScale(control->fontSize));
			control->preferredHeight = 14;
			control->imageType = OS_CONTROL_IMAGE_NONE;
		} break;

		case OS_CONTROL_TEXTBOX: {
			control->preferredWidth = 160;
			control->preferredHeight = 21;
			control->textBoundsBorder = 4;
			control->imageType = OS_CONTROL_IMAGE_FILL;
			control->image = OSRectangle(1, 1 + 7, 122, 122 + 21);
			control->canHaveFocus = true;
			control->textAlign = OS_DRAW_STRING_HALIGN_LEFT | OS_DRAW_STRING_VALIGN_CENTER;
			control->cursorStyle = OS_CURSOR_TEXT;
		} break;

		case OS_CONTROL_TITLEBAR: {
			control->imageType = OS_CONTROL_IMAGE_FILL;
			control->image = OSRectangle(149, 149 + 7, 64, 64 + 24);
			control->manualImage = true;
			control->textShadow = true;
			control->fontSize = 20;
		} break;

		case OS_CONTROL_WINDOW_BORDER: {
			control->imageType = OS_CONTROL_IMAGE_TRANSPARENT;
			control->manualImage = true;
		} break;
	}

	OSSetControlText(control, text, textLength);

	return control;
}

static void InitialisePane(Pane *pane, OSRectangle bounds, Window *window, Pane *parent, size_t gridWidth, size_t gridHeight, Control *control) {
	pane->bounds = bounds;
	pane->window = window;
	pane->parent = parent;
	pane->gridWidth = gridWidth;
	pane->gridHeight = gridHeight;
	pane->grid = (Pane *) OSHeapAllocate(sizeof(Pane) * gridWidth * gridHeight, true);
	pane->control = control;

	if (control) {
		control->pane = pane;
		control->window = window;
		control->bounds = bounds;
		OSInvalidateControl(control);
	}
}

OSObject OSCreateWindow(char *title, size_t titleLengthBytes, unsigned width, unsigned height, unsigned flags) {
	(void) flags;

	Window *window = (Window *) OSHeapAllocate(sizeof(Window), true);

	// Add the size of the border.
	width += BORDER_SIZE_X;
	height += BORDER_SIZE_Y;

	window->width = width;
	window->height = height;

	OSError result = OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) window, width, height, 0);

	if (result != OS_SUCCESS) {
		OSHeapFree(window);
		return nullptr;
	}

	// Create the root and content panes.
	InitialisePane(&window->pane, OSRectangle(0, width, 0, height), window, nullptr, 10, 1, nullptr);
	InitialisePane(window->pane.grid + 0, OSRectangle(4, width - 4, 30, height - 4), window, &window->pane, 0, 0, nullptr);
	
	{
		// Draw the window background and border.
		OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, width, 0, height), 
				OSRectangle(96, 105, 42, 77), OSRectangle(96 + 3, 96 + 5, 42 + 29, 42 + 31), OS_DRAW_MODE_REPEAT_FIRST);

		// TODO Increase the resize region sizes.

		Control *titlebar = (Control *) OSCreateControl(OS_CONTROL_TITLEBAR, title, titleLengthBytes, 0);
		InitialisePane(window->pane.grid + 1, OSRectangle(4, width - 4, 4, 28), window, &window->pane, 0, 0, titlebar);

		Control *resizeTop = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeTop->cursorStyle = OS_CURSOR_RESIZE_VERTICAL;
		resizeTop->resizeRegionIndex = 0;
		InitialisePane(window->pane.grid + 2, OSRectangle(4, width - 8, 0, 4), window, &window->pane, 0, 0, resizeTop);

		Control *resizeBottom = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeBottom->cursorStyle = OS_CURSOR_RESIZE_VERTICAL;
		resizeBottom->resizeRegionIndex = 1;
		InitialisePane(window->pane.grid + 3, OSRectangle(4, width - 8, height - 4, height), window, &window->pane, 0, 0, resizeBottom);

		Control *resizeLeft = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeLeft->cursorStyle = OS_CURSOR_RESIZE_HORIZONTAL;
		resizeLeft->resizeRegionIndex = 2;
		InitialisePane(window->pane.grid + 4, OSRectangle(0, 4, 4, height - 8), window, &window->pane, 0, 0, resizeLeft);

		Control *resizeRight = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeRight->cursorStyle = OS_CURSOR_RESIZE_HORIZONTAL;
		resizeRight->resizeRegionIndex = 3;
		InitialisePane(window->pane.grid + 5, OSRectangle(width - 4, width, 4, height - 8), window, &window->pane, 0, 0, resizeRight);

		Control *resizeTopLeft = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeTopLeft->cursorStyle = OS_CURSOR_RESIZE_DIAGONAL_2;
		resizeTopLeft->resizeRegionIndex = 4;
		InitialisePane(window->pane.grid + 6, OSRectangle(0, 4, 0, 4), window, &window->pane, 0, 0, resizeTopLeft);

		Control *resizeBottomRight = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeBottomRight->cursorStyle = OS_CURSOR_RESIZE_DIAGONAL_2;
		resizeBottomRight->resizeRegionIndex = 5;
		InitialisePane(window->pane.grid + 7, OSRectangle(width - 4, width, height - 4, height), window, &window->pane, 0, 0, resizeBottomRight);

		Control *resizeTopRight = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeTopRight->cursorStyle = OS_CURSOR_RESIZE_DIAGONAL_1;
		resizeTopRight->resizeRegionIndex = 6;
		InitialisePane(window->pane.grid + 8, OSRectangle(width - 4, width, 0, 4), window, &window->pane, 0, 0, resizeTopRight);

		Control *resizeBottomLeft = (Control *) OSCreateControl(OS_CONTROL_WINDOW_BORDER, nullptr, 0, 0);
		resizeBottomLeft->cursorStyle = OS_CURSOR_RESIZE_DIAGONAL_1;
		resizeBottomLeft->resizeRegionIndex = 7;
		InitialisePane(window->pane.grid + 9, OSRectangle(0, 4, height - 4, height), window, &window->pane, 0, 0, resizeBottomLeft);
	}

	OSUpdateWindow(window);

	return window;
}

void RemoveSelectedText(Control *control) {
	OSCallbackData callback = {};
	callback.type = OS_CALLBACK_REMOVE_TEXT;

	if (control->caret.byte < control->caret2.byte) {
		callback.removeText.caretStart = &control->caret;
		callback.removeText.caretEnd = &control->caret2;
	} else {
		callback.removeText.caretStart = &control->caret2;
		callback.removeText.caretEnd = &control->caret;
	}

	SendCallback(control, control->removeText, callback);

	if (control->caret.byte < control->caret2.byte) {
		control->caret2 = control->caret;
	} else {
		control->caret = control->caret2;
	}
}

enum CharacterType {
	CHARACTER_INVALID,
	CHARACTER_IDENTIFIER, // A-Z, a-z, 0-9, _, >= 0x7F
	CHARACTER_WHITESPACE, // space, tab, newline
	CHARACTER_OTHER,
};

CharacterType GetCharacterType(int character) {
	if ((character >= '0' && character <= '9') 
			|| (character >= 'a' && character <= 'z')
			|| (character >= 'A' && character <= 'Z')
			|| (character == '_')
			|| (character >= 0x80)) {
		return CHARACTER_IDENTIFIER;
	}

	if (character == '\n' || character == '\t' || character == ' ') {
		return CHARACTER_WHITESPACE;
	}

	return CHARACTER_OTHER;
}

void MoveCaret(OSString *string, OSCaret *caret, bool right, bool word, bool strongWhitespace = false) {
	CharacterType type = CHARACTER_INVALID;

	if (word && right) goto checkCharacterType;

	while (true) {

		if (!right) {
			if (caret->character) {
				caret->character--;
				caret->byte = utf8_retreat(string->buffer + caret->byte) - string->buffer;
			} else {
				return; // We cannot move any further left.
			}
		} else {
			if (caret->character != string->characters) {
				caret->character++;
				caret->byte = utf8_advance(string->buffer + caret->byte) - string->buffer;
			} else {
				return; // We cannot move any further right.
			}
		}

		if (!word) {
			return;
		}

		checkCharacterType:;
		CharacterType newType = GetCharacterType(utf8_value(string->buffer + caret->byte));

		if (type == CHARACTER_INVALID) {
			if (newType != CHARACTER_WHITESPACE || strongWhitespace) {
				type = newType;
			}
		} else {
			if (newType != type) {
				if (!right) {
					// We've gone too far.
					MoveCaret(string, caret, true, false);
				}

				break;
			}
		}
	}
}

static void FindCaret(Control *control, int positionX, int positionY, bool secondCaret, unsigned clickChainCount) {
	if (!clickChainCount) {
		Panic();
	}

	if (control->type == OS_CONTROL_TEXTBOX && !control->disabled) {
		OSCallbackData textEvent = {};
		OSString string;
		textEvent.getText.string = &string;
		textEvent.type = OS_CALLBACK_GET_TEXT;
		SendCallback(control, control->getText, textEvent);

		if (clickChainCount >= 3) {
			control->caret.byte = 0;
			control->caret.character = 0;
			control->caret2.byte = string.bytes;
			control->caret2.character = string.characters;
		} else {
			OSRectangle textBounds = GetControlTextBounds(control);
			OSFindCharacterAtCoordinate(textBounds, OSPoint(positionX, positionY), &string, control->textAlign, &control->caret2);

			if (!secondCaret) {
				control->caret = control->caret2;

				if (clickChainCount == 2) {
					MoveCaret(&string, &control->caret, false, true, true);
					MoveCaret(&string, &control->caret2, true, true, true);
					control->wordSelectionAnchor  = control->caret;
					control->wordSelectionAnchor2 = control->caret2;
				}
			} else {
				if (clickChainCount == 2) {
					if (control->caret2.byte < control->caret.byte) {
						MoveCaret(&string, &control->caret2, false, true);
						control->caret = control->wordSelectionAnchor2;
					} else {
						MoveCaret(&string, &control->caret2, true, true);
						control->caret = control->wordSelectionAnchor;
					}
				}
			}
		}

		control->caretBlink = 0;

		if (textEvent.getText.freeText) {
			OSHeapFree(string.buffer);
		}
	}
}

static unsigned lastClickChainCount;
static int lastClickX, lastClickY;
static int lastClickWindowWidth, lastClickWindowHeight;

static Control *FindControl(Pane *pane, int x, int y) {
	OSRectangle bounds = GetPaneBounds(pane);

	if (bounds.left > x || bounds.top > y
			|| bounds.right <= x || bounds.bottom <= y) {
		return nullptr;
	}

	for (uintptr_t i = 0; i < pane->gridWidth * pane->gridHeight; i++) {
		Control *control = FindControl(pane->grid + i, x, y);

		if (control) {
			return control;
		}
	}

	return ControlHitTest(pane->control, x, y) ? pane->control : nullptr;
}

static void UpdateMousePosition(Window *window, int x, int y, int sx, int sy) {
	Control *previousHoverControl = window->hoverControl;
	window->previousHoverControl = previousHoverControl;

	if (previousHoverControl) {
		if (!ControlHitTest(previousHoverControl, x, y)) {
			window->hoverControl = nullptr;
			OSInvalidateControl(previousHoverControl);
		}
	}

	if (!window->hoverControl) {
		Control *control = FindControl(&window->pane, x, y);

		if (control) {
			window->hoverControl = control;
			OSInvalidateControl(window->hoverControl);

			if (!window->pressedControl) {
				OSSetCursorStyle(window->handle, control->cursorStyle);
			}
		}
	}

	if (!window->hoverControl && !window->pressedControl) {
		OSSetCursorStyle(window->handle, OS_CURSOR_NORMAL);
	}

	if (window->pressedControl) {
		Control *control = window->pressedControl;

		if (control->type == OS_CONTROL_TEXTBOX) {
			if (!lastClickChainCount) return;
			FindCaret(control, x, y, true, lastClickChainCount);
			OSInvalidateControl(control);
		} else if (control->type == OS_CONTROL_TITLEBAR) {
			OSRectangle bounds;
			OSGetWindowBounds(window->handle, &bounds);
			int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
			bounds.left = sx - lastClickX;
			bounds.right = sx - lastClickX + width;
			bounds.top = sy - lastClickY;
			bounds.bottom = sy - lastClickY + height;
			OSMoveWindow(window->handle, bounds);
			window->dirty = true;
		} else if (control->type == OS_CONTROL_WINDOW_BORDER) {
			OSRectangle bounds;
			OSGetWindowBounds(window->handle, &bounds);

			bool la = false, ta = false;

			switch (control->resizeRegionIndex) {
				case 0: {
					bounds.top = sy - lastClickY;
					ta = true;
				} break;

				case 1: {
					bounds.bottom = sy - lastClickY + lastClickWindowHeight;
				} break;

				case 2: {
					bounds.left = sx - lastClickX;
					la = true;
				} break;

				case 3: {
					bounds.right = sx - lastClickX + lastClickWindowWidth;
				} break;

				case 4: {
					bounds.top = sy - lastClickY;
					bounds.left = sx - lastClickX;
					ta = true;
					la = true;
				} break;

				case 5: {
					bounds.bottom = sy - lastClickY + lastClickWindowHeight;
					bounds.right = sx - lastClickX + lastClickWindowWidth;
				} break;

				case 6: {
					bounds.top = sy - lastClickY;
					bounds.right = sx - lastClickX + lastClickWindowWidth;
					ta = true;
				} break;

				case 7: {
					bounds.bottom = sy - lastClickY + lastClickWindowHeight;
					bounds.left = sx - lastClickX;
					la = true;
				} break;
			}

			int width = bounds.right - bounds.left;
			int height = bounds.bottom - bounds.top;

			// TODO Window minimum size.

			if (width < 256) {
				width = 256;
				if (!la) bounds.right = bounds.left + 256;
				else bounds.left = bounds.right - 256;
			}

			if (height < 256) {
				height = 256;
				if (!ta) bounds.bottom = bounds.top + 256;
				else bounds.top = bounds.bottom - 256;
			}

			window->width = width;
			window->height = height;

			window->pane.grid[0].bounds = OSRectangle(4, width - 4, 30, height - 4);
			window->pane.grid[1].bounds = OSRectangle(4, width - 4, 4, 28);
			window->pane.grid[2].bounds = OSRectangle(4, width - 4, 0, 4);
			window->pane.grid[3].bounds = OSRectangle(4, width - 4, height - 4, height);
			window->pane.grid[4].bounds = OSRectangle(0, 4, 4, height - 4);
			window->pane.grid[5].bounds = OSRectangle(width - 4, width, 4, height - 4);
			window->pane.grid[6].bounds = OSRectangle(0, 4, 0, 4);
			window->pane.grid[7].bounds = OSRectangle(width - 4, width, height - 4, height);
			window->pane.grid[8].bounds = OSRectangle(width - 4, width, 0, 4);
			window->pane.grid[9].bounds = OSRectangle(0, 4, height - 4, height);

			for (uintptr_t i = 1; i < 10; i++) window->pane.grid[i].control->bounds = window->pane.grid[i].bounds;

			OSMoveWindow(window->handle, bounds);

			// Redraw the window background and border.
			OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, window->width, 0, window->height), 
					OSRectangle(96, 105, 42, 77), OSRectangle(96 + 3, 96 + 5, 42 + 29, 42 + 31), OS_DRAW_MODE_REPEAT_FIRST);

			// Layout the window.
			OSLayoutPane(window->pane.grid);

			// Redraw the window.
			DrawPane(&window->pane, true);
		}
	}
}

static void ProcessTextboxInput(OSMessage *message, Control *control) {
	int ic = -1,
	    isc = -1;

	if (control && !control->disabled && control->type == OS_CONTROL_TEXTBOX) {
		control->caretBlink = 0;

		switch (message->keyboard.scancode) {
			case OS_SCANCODE_A: ic = 'a'; isc = 'A'; break;
			case OS_SCANCODE_B: ic = 'b'; isc = 'B'; break;
			case OS_SCANCODE_C: ic = 'c'; isc = 'C'; break;
			case OS_SCANCODE_D: ic = 'd'; isc = 'D'; break;
			case OS_SCANCODE_E: ic = 'e'; isc = 'E'; break;
			case OS_SCANCODE_F: ic = 'f'; isc = 'F'; break;
			case OS_SCANCODE_G: ic = 'g'; isc = 'G'; break;
			case OS_SCANCODE_H: ic = 'h'; isc = 'H'; break;
			case OS_SCANCODE_I: ic = 'i'; isc = 'I'; break;
			case OS_SCANCODE_J: ic = 'j'; isc = 'J'; break;
			case OS_SCANCODE_K: ic = 'k'; isc = 'K'; break;
			case OS_SCANCODE_L: ic = 'l'; isc = 'L'; break;
			case OS_SCANCODE_M: ic = 'm'; isc = 'M'; break;
			case OS_SCANCODE_N: ic = 'n'; isc = 'N'; break;
			case OS_SCANCODE_O: ic = 'o'; isc = 'O'; break;
			case OS_SCANCODE_P: ic = 'p'; isc = 'P'; break;
			case OS_SCANCODE_Q: ic = 'q'; isc = 'Q'; break;
			case OS_SCANCODE_R: ic = 'r'; isc = 'R'; break;
			case OS_SCANCODE_S: ic = 's'; isc = 'S'; break;
			case OS_SCANCODE_T: ic = 't'; isc = 'T'; break;
			case OS_SCANCODE_U: ic = 'u'; isc = 'U'; break;
			case OS_SCANCODE_V: ic = 'v'; isc = 'V'; break;
			case OS_SCANCODE_W: ic = 'w'; isc = 'W'; break;
			case OS_SCANCODE_X: ic = 'x'; isc = 'X'; break;
			case OS_SCANCODE_Y: ic = 'y'; isc = 'Y'; break;
			case OS_SCANCODE_Z: ic = 'z'; isc = 'Z'; break;
			case OS_SCANCODE_0: ic = '0'; isc = ')'; break;
			case OS_SCANCODE_1: ic = '1'; isc = '!'; break;
			case OS_SCANCODE_2: ic = '2'; isc = '@'; break;
			case OS_SCANCODE_3: ic = '3'; isc = '#'; break;
			case OS_SCANCODE_4: ic = '4'; isc = '$'; break;
			case OS_SCANCODE_5: ic = '5'; isc = '%'; break;
			case OS_SCANCODE_6: ic = '6'; isc = '^'; break;
			case OS_SCANCODE_7: ic = '7'; isc = '&'; break;
			case OS_SCANCODE_8: ic = '8'; isc = '*'; break;
			case OS_SCANCODE_9: ic = '9'; isc = '('; break;
			case OS_SCANCODE_SLASH: 	ic = '/';  isc = '?'; break;
			case OS_SCANCODE_BACKSLASH: 	ic = '\\'; isc = '|'; break;
			case OS_SCANCODE_LEFT_BRACE: 	ic = '[';  isc = '{'; break;
			case OS_SCANCODE_RIGHT_BRACE: 	ic = ']';  isc = '}'; break;
			case OS_SCANCODE_EQUALS: 	ic = '=';  isc = '+'; break;
			case OS_SCANCODE_BACKTICK: 	ic = '`';  isc = '~'; break;
			case OS_SCANCODE_HYPHEN: 	ic = '-';  isc = '_'; break;
			case OS_SCANCODE_SEMICOLON: 	ic = ';';  isc = ':'; break;
			case OS_SCANCODE_QUOTE: 	ic = '\''; isc = '"'; break;
			case OS_SCANCODE_COMMA: 	ic = ',';  isc = '<'; break;
			case OS_SCANCODE_PERIOD: 	ic = '.';  isc = '>'; break;
			case OS_SCANCODE_SPACE:		ic = ' ';  isc = ' '; break;

			case OS_SCANCODE_BACKSPACE: {
				if (control->caret.byte == control->caret2.byte && control->caret.byte) {
					MoveCaret(&control->text, &control->caret2, false, message->keyboard.ctrl);

					OSCallbackData callback = {};
					callback.type = OS_CALLBACK_REMOVE_TEXT;
					callback.removeText.caretStart = &control->caret2;
					callback.removeText.caretEnd = &control->caret;

					SendCallback(control, control->removeText, callback);

					control->caret = control->caret2;
				} else {
					RemoveSelectedText(control);
				}
			} break;

			case OS_SCANCODE_DELETE: {
				if (control->caret.byte == control->caret2.byte && control->caret.byte != control->text.bytes) {
					MoveCaret(&control->text, &control->caret2, true, message->keyboard.ctrl);

					OSCallbackData callback = {};
					callback.type = OS_CALLBACK_REMOVE_TEXT;
					callback.removeText.caretStart = &control->caret;
					callback.removeText.caretEnd = &control->caret2;

					SendCallback(control, control->removeText, callback);

					control->caret2 = control->caret;
				} else {
					RemoveSelectedText(control);
				}
			} break;

			case OS_SCANCODE_LEFT_ARROW: {
				if (message->keyboard.shift) {
					MoveCaret(&control->text, &control->caret2, false, message->keyboard.ctrl);
				} else {
					bool move = control->caret2.byte == control->caret.byte;
					if (control->caret2.byte < control->caret.byte) control->caret = control->caret2;
					else control->caret2 = control->caret;

					if (move) {
						MoveCaret(&control->text, &control->caret2, false, message->keyboard.ctrl);
					}

					control->caret = control->caret2;
				}
			} break;

			case OS_SCANCODE_RIGHT_ARROW: {
				if (message->keyboard.shift) {
					MoveCaret(&control->text, &control->caret2, true, message->keyboard.ctrl);
				} else {
					bool move = control->caret2.byte == control->caret.byte;
					if (control->caret2.byte > control->caret.byte) control->caret = control->caret2;
					else control->caret2 = control->caret;

					if (move) {
						MoveCaret(&control->text, &control->caret2, true, message->keyboard.ctrl);
					}

					control->caret = control->caret2;
				}
			} break;

			case OS_SCANCODE_HOME: {
				control->caret2.byte = 0;
				control->caret2.character = 0;
				if (!message->keyboard.shift) control->caret = control->caret2;
			} break;

			case OS_SCANCODE_END: {
				control->caret2.byte = control->text.bytes;
				control->caret2.character = control->text.characters;
				if (!message->keyboard.shift) control->caret = control->caret2;
			} break;
		}

		if (message->keyboard.scancode == OS_SCANCODE_A && message->keyboard.ctrl && !message->keyboard.alt) {
			control->caret.byte = 0;
			control->caret.character = 0;

			control->caret2.byte = control->text.bytes;
			control->caret2.character = control->text.characters;
		}

		if (ic != -1 && !message->keyboard.alt && !message->keyboard.ctrl) {
			RemoveSelectedText(control);

			{
				char data[4];
				int bytes = utf8_encode(message->keyboard.shift ? isc : ic, data);

				OSString insert = {};
				insert.buffer = data;
				insert.bytes = bytes;
				insert.characters = 1;

				// Insert the pressed character.
				OSCallbackData callback = {};
				callback.type = OS_CALLBACK_INSERT_TEXT;
				callback.insertText.string = &insert;
				callback.insertText.caret = &control->caret;
				SendCallback(control, control->insertText, callback);
			}

			{
				// Update the caret and redraw the control.
				control->caret.character++;
				control->caret.byte = utf8_advance(control->text.buffer + control->caret.byte) - control->text.buffer;
				control->caret2 = control->caret;
			}
		}

		OSInvalidateControl(control);
	}
}

OSError OSProcessGUIMessage(OSMessage *message) {
	// TODO Message security. 
	// 	How should we tell who sent the message?
	// 	(and that they gave us a valid window?)

	Window *window = (Window *) message->targetWindow;

	switch (message->type) {
		case OS_MESSAGE_MOUSE_MOVED: {
			UpdateMousePosition(window, 
					message->mouseMoved.newPositionX, 
					message->mouseMoved.newPositionY,
					message->mouseMoved.newPositionXScreen,
					message->mouseMoved.newPositionYScreen);
		} break;

		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			lastClickX = message->mousePressed.positionX;
			lastClickY = message->mousePressed.positionY;
			lastClickWindowWidth = window->width;
			lastClickWindowHeight = window->height;

			if (window->hoverControl) {
				Control *control = window->hoverControl;

				if (control->canHaveFocus) {
					window->focusedControl = control; // TODO Lose when the window is deactivated.
					FindCaret(control, message->mousePressed.positionX, message->mousePressed.positionY, false, (lastClickChainCount = message->mousePressed.clickChainCount));
				}

				window->pressedControl = control;
				OSInvalidateControl(control);
			}
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			if (window->pressedControl) {
				Control *previousPressedControl = window->pressedControl;
				window->pressedControl = nullptr;

				OSInvalidateControl(previousPressedControl);

				if (window->hoverControl == previousPressedControl
						/* || (!window->hoverControl && window->previousHoverControl == previousPressedControl) */) { 
					OSCallbackData data = {};
					data.type = OS_CALLBACK_ACTION;
					SendCallback(previousPressedControl, previousPressedControl->action, data);
				}

				UpdateMousePosition(window, 
						message->mousePressed.positionX, 
						message->mousePressed.positionY,
						message->mousePressed.positionXScreen,
						message->mousePressed.positionYScreen);
			}
		} break;

		case OS_MESSAGE_WINDOW_CREATED: {
			window->dirty = true;
			InvalidatePane(&window->pane);
		} break;

		case OS_MESSAGE_WINDOW_BLINK_TIMER: {
			if (window->focusedControl) {
				window->focusedControl->caretBlink++;

				if (window->focusedControl->caretBlink == 3) {
					window->focusedControl->caretBlink = 1;
				}

				OSInvalidateControl(window->focusedControl);
			}
		} break;

		case OS_MESSAGE_KEY_PRESSED: {
			Control *control = window->focusedControl;
			ProcessTextboxInput(message, control);
		} break;

		case OS_MESSAGE_KEY_RELEASED: {
		} break;

		default: {
			return OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI;
		}
	}

	if (window->dirty) {
		DrawPane(&window->pane, false);
		OSUpdateWindow(window);
		window->dirty = false;
	}

	return OS_SUCCESS;
}
