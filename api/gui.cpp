// TODO Textboxes
//	- Clipboard and undo
//	- Scrolling (including drag selections)
//	- Actually use the OS_CALLBACK_GET_TEXT callback for everything.
//	- Heap corruption?

// TODO Menus
//	- Arrow keys
//	- Alt sequences
//	- Checkboxes and radioboxes
//	- Crash?

// TODO Replace Panic() with OSCrashProcess().

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
	OSCallback populateMenu;

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
	union {
		struct { OSCaret wordSelectionAnchor, wordSelectionAnchor2; };
		uint8_t resizeRegionIndex;
		struct {bool menuHasChildren : 1, menuBar : 1; };
	};

	// State:

	OSCaret caret, caret2;
	OSString text;
};

struct Pane {
	struct Window *window;
	struct Pane *parent;
	Control *control;
	OSRectangle bounds, image, imageBorder;
	unsigned preferredWidth, preferredHeight;
	OSCallback layout, measure;

	struct Pane *grid;
	size_t gridWidth, gridHeight;

	unsigned flags;
	uint8_t dirty; // 1 = controls dirty, 2 = background dirty
};

struct Window {
	OSHandle handle;
	OSHandle surface;

	size_t width, height;

	Pane pane;

	Control *hoverControl, *previousHoverControl;
	Control *pressedControl;
	Control *focusedControl;

	Window *popupParent;					// The parent window to this popup.
	Control *popupMenuControl, *lastPopupMenuControl;	// The control that created the child popup window.
	Window *popupChild;					// The current child popup window.

	bool dirty;
	unsigned flags;
};

#define BORDER_OFFSET_X (5)
#define BORDER_OFFSET_Y (29)

#define BORDER_SIZE_X (8)
#define BORDER_SIZE_Y (34)

static void CreatePopupMenu(Window *parent, OSObject generator, Window *existingWindow);
static void DeactivateWindow(Window *window, Window *newWindow);

static bool SendCallback(OSObject _from, OSCallback &callback, OSCallbackData &data) {
	if (callback.callback) {
		callback.callback(_from, callback.argument, &data);
	} else {
		Control *from = (Control *) _from;

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

			case OS_CALLBACK_MEASURE_PANE:
			case OS_CALLBACK_LAYOUT_PANE: {
				return false;
			} break;

			default: {
				OSCrashProcess(OS_FATAL_ERROR_MISSING_CALLBACK);
			} break;
		}
	}

	return true;
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

	bool isHoverControl = (control == window->hoverControl) || (control == window->focusedControl) || (control == window->popupMenuControl);
	bool isPressedControl = (control == window->pressedControl) || (control == window->focusedControl) || (control == window->popupMenuControl);

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

	{
		// Draw the background of the parent pane.
		Pane *pane = control->pane;
		while (pane && !pane->image.left) pane = pane->parent;
		if (pane) OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, control->pane->bounds, pane->image, pane->imageBorder, OS_DRAW_MODE_REPEAT_FIRST);
	}

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
				control->disabled ? 0x808080 : 0x000000, -1);
	} else if (control->imageType == OS_CONTROL_IMAGE_NONE) {
		OSDrawString(window->surface, textBounds, 
				&controlText,
				control->textAlign,
				control->disabled ? 0x808080 : 0x000000, -1);
	}

	if (textEvent.getText.freeText) {
		OSHeapFree(controlText.buffer);
	}

	if (control->type == OS_CONTROL_MENU && !control->menuBar && control->menuHasChildren) {
		int center = (bounds.bottom - bounds.top) / 2 + bounds.top;
		OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
				OSRectangle(bounds.right - 9, bounds.right - 4, 
					center - 4, center + 3),
				OSRectangle(86, 86 + 5, 94, 94 + 7),
				OSRectangle(86 + 1, 86 + 2, 94 + 2, 94 + 3),
				OS_DRAW_MODE_REPEAT_FIRST);
	}
}

static void DrawPane(Pane *pane, bool force) {
	bool drawBackground = true;

	if (!pane || !pane->window) {
		OSCrashProcess(OS_FATAL_ERROR_INVALID_PANE_OBJECT);
	}

	if (pane->dirty == false && !force) {
		return;
	} else {
		drawBackground = pane->dirty == 2 || force;
		pane->dirty = false;
	}

	if (drawBackground) {
		// Draw the background of the pane.
		OSDrawSurface(pane->window->surface, OS_SURFACE_UI_SHEET,
				pane->bounds, pane->image, pane->imageBorder, OS_DRAW_MODE_REPEAT_FIRST);
	}

	if (pane->control && (pane->control->dirty || force)) {
		DrawControl(pane->control);
		pane->control->dirty = false;
	}

	for (uintptr_t i = 0; i < pane->gridWidth * pane->gridHeight; i++) {
		if (pane->grid[i].window) {
			DrawPane(pane->grid + i, force);
		}
	}
}

void OSSetControlText(OSObject _control, char *text, size_t textLength) {
	Control *control = (Control *) _control;

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
	if (!_control) return;

	Control *control = (Control *) _control;
	control->dirty = true;
	if (!control->window) return;

	Pane *pane = control->pane;
	while (pane) {
		if (!pane->dirty) pane->dirty = true;
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

OSObject OSGetWindowMenuBar(OSObject _window) {
	Window *window = (Window *) _window;

	if (window->flags & OS_CREATE_WINDOW_WITH_MENU_BAR) {
		return window->pane.grid + 10;
	} else {
		return nullptr;
	}
}

OSObject OSGetPane(OSObject _pane, uintptr_t gridX, uintptr_t gridY) {
	Pane *pane = (Pane *) _pane;

	if (pane->gridWidth <= gridX || pane->gridHeight <= gridY) {
		OSCrashProcess(OS_FATAL_ERROR_INVALID_PANE_CHILD);
	}

	return pane->grid + gridX + gridY * pane->gridWidth;
}

OSObject OSGetControl(OSObject _pane, uintptr_t gridX, uintptr_t gridY) {
	Pane *pane = (Pane *) _pane;

	if (pane->gridWidth <= gridX || pane->gridHeight <= gridY) {
		OSCrashProcess(OS_FATAL_ERROR_INVALID_PANE_CHILD);
	}

	return (pane->grid + gridX + gridY * pane->gridWidth)->control;
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

				case OS_CALLBACK_POPULATE_MENU: {
					control->populateMenu.callback = function;
					control->populateMenu.argument = argument;
				} break;

				default: { goto unsupported; } break;
			}
		} break;

		case OS_OBJECT_PANE: {
			Pane *pane = (Pane *) object;

			switch (type) {
				case OS_CALLBACK_LAYOUT_PANE: {
					pane->layout.callback = function;
					pane->layout.argument = argument;
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

	if (!pane->image.left) {
		// Initialise the pane's background.
		pane->image = OSRectangle(84, 87, 106, 109);
		pane->imageBorder = OSRectangle(85, 86, 107, 108);
	}

	InitialisePane(pane, pane->bounds, pane->window, pane->parent, gridWidth, gridHeight, pane->control);

	if (pane->parent) {
		pane->window = pane->parent->window;
	}

	for (uintptr_t i = 0; i < gridWidth * gridHeight; i++) {
		InitialisePane(pane->grid + i, OSRectangle(0, 0, 0, 0), pane->window, pane, 0, 0, nullptr);
	}
}

void OSSetPaneObject(OSObject _pane, OSObject object, unsigned flags) {
	Control *control = (Control *) object;
	
	Pane *pane = (Pane *) _pane;
	pane->flags = flags;

	InitialisePane(pane, pane->bounds, pane->window, pane->parent, 0, 0, control);
}

#define PUSH (-1)

static void MeasurePane(Pane *pane, int &width, int &height) {
	OSCallbackData callback = {};
	callback.type = OS_CALLBACK_MEASURE_PANE;
	callback.measure.pane = pane;
	if (SendCallback(pane, pane->measure, callback)) return;

	if (pane->control) {
		width = pane->control->preferredWidth;
		height = pane->control->preferredHeight;

		if (pane->flags & OS_SET_PANE_OBJECT_HORIZONTAL_PUSH) {
			width = PUSH;
		}

		if (pane->flags & OS_SET_PANE_OBJECT_VERTICAL_PUSH) {
			height = PUSH;
		}
	}

	if (!pane->gridWidth || !pane->gridHeight) {
		// No children in the grid to layout.
		return;
	}

	int columnWidths[pane->gridWidth];
	int rowHeights[pane->gridHeight];

	OSZeroMemory(columnWidths, sizeof(columnWidths));
	OSZeroMemory(rowHeights, sizeof(rowHeights));

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		for (uintptr_t j = 0; j < pane->gridHeight; j++) {
			Pane *cell = (Pane *) OSGetPane(pane, i, j);
			int width, height;
			MeasurePane(cell, width, height);

			if ((columnWidths[i] < width && columnWidths[i] != PUSH) || width == PUSH) {
				columnWidths[i] = width;
			}

			if ((rowHeights[j] < height && rowHeights[j] != PUSH) || height == PUSH) {
				rowHeights[j] = height;
			}
		}
	}

	width = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT_H) ? 0 : 4;
	height = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT_V) ? 0 : 4;

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		width += columnWidths[i] + ((pane->flags & OS_CONFIGURE_PANE_NO_SPACE_H) ? 0 : 4);

		if (columnWidths[i] == PUSH) {
			width = PUSH;
			break;
		}
	}

	for (uintptr_t j = 0; j < pane->gridHeight; j++) {
		height += rowHeights[j] + ((pane->flags & OS_CONFIGURE_PANE_NO_SPACE_V) ? 0 : 4);

		if (rowHeights[j] == PUSH) {
			height = PUSH;
			break;
		}
	}

	if (pane->flags & OS_CONFIGURE_PANE_NO_SPACE_H) {
		width += 4;
	} else if (pane->flags & OS_CONFIGURE_PANE_NO_INDENT_H) {
		if (width) width -= 4;
	}

	if (pane->flags & OS_CONFIGURE_PANE_NO_SPACE_V) {
		height += 4;
	} else if (pane->flags & OS_CONFIGURE_PANE_NO_INDENT_V) {
		if (height) height -= 4;
	}
}

static void LayoutPane(Pane *pane) {
	OSCallbackData callback = {};
	callback.type = OS_CALLBACK_LAYOUT_PANE;
	callback.layout.pane = pane;
	callback.layout.bounds = pane->bounds;
	if (SendCallback(pane, pane->layout, callback)) return;

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
	}

	if (!pane->gridWidth || !pane->gridHeight) {
		// No children in the grid to layout.
		return;
	}

	int columnWidths[pane->gridWidth];
	int rowHeights[pane->gridHeight];

	int pushColumnCount = 0;
	int pushRowCount = 0;

	OSZeroMemory(columnWidths, sizeof(columnWidths));
	OSZeroMemory(rowHeights, sizeof(rowHeights));

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		for (uintptr_t j = 0; j < pane->gridHeight; j++) {
			Pane *cell = (Pane *) OSGetPane(pane, i, j);
			int width, height;
			MeasurePane(cell, width, height);

			if (columnWidths[i] != PUSH && (columnWidths[i] < width || width == PUSH)) {
				columnWidths[i] = width;
				if (width == PUSH) pushColumnCount++;
			}

			if (rowHeights[j] != PUSH && (rowHeights[j] < height || height == PUSH)) {
				rowHeights[j] = height;
				if (height == PUSH) pushRowCount++;
			}
		}
	}

	if (pushColumnCount) {
		int usedWidth = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT_H) ? 0 : 4;

		for (uintptr_t i = 0; i < pane->gridWidth; i++) {
			if (columnWidths[i] != PUSH) {
				usedWidth += columnWidths[i] + ((pane->flags & OS_CONFIGURE_PANE_NO_SPACE_H) ? 0 : 4);
			}
		}

		int widthPerPush = (pane->bounds.right - pane->bounds.left - usedWidth) / pushColumnCount - ((pane->flags & OS_CONFIGURE_PANE_NO_INDENT_H) ? 0 : 4);

		for (uintptr_t i = 0; i < pane->gridWidth; i++) {
			if (columnWidths[i] == PUSH) {
				columnWidths[i] = widthPerPush;
			}
		}
	}

	if (pushRowCount) {
		int usedHeight = (pane->flags & OS_CONFIGURE_PANE_NO_INDENT_V) ? 0 : 4;

		for (uintptr_t i = 0; i < pane->gridHeight; i++) {
			if (rowHeights[i] != PUSH) {
				usedHeight += rowHeights[i] + ((pane->flags & OS_CONFIGURE_PANE_NO_SPACE_V) ? 0 : 4);
			}
		}

		int heightPerPush = (pane->bounds.bottom - pane->bounds.top - usedHeight) / pushRowCount - ((pane->flags & OS_CONFIGURE_PANE_NO_INDENT_V) ? 0 : 4);

		for (uintptr_t i = 0; i < pane->gridHeight; i++) {
			if (rowHeights[i] == PUSH) {
				rowHeights[i] = heightPerPush;
			}
		}
	}

	unsigned positionX = ((pane->flags & OS_CONFIGURE_PANE_NO_INDENT_H) ? 0 : 4) + pane->bounds.left;

	for (uintptr_t i = 0; i < pane->gridWidth; i++) {
		unsigned positionY = ((pane->flags & OS_CONFIGURE_PANE_NO_INDENT_V) ? 0 : 4) + pane->bounds.top;

		for (uintptr_t j = 0; j < pane->gridHeight; j++) {
			Pane *cell = (Pane *) OSGetPane(pane, i, j);
			cell->bounds = OSRectangle(positionX, positionX + columnWidths[i], positionY, positionY + rowHeights[j]);
			LayoutPane(cell);
			positionY += rowHeights[j] + ((pane->flags & OS_CONFIGURE_PANE_NO_SPACE_V) ? 0 : 4);
		}

		positionX += columnWidths[i] + ((pane->flags & OS_CONFIGURE_PANE_NO_SPACE_H) ? 0 : 4);
	}

	pane->dirty = 2;
}

static void InvalidatePane(Pane *pane) {
	if (!pane->window) return;

	if (pane->control) {
		pane->control->dirty = true;
	}

	if (!pane->dirty) pane->dirty = true;
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

		case OS_CONTROL_MENU: {
			control->preferredWidth = 14 + MeasureStringWidth(text, textLength, GetGUIFontScale(control->fontSize));
			control->preferredHeight = 20;
			control->imageType = OS_CONTROL_IMAGE_FILL;
			control->menuHasChildren = flags & OS_CONTROL_MENU_HAS_CHILDREN;
			control->menuBar = flags & OS_CONTROL_MENU_STYLE_BAR;

			if (flags & OS_CONTROL_MENU_STYLE_BAR) {
				control->image = OSRectangle(42, 42 + 8, 124, 124 + 17);
			} else {
				control->image = OSRectangle(42, 42 + 8, 142, 142 + 17);
			}
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

void OSSetMenuBarMenus(OSObject menuBar, size_t count) {
	OSConfigurePane(menuBar, count, 1, OS_CONFIGURE_PANE_NO_INDENT_V | OS_CONFIGURE_PANE_NO_SPACE_H);
}

void OSSetMenuBarMenu(OSObject menuBar, uintptr_t index, OSObject menu) {
	OSSetPaneObject(OSGetPane(menuBar, index, 0), menu, OS_SET_PANE_OBJECT_VERTICAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_CENTER);
}

void OSSetMenuItems(OSObject menu, size_t count) {
	OSConfigurePane(menu, 1, count, OS_CONFIGURE_PANE_NO_SPACE_V | OS_CONFIGURE_PANE_NO_INDENT_V | OS_CONFIGURE_PANE_NO_INDENT_H);
	((Pane *) menu)->image = OSRectangle(84, 87, 106, 109);
	((Pane *) menu)->imageBorder = OSRectangle(85, 86, 107, 108);
}

void OSSetMenuItem(OSObject menuBar, uintptr_t index, OSObject menu) {
	OSSetPaneObject(OSGetPane(menuBar, 0, index), menu, OS_SET_PANE_OBJECT_HORIZONTAL_PUSH);
	((Control *) menu)->textAlign = OS_DRAW_STRING_HALIGN_LEFT | OS_DRAW_STRING_VALIGN_CENTER;
	((Control *) menu)->textBoundsBorder = 4;
}

void LayoutRootPane(OSObject generator, void *argument, OSCallbackData *data) {
	(void) generator;
	(void) data;

	Pane *pane = (Pane *) argument;
	int width = pane->window->width;
	int height = pane->window->height;
	pane->bounds = OSRectangle(0, width, 0, height);
	if (pane->window->flags & OS_CREATE_WINDOW_NO_DECORATIONS) {
		if (pane->window->flags & OS_CREATE_WINDOW_POPUP) {
			pane->grid[0].bounds = OSRectangle(0 + 5, width - 2, 0 + 2, height - 2);
		} else {
			pane->grid[0].bounds = pane->bounds;
		}
	} else {
		pane->grid[0].bounds = OSRectangle(4, width - 4, 30, height - 4);
		pane->grid[1].bounds = OSRectangle(4, width - 4, 4, 28);
		pane->grid[2].bounds = OSRectangle(4, width - 4, 0, 4);
		pane->grid[3].bounds = OSRectangle(4, width - 4, height - 4, height);
		pane->grid[4].bounds = OSRectangle(0, 4, 4, height - 4);
		pane->grid[5].bounds = OSRectangle(width - 4, width, 4, height - 4);
		pane->grid[6].bounds = OSRectangle(0, 4, 0, 4);
		pane->grid[7].bounds = OSRectangle(width - 4, width, height - 4, height);
		pane->grid[8].bounds = OSRectangle(width - 4, width, 0, 4);
		pane->grid[9].bounds = OSRectangle(0, 4, height - 4, height);

		if (pane->window->flags & OS_CREATE_WINDOW_WITH_MENU_BAR) {
			pane->grid[10].bounds = OSRectangle(4, width - 4, pane->grid[0].bounds.top, pane->grid[0].bounds.top + 21);
			pane->grid[0].bounds.top += 21;
		}
	}

	for (uintptr_t i = 1; i < 16; i++) {
		if (pane->grid[i].control) {
			pane->grid[i].control->bounds = pane->grid[i].bounds;
		}
	}

	LayoutPane(pane->grid + 0);
	LayoutPane(pane->grid + 10);
}

OSObject CreateWindow(char *title, size_t titleLengthBytes, int x, int y, int width, int height, unsigned flags) {
	Window *window = (Window *) OSHeapAllocate(sizeof(Window), true);
	window->flags = flags;

	if (!(flags & OS_CREATE_WINDOW_NO_DECORATIONS)) {
		// Add the size of the border.
		width += BORDER_SIZE_X;
		height += BORDER_SIZE_Y;
	}

	if (flags & OS_CREATE_WINDOW_WITH_MENU_BAR) {
		height += 21;
	}

	window->width = width;
	window->height = height;

	OSRectangle bounds = OSRectangle(x, x + width, y, y + height);
	OSError result = OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) window, (uintptr_t) &bounds, 0, 0);

	if (result != OS_SUCCESS) {
		OSHeapFree(window);
		return nullptr;
	}

	OSSetObjectCallback(&window->pane, OS_OBJECT_PANE, OS_CALLBACK_LAYOUT_PANE, LayoutRootPane, &window->pane);

	// Create the root and content panes.
	InitialisePane(&window->pane, OSRectangle(0, width, 0, height), window, nullptr, 16, 1, nullptr);
	InitialisePane(window->pane.grid + 0, OSRectangle(4, width - 4, 30, height - 4), window, &window->pane, 0, 0, nullptr);

	if (!(flags & OS_CREATE_WINDOW_NO_DECORATIONS)) {
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

		if (flags & OS_CREATE_WINDOW_WITH_MENU_BAR) {
			InitialisePane(window->pane.grid + 10, OSRectangle(0, 0, 0, 0), window, &window->pane, 0, 0, nullptr);
			window->pane.grid[10].image = OSRectangle(34, 34 + 6, 124, 124 + 21);
			window->pane.grid[10].imageBorder = OSRectangle(34 + 2, 34 + 3, 124 + 2, 124 + 3);
		}
	} else {
		// Draw the window background.
		OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, width, 0, height), 
				OSRectangle(84, 88, 106, 110), OSRectangle(84 + 2, 84 + 3, 106 + 2, 106 + 3), OS_DRAW_MODE_REPEAT_FIRST);
	}

	OSLayoutPane(&window->pane);
	OSUpdateWindow(window);

	return window;
}

OSObject OSCreateWindow(char *title, size_t titleLengthBytes, unsigned width, unsigned height, unsigned flags) {
	return CreateWindow(title, titleLengthBytes, 0, 0, width, height, flags);
}

static void DestroyPane(Pane *pane) {
	Control *control = pane->control;

	if (control) {
		OSHeapFree(control->text.buffer);
		OSHeapFree(control);
	}

	for (uintptr_t i = 0; i < pane->gridWidth * pane->gridHeight; i++) {
		DestroyPane(pane->grid + i);
	}

	OSHeapFree(pane->grid);
}

void OSCloseWindow(OSObject _window) {
	OSMessage message;
	message.type = OS_MESSAGE_CLOSE_WINDOW;
	message.targetWindow = _window;
	OSPostMessage(&message);
}

static void RemoveSelectedText(Control *control) {
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

static CharacterType GetCharacterType(int character) {
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

static void MoveCaret(OSString *string, OSCaret *caret, bool right, bool word, bool strongWhitespace = false) {
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

	if (window->hoverControl && window->hoverControl->type == OS_CONTROL_MENU && window->popupMenuControl != window->hoverControl) {
		if ((window->popupChild || !window->hoverControl->menuBar) && window->hoverControl->menuHasChildren) {
			Control *control = window->hoverControl;
			if (window->popupMenuControl) OSInvalidateControl(window->popupMenuControl);
			CreatePopupMenu(window, control, window->popupChild);
		} else if (window->popupChild && !window->hoverControl->menuBar) {
			DeactivateWindow(window->popupChild, window);
		}
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

			if (width < 200) {
				width = 200;
				if (!la) bounds.right = bounds.left + 200;
				else bounds.left = bounds.right - 200;
			}

			if (height < 200) {
				height = 200;
				if (!ta) bounds.bottom = bounds.top + 200;
				else bounds.top = bounds.bottom - 200;
			}

			if (OSMoveWindow(window->handle, bounds) == OS_SUCCESS) {
				window->width = width;
				window->height = height;

				// Redraw the window background and border.
				OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, window->width, 0, window->height), 
						OSRectangle(96, 105, 42, 77), OSRectangle(96 + 3, 96 + 5, 42 + 29, 42 + 31), OS_DRAW_MODE_REPEAT_FIRST);

				// Layout the window.
				OSLayoutPane(&window->pane);

				// Redraw the window.
				DrawPane(&window->pane, true);
			}
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

static void DeactivateWindow(Window *window, Window *newWindow) {
	{
		Window *window2 = newWindow;

		while (window2) {
			if (window2 == window) {
				return;
			}

			window2 = window2->popupParent;
		}
	}

	UpdateMousePosition(window, -1, -1, -1, -1);

	if (window->popupChild) {
		Window *window2 = window->popupChild;
		window->popupChild = nullptr;
		DeactivateWindow(window2, newWindow);
	}

	if (!(window->flags & OS_CREATE_WINDOW_NO_DECORATIONS)) {
		// If we're drawing decorations, then change the border of the window.
		window->pane.grid[1].control->image = OSRectangle(159, 159 + 7, 64, 64 + 24);
		OSInvalidateControl(window->pane.grid[1].control);
		OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, window->width, 0, window->height), 
				OSRectangle(106, 106 + 9, 42, 77), OSRectangle(106 + 3, 106 + 5, 42 + 29, 42 + 31), OS_DRAW_MODE_TRANSPARENT);
	}

	if (window->flags & OS_CREATE_WINDOW_POPUP) {
		if (window->popupParent && window->popupParent->popupChild) {
			window->popupParent->popupChild = nullptr;
			OSInvalidateControl(window->popupParent->popupMenuControl);
			window->popupParent->popupMenuControl = nullptr;

			DeactivateWindow(window->popupParent, newWindow);
		}

		OSCloseWindow(window);
	}

	return;
}

OSError OSProcessGUIMessage(OSMessage *message) {
	// TODO Message security. 
	// 	How should we tell who sent the message?
	// 	(and that they gave us a valid window?)

	Window *window = (Window *) message->targetWindow;
	if (!window) return OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI;

	switch (message->type) {
		case OS_MESSAGE_MOUSE_MOVED: {
			UpdateMousePosition(window, 
					message->mouseMoved.newPositionX, 
					message->mouseMoved.newPositionY,
					message->mouseMoved.newPositionXScreen,
					message->mouseMoved.newPositionYScreen);
		} break;

		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			UpdateMousePosition(window, 
					message->mousePressed.positionX, 
					message->mousePressed.positionY,
					message->mousePressed.positionXScreen,
					message->mousePressed.positionYScreen);

			lastClickX = message->mousePressed.positionX;
			lastClickY = message->mousePressed.positionY;
			lastClickWindowWidth = window->width;
			lastClickWindowHeight = window->height;

			if (window->hoverControl) {
				Control *control = window->hoverControl;

				if (control->canHaveFocus) {
					window->focusedControl = control; 
					FindCaret(control, message->mousePressed.positionX, message->mousePressed.positionY, false, (lastClickChainCount = message->mousePressed.clickChainCount));
				}

				window->pressedControl = control;
				OSInvalidateControl(control);
			}

			if (window->pressedControl) {
				Control *control = window->pressedControl;

				if (control->type == OS_CONTROL_MENU && control->menuHasChildren && control != window->lastPopupMenuControl) {
					CreatePopupMenu(control->window, control, window->popupChild);
				}
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

				if (window->hoverControl) {
					OSSetCursorStyle(window->handle, window->hoverControl->cursorStyle);
				}
			}

			window->lastPopupMenuControl = window->popupMenuControl;
		} break;

		case OS_MESSAGE_WINDOW_CREATED: {
			window->dirty = true;
			LayoutPane(&window->pane);
			InvalidatePane(&window->pane);
			DrawPane(&window->pane, true);
		} break;

		case OS_MESSAGE_WINDOW_DESTROYED: {
			OSHeapFree(window);
			window = nullptr;
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

		case OS_MESSAGE_WINDOW_ACTIVATED: {
			if (!(window->flags & OS_CREATE_WINDOW_NO_DECORATIONS)) {
				window->pane.grid[1].control->image = OSRectangle(149, 149 + 7, 64, 64 + 24);
				OSInvalidateControl(window->pane.grid[1].control);
				OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, window->width, 0, window->height), 
						OSRectangle(96, 96 + 9, 42, 77), OSRectangle(96 + 3, 96 + 5, 42 + 29, 42 + 31), OS_DRAW_MODE_TRANSPARENT);
			}

			if (window->popupChild) {
				DeactivateWindow(window->popupChild, window);
				window->popupChild = nullptr;
			}
		} break;

		case OS_MESSAGE_WINDOW_DEACTIVATED: {
			DeactivateWindow(window, (Window *) message->windowDeactivated.newWindow);
		} break;

		case OS_MESSAGE_MOUSE_EXIT: {
			UpdateMousePosition(window, -1, -1, -1, -1);
		} break;

		case OS_MESSAGE_CLOSE_WINDOW: {
			DestroyPane(&window->pane);
			OSCloseHandle(window->handle);
			OSCloseHandle(window->surface);
		} break;

		default: {
			return OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI;
		}
	}

	if (window && window->dirty) {
		DrawPane(&window->pane, false);
		OSUpdateWindow(window);
		window->dirty = false;
	}

	return OS_SUCCESS;
}

static void CreatePopupMenu(Window *parent, OSObject generator, Window *existingWindow) {
	int x, y;

	OSRectangle bounds;
	Control *control = (Control *) generator;
	OSGetWindowBounds(parent->handle, &bounds);

	if (control->menuBar) {
		x = bounds.left + control->bounds.left;
		y = bounds.top + control->bounds.bottom - 2;
	} else {
		x = bounds.left + control->bounds.right;
		y = bounds.top + control->bounds.top;
	}

	Pane pane = {};
	OSCallbackData populateMenu = {};
	populateMenu.populateMenu.popupMenu = &pane;
	SendCallback(generator, (control)->populateMenu, populateMenu);

	int width, height;
	MeasurePane(&pane, width, height);
	DestroyPane(&pane);
	
	if (width < 100) {
		width = 100;
	}

	if (height < 20) {
		height = 20;
	}

	Window *window = existingWindow;

	if (!window) {
		window = (Window *) CreateWindow((char *) "", 0, x, y, width, height, OS_CREATE_WINDOW_NO_DECORATIONS | OS_CREATE_WINDOW_POPUP);
	} else {
		Window *child = existingWindow->popupChild;
		while (child) if (child->popupChild) child = child->popupChild; else break;
		if (child) DeactivateWindow(child, existingWindow); 
		window->width = width;
		window->height = height;
		DestroyPane(window->pane.grid);
		OSZeroMemory(window->pane.grid, sizeof(Pane));
		InitialisePane(window->pane.grid, OSRectangle(0, 0, 0, 0), window, &window->pane, 0, 0, nullptr);
		OSMoveWindow(window->handle, OSRectangle(x, x + width, y, y + height));
	}

	window->popupParent = parent;
	window->pane.image = OSRectangle(10, 19, 114, 120);
	window->pane.imageBorder = OSRectangle(14, 18, 115, 119);
	window->pane.dirty = 2;

	{
		Pane *pane = (Pane *) OSGetWindowContentPane(window);
		OSCallbackData populateMenu = {};
		populateMenu.populateMenu.popupMenu = pane;
		SendCallback(generator, (control)->populateMenu, populateMenu);
	}

	parent->popupMenuControl = control;
	parent->lastPopupMenuControl = parent->popupMenuControl;
	parent->popupChild = window;

	{
		OSMessage message;
		message.type = OS_MESSAGE_WINDOW_CREATED;
		message.targetWindow = window;
		OSProcessGUIMessage(&message);
	}
}
