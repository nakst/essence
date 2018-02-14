#define UI_IMAGE(_image) _image.region, _image.border
#define DIMENSION_PUSH (-1)

#define RESIZE_LEFT 		(1)
#define RESIZE_RIGHT 		(2)
#define RESIZE_TOP 		(4)
#define RESIZE_BOTTOM 		(8)
#define RESIZE_TOP_LEFT 	(5)
#define RESIZE_TOP_RIGHT 	(6)
#define RESIZE_BOTTOM_LEFT 	(9)
#define RESIZE_BOTTOM_RIGHT 	(10)
#define RESIZE_MOVE		(0)

struct UIImage {
	OSRectangle region;
	OSRectangle border;
};

UIImage activeWindowBorder11, activeWindowBorder12, activeWindowBorder13, 
	activeWindowBorder21, activeWindowBorder22, activeWindowBorder23, 
	activeWindowBorder31, activeWindowBorder32, activeWindowBorder33, 
	activeWindowBorder41, activeWindowBorder42, activeWindowBorder43;
UIImage inactiveWindowBorder11, inactiveWindowBorder12, inactiveWindowBorder13, 
	inactiveWindowBorder21, inactiveWindowBorder22, inactiveWindowBorder23, 
	inactiveWindowBorder31, inactiveWindowBorder32, inactiveWindowBorder33, 
	inactiveWindowBorder41, inactiveWindowBorder42, inactiveWindowBorder43;

// TODO Notification callbacks.
//	- Simplifying the target/generator mess.

struct Control : APIObject {
	unsigned layout;
	OSString text;
	OSRectangle bounds, cellBounds;
	int preferredWidth, preferredHeight;
	bool repaint, relayout;
	UIImage background;
	OSCursorStyle cursor;
};

struct WindowResizeControl : Control {
	unsigned direction;
};

struct Grid : APIObject {
	unsigned columns, rows;
	OSObject *objects;
	OSRectangle bounds;
	int *widths, *heights;
	int width, height;
	bool relayout;
	int borderSize, gapSize;
};

struct Window : APIObject {
	OSHandle window, surface;
	unsigned width, height;
	Grid *root;
	bool destroyed;
	unsigned flags;
	OSCursorStyle cursor, cursorOld;
	struct Control *drag, *hover;
};

static bool IsPointInRectangle(OSRectangle rectangle, int x, int y) {
	if (rectangle.left > x || rectangle.right <= x || rectangle.top > y || rectangle.bottom <= y) {
		return false;
	}
	
	return true;
}

static OSCallbackResponse ProcessControlMessage(OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Control *control = (Control *) message->context;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			if (control->cellBounds.left == message->layout.left
					&& control->cellBounds.right == message->layout.right
					&& control->cellBounds.top == message->layout.top
					&& control->cellBounds.bottom == message->layout.bottom
					&& !control->relayout) {
				break;
			}

			control->cellBounds = control->bounds = OSRectangle(
					message->layout.left, message->layout.right,
					message->layout.top, message->layout.bottom);

			int width = control->bounds.right - control->bounds.left;
			int height = control->bounds.bottom - control->bounds.top;

			if (width > control->preferredWidth) {
				if (control->layout & OS_CELL_H_CENTER) {
					control->bounds.left = control->bounds.left + width / 2 - control->preferredWidth / 2;
					control->bounds.right = control->bounds.left + control->preferredWidth;
				} else if (control->layout & OS_CELL_H_LEFT) {
					control->bounds.right = control->bounds.left + control->preferredWidth;
				} else if (control->layout & OS_CELL_H_RIGHT) {
					control->bounds.left = control->bounds.right - control->preferredWidth;
				}
			}

			if (height > control->preferredHeight) {
				if (control->layout & OS_CELL_V_CENTER) {
					control->bounds.top = control->bounds.top + height / 2 - control->preferredHeight / 2;
					control->bounds.bottom = control->bounds.top + control->preferredHeight;
				} else if (control->layout & OS_CELL_V_TOP) {
					control->bounds.bottom = control->bounds.top + control->preferredHeight;
				} else if (control->layout & OS_CELL_V_BOTTOM) {
					control->bounds.top = control->bounds.bottom - control->preferredHeight;
				}
			}

			control->relayout = false;
			control->repaint = true;
			SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);
		} break;

		case OS_MESSAGE_MEASURE: {
			message->measure.width = control->preferredWidth;
			message->measure.height = control->preferredHeight;

			if (control->layout & OS_CELL_H_PUSH) message->measure.width = DIMENSION_PUSH;
			if (control->layout & OS_CELL_V_PUSH) message->measure.height = DIMENSION_PUSH;
		} break;

		case OS_MESSAGE_PAINT: {
			if (control->repaint || message->paint.force) {
				if (control->background.region.left) {
					OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->background.region, control->background.border, OS_DRAW_MODE_REPEAT_FIRST);
				} else {
					OSFillRectangle(message->paint.surface, control->bounds, OSColor(255, 255, 255));
				}

				OSDrawString(message->paint.surface, control->bounds, &control->text,
						OS_DRAW_STRING_HALIGN_CENTER | OS_DRAW_STRING_VALIGN_CENTER, 0x003060, -1);

				control->repaint = false;
			}
		} break;

		case OS_MESSAGE_PARENT_UPDATED: {
			control->repaint = true;
			SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);
		} break;

		case OS_MESSAGE_DESTROY: {
			OSHeapFree(control->text.buffer);
			OSHeapFree(control);

			Window *window = (Window *) message->window;

			if (window->hover == control) window->hover = nullptr;
			if (window->drag == control) window->drag = nullptr;
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (!IsPointInRectangle(control->bounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				break;
			}

			Window *window = (Window *) message->window;
			window->cursor = control->cursor;
			window->hover = control;
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

static void CreateString(char *text, size_t textBytes, OSString *string) {
	OSHeapFree(string->buffer);
	string->buffer = (char *) OSHeapAllocate(textBytes, false);
	string->bytes = textBytes;
	OSCopyMemory(string->buffer, text, textBytes);
}

static OSCallbackResponse ProcessWindowResizeHandleMessage(OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	WindowResizeControl *control = (WindowResizeControl *) message->context;

	if (message->type == OS_MESSAGE_MOUSE_DRAGGED) {
		Window *window = (Window *) message->window;
		OSRectangle bounds;
		OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, window->window, (uintptr_t) &bounds, 0, 0);

		if (control->direction & RESIZE_LEFT) bounds.left = message->mouseMoved.newPositionXScreen;
		if (control->direction & RESIZE_RIGHT) bounds.right = message->mouseMoved.newPositionXScreen;
		if (control->direction & RESIZE_TOP) bounds.top = message->mouseMoved.newPositionYScreen;
		if (control->direction & RESIZE_BOTTOM) bounds.bottom = message->mouseMoved.newPositionYScreen;
		
		OSSyscall(OS_SYSCALL_MOVE_WINDOW, window->window, (uintptr_t) &bounds, 0, 0);

		window->width = bounds.right - bounds.left;
		window->height = bounds.bottom - bounds.top;

		OSMessage layout;
		layout.type = OS_MESSAGE_LAYOUT;
		layout.layout.left = 0;
		layout.layout.top = 0;
		layout.layout.right = window->width;
		layout.layout.bottom = window->height;
		layout.layout.force = true;
		OSSendMessage(window->root, &layout);
	} else {
		response = OSForwardMessage(OSCallback(ProcessControlMessage, control), message);
	}

	return response;
}

static OSObject CreateWindowResizeHandle(UIImage image, unsigned direction) {
	WindowResizeControl *control = (WindowResizeControl *) OSHeapAllocate(sizeof(WindowResizeControl), true);
	control->type = API_OBJECT_CONTROL;
	control->background = image;
	control->preferredWidth = image.region.right - image.region.left;
	control->preferredHeight = image.region.bottom - image.region.top;
	control->direction = direction;
	OSSetCallback(control, OSCallback(ProcessWindowResizeHandleMessage, control));

	switch (direction) {
		case RESIZE_LEFT:
		case RESIZE_RIGHT:
			control->cursor = OS_CURSOR_RESIZE_HORIZONTAL;
			break;

		case RESIZE_TOP:
		case RESIZE_BOTTOM:
			control->cursor = OS_CURSOR_RESIZE_VERTICAL;
			break;

		case RESIZE_TOP_RIGHT:
		case RESIZE_BOTTOM_LEFT:
			control->cursor = OS_CURSOR_RESIZE_DIAGONAL_1;
			break;

		case RESIZE_TOP_LEFT:
		case RESIZE_BOTTOM_RIGHT:
			control->cursor = OS_CURSOR_RESIZE_DIAGONAL_2;
			break;
	}

	return control;
}

OSObject OSCreateLabel(char *text, size_t textBytes) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;

	OSSetText(control, text, textBytes);
	OSSetCallback(control, OSCallback(ProcessControlMessage, control));

	return control;
}

void OSSetText(OSObject _control, char *text, size_t textBytes) {
	Control *control = (Control *) _control;
	CreateString(text, textBytes, &control->text);

	control->preferredWidth = MeasureStringWidth(text, textBytes, FONT_SIZE) + 4;
	control->preferredHeight = FONT_SIZE + 4;

	control->repaint = true;
	control->relayout = true;
	SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);

	{
		OSMessage message;
		message.type = OS_MESSAGE_CHILD_UPDATED;
		OSSendMessage(control->parent, &message);
	}
}

void OSAddControl(OSObject _grid, unsigned column, unsigned row, OSObject _control, unsigned layout) {
	APIObject *_object = (APIObject *) _grid;

	if (_object->type == API_OBJECT_WINDOW) {
		_grid = ((Window *) _grid)->root;
	}

	Grid *grid = (Grid *) _grid;

	grid->relayout = true;
	SetParentDescendentInvalidationFlags(grid, DESCENDENT_RELAYOUT);

	if (column >= grid->columns || row >= grid->rows) {
		OSCrashProcess(OS_FATAL_ERROR_OUT_OF_GRID_BOUNDS);
	}

	Control *control = (Control *) _control;
	if (control->type != API_OBJECT_CONTROL && layout != OS_ADD_CHILD_GRID) OSCrashProcess(OS_FATAL_ERROR_INVALID_PANE_OBJECT);
	if (layout != OS_ADD_CHILD_GRID) control->layout = layout;
	control->parent = grid;

	OSObject *object = grid->objects + (row * grid->columns + column);
	if (*object) OSCrashProcess(OS_FATAL_ERROR_OVERWRITE_GRID_OBJECT);
	*object = control;

	{
		OSMessage message;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(control, &message);
	}
}

static OSCallbackResponse ProcessGridMessage(OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Grid *grid = (Grid *) message->context;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			if (grid->relayout || message->layout.force) {
				grid->relayout = false;

				grid->bounds = OSRectangle(
						message->layout.left, message->layout.right,
						message->layout.top, message->layout.bottom);

				OSZeroMemory(grid->widths, sizeof(int) * grid->columns);
				OSZeroMemory(grid->heights, sizeof(int) * grid->rows);

				int pushH = 0, pushV = 0;

				for (uintptr_t i = 0; i < grid->columns; i++) {
					for (uintptr_t j = 0; j < grid->rows; j++) {
						OSObject *object = grid->objects + (j * grid->columns + i);
						message->type = OS_MESSAGE_MEASURE;
						if (OSSendMessage(*object, message) == OS_CALLBACK_NOT_HANDLED) continue;

						int width = message->measure.width;
						int height = message->measure.height;

						if (width == DIMENSION_PUSH) { bool a = grid->widths[i] == DIMENSION_PUSH; grid->widths[i] = DIMENSION_PUSH; if (!a) pushH++; }
						else if (grid->widths[i] < width && grid->widths[i] != DIMENSION_PUSH) grid->widths[i] = width;
						if (height == DIMENSION_PUSH) { bool a = grid->heights[j] == DIMENSION_PUSH; grid->heights[j] = DIMENSION_PUSH; if (!a) pushV++; }
						else if (grid->heights[j] < height && grid->heights[j] != DIMENSION_PUSH) grid->heights[j] = height;
					}
				}

				if (pushH) {
					int usedWidth = grid->borderSize * 2; 
					for (uintptr_t i = 0; i < grid->columns; i++) if (grid->widths[i] != DIMENSION_PUSH) usedWidth += grid->widths[i] + (i == grid->columns - 1 ? grid->gapSize : 0);
					int widthPerPush = (grid->bounds.right - grid->bounds.left - usedWidth) / pushH - grid->gapSize; // This isn't completely correct... but I think it's good enough for now?
					for (uintptr_t i = 0; i < grid->columns; i++) if (grid->widths[i] == DIMENSION_PUSH) grid->widths[i] = widthPerPush;
				}

				if (pushV) {
					int usedHeight = grid->borderSize * 2; 
					for (uintptr_t j = 0; j < grid->rows; j++) if (grid->heights[j] != DIMENSION_PUSH) usedHeight += grid->heights[j] + (j == grid->rows - 1 ? grid->gapSize : 0);
					int heightPerPush = (grid->bounds.bottom - grid->bounds.top - usedHeight) / pushV - grid->gapSize; // This isn't completely correct... but I think it's good enough for now?
					for (uintptr_t j = 0; j < grid->rows; j++) if (grid->heights[j] == DIMENSION_PUSH) grid->heights[j] = heightPerPush;
				}

				int posX = grid->bounds.left + grid->borderSize;

				for (uintptr_t i = 0; i < grid->columns; i++) {
					int posY = grid->bounds.top + grid->borderSize;

					for (uintptr_t j = 0; j < grid->rows; j++) {
						OSObject *object = grid->objects + (j * grid->columns + i);

						message->type = OS_MESSAGE_LAYOUT;
						message->layout.left = posX;
						message->layout.right = posX + grid->widths[i];
						message->layout.top = posY;
						message->layout.bottom = posY + grid->heights[j];
						message->layout.force = true;

						OSSendMessage(*object, message);

						posY += grid->heights[j] + grid->gapSize;
					}

					posX += grid->widths[i] + grid->gapSize;
				}
			} else if (grid->descendentInvalidationFlags & DESCENDENT_RELAYOUT) {
				for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
					if (grid->objects[i]) {
						OSSendMessage(grid->objects[i], message);
					}
				}
			}

			grid->descendentInvalidationFlags &= ~DESCENDENT_RELAYOUT;
		} break;

		case OS_MESSAGE_MEASURE: {
			OSZeroMemory(grid->widths, sizeof(int) * grid->columns);
			OSZeroMemory(grid->heights, sizeof(int) * grid->rows);

			bool pushH = false, pushV = false;

			for (uintptr_t i = 0; i < grid->columns; i++) {
				for (uintptr_t j = 0; j < grid->rows; j++) {
					OSObject *object = grid->objects + (j * grid->columns + i);
					if (OSSendMessage(*object, message) == OS_CALLBACK_NOT_HANDLED) continue;

					int width = message->measure.width;
					int height = message->measure.height;

					if (width == DIMENSION_PUSH) pushH = true;
					if (height == DIMENSION_PUSH) pushV = true;

					if (grid->widths[i] < width) grid->widths[i] = width;
					if (grid->heights[j] < height) grid->heights[j] = height;
				}
			}

			int width = pushH ? DIMENSION_PUSH : grid->borderSize, height = pushV ? DIMENSION_PUSH : grid->borderSize;

			if (!pushH) for (uintptr_t i = 0; i < grid->columns; i++) width += grid->widths[i] + (i == grid->columns - 1 ? grid->borderSize : grid->gapSize);
			if (!pushV) for (uintptr_t j = 0; j < grid->rows; j++) height += grid->heights[j] + (j == grid->rows - 1 ? grid->borderSize : grid->gapSize);

			grid->width = message->measure.width = width;
			grid->height = message->measure.height = height;
		} break;

		case OS_MESSAGE_PAINT: {
			if (grid->descendentInvalidationFlags & DESCENDENT_REPAINT) {
				grid->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;

				for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
					if (grid->objects[i]) {
						OSSendMessage(grid->objects[i], message);
					}
				}
			}
		} break;

		case OS_MESSAGE_CHILD_UPDATED: {
			grid->relayout = true;
			SetParentDescendentInvalidationFlags(grid, DESCENDENT_RELAYOUT);
		} break;

		case OS_MESSAGE_DESTROY: {
			for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
				OSSendMessage(grid->objects[i], message);
			}

			OSHeapFree(grid);
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (!IsPointInRectangle(grid->bounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				break;
			}

			for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
				OSSendMessage(grid->objects[i], message);
			}
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

OSObject OSCreateGrid(unsigned columns, unsigned rows, unsigned flags) {
	uint8_t *memory = (uint8_t *) OSHeapAllocate(sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * (columns + rows), true);

	Grid *grid = (Grid *) memory;
	grid->type = API_OBJECT_GRID;

	grid->columns = columns;
	grid->rows = rows;
	grid->objects = (OSObject *) (memory + sizeof(Grid));
	grid->widths = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows);
	grid->heights = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns);

	if (flags & OS_CREATE_GRID_NO_BORDER) grid->borderSize = 0; else grid->borderSize = 4;
	if (flags & OS_CREATE_GRID_NO_GAP) grid->gapSize = 0; else grid->gapSize = 4;

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
		} break;

		case OS_MESSAGE_WINDOW_DESTROYED: {
			OSHeapFree(window);
			return response;
		} break;

		case OS_MESSAGE_DESTROY: {
			OSSendMessage(window->root, message);
			OSCloseHandle(window->surface);
			OSCloseHandle(window->window);
			window->destroyed = true;
		} break;

		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			window->drag = window->hover;
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			window->drag = nullptr;

			OSMessage m = *message;
			m.type = OS_MESSAGE_MOUSE_MOVED;
			m.mouseMoved.oldPositionX = message->mousePressed.positionX;
			m.mouseMoved.oldPositionY = message->mousePressed.positionY;
			m.mouseMoved.newPositionX = message->mousePressed.positionX;
			m.mouseMoved.newPositionY = message->mousePressed.positionY;
			m.mouseMoved.newPositionXScreen = message->mousePressed.positionXScreen;
			m.mouseMoved.newPositionYScreen = message->mousePressed.positionYScreen;
			OSSendMessage(window->root, &m);
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (window->drag) {
				message->type = OS_MESSAGE_MOUSE_DRAGGED;
				OSSendMessage(window->drag, message);
			} else {
				OSSendMessage(window->root, message);
			}
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	if (window->destroyed) {
		return response;
	}

	if (window->descendentInvalidationFlags & DESCENDENT_RELAYOUT) {
		window->descendentInvalidationFlags &= ~DESCENDENT_RELAYOUT;

		OSMessage message;
		message.type = OS_MESSAGE_LAYOUT;
		message.layout.left = 0;
		message.layout.top = 0;
		message.layout.right = window->width;
		message.layout.bottom = window->height;
		message.layout.force = false;
		OSSendMessage(window->root, &message);
	}

	bool updateWindow = false;

	if (window->cursor != window->cursorOld) {
		OSSyscall(OS_SYSCALL_SET_CURSOR_STYLE, window->window, window->cursor, 0, 0);
		window->cursorOld = window->cursor;
		updateWindow = true;
	}

	if (window->descendentInvalidationFlags & DESCENDENT_REPAINT) {
		window->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;

		OSMessage message;
		message.type = OS_MESSAGE_PAINT;
		message.paint.surface = window->surface;
		message.paint.force = false;
		OSSendMessage(window->root, &message);
		updateWindow = true;
	}

	if (updateWindow) {
		OSSyscall(OS_SYSCALL_UPDATE_WINDOW, window->window, 0, 0, 0);
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
	window->flags = flags;
	window->cursor = OS_CURSOR_NORMAL;

	OSSetCallback(window, OSCallback(ProcessWindowMessage, window));

	window->root = (Grid *) OSCreateGrid(3, 4, OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_GAP);
	window->root->parent = window;

	{
		OSMessage message;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(window->root, &message);
	}

	{
		OSAddControl(window->root, 0, 0, CreateWindowResizeHandle(activeWindowBorder11, RESIZE_TOP_LEFT), 0);
		OSAddControl(window->root, 1, 0, CreateWindowResizeHandle(activeWindowBorder12, RESIZE_TOP), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSAddControl(window->root, 2, 0, CreateWindowResizeHandle(activeWindowBorder13, RESIZE_TOP_RIGHT), 0);
		OSAddControl(window->root, 0, 1, CreateWindowResizeHandle(activeWindowBorder21, RESIZE_LEFT), 0);
		OSAddControl(window->root, 1, 1, CreateWindowResizeHandle(activeWindowBorder22, RESIZE_MOVE), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSAddControl(window->root, 2, 1, CreateWindowResizeHandle(activeWindowBorder23, RESIZE_RIGHT), 0);
		OSAddControl(window->root, 0, 2, CreateWindowResizeHandle(activeWindowBorder31, RESIZE_LEFT), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
		OSAddControl(window->root, 1, 2, CreateWindowResizeHandle(activeWindowBorder32, RESIZE_MOVE), OS_CELL_H_EXPAND | OS_CELL_V_EXPAND); // TODO Temporary.
		OSAddControl(window->root, 2, 2, CreateWindowResizeHandle(activeWindowBorder33, RESIZE_RIGHT), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
		OSAddControl(window->root, 0, 3, CreateWindowResizeHandle(activeWindowBorder41, RESIZE_BOTTOM_LEFT), 0);
		OSAddControl(window->root, 1, 3, CreateWindowResizeHandle(activeWindowBorder42, RESIZE_BOTTOM), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSAddControl(window->root, 2, 3, CreateWindowResizeHandle(activeWindowBorder43, RESIZE_BOTTOM_RIGHT), 0);
	}

	(void) title;
	(void) titleBytes;

	return window;
}

void OSInitialiseGUI() {
	activeWindowBorder11 =   { {1, 1 + 6, 144, 144 + 6}, 			{1, 1, 144, 144} };
	activeWindowBorder12 =   { {8, 8 + 1, 144, 144 + 6}, 			{7, 8, 144, 144} };
	activeWindowBorder13 =   { {10, 10 + 6, 144, 144 + 6}, 			{10, 10, 144, 144} };
	activeWindowBorder21 =   { {1, 1 + 6, 151, 151 + 24}, 			{1, 1, 151, 151} };
	activeWindowBorder22 =   { {8, 8 + 1, 151, 151 + 24}, 			{7, 8, 151, 151} };
	activeWindowBorder23 =   { {10, 10 + 6, 151, 151 + 24}, 		{10, 10, 151, 151} };
	activeWindowBorder31 =   { {1, 1 + 6, 176, 176 + 1}, 			{1, 1, 175, 176} };
	activeWindowBorder32 =   { {8, 8 + 1, 176, 176 + 1}, 			{7, 8, 175, 176} };
	activeWindowBorder33 =   { {10, 10 + 6, 176, 176 + 1}, 			{10, 10, 175, 176} };
	activeWindowBorder41 =   { {1, 1 + 6, 178, 178 + 6}, 			{1, 1, 178, 178} };
	activeWindowBorder42 =   { {8, 8 + 1, 178, 178 + 6}, 			{7, 8, 178, 178} };
	activeWindowBorder43 =   { {10, 10 + 6, 178, 178 + 6}, 			{10, 10, 178, 178} };
	inactiveWindowBorder11 =   { {16 + 1, 16 + 1 + 6, 144, 144 + 6}, 	{16 + 1, 16 + 1, 144, 144} };
	inactiveWindowBorder12 =   { {16 + 8, 16 + 8 + 1, 144, 144 + 6}, 	{16 + 7, 16 + 8, 144, 144} };
	inactiveWindowBorder13 =   { {16 + 10, 16 + 10 + 6, 144, 144 + 6}, 	{16 + 10, 16 + 10, 144, 144} };
	inactiveWindowBorder21 =   { {16 + 1, 16 + 1 + 6, 151, 151 + 24}, 	{16 + 1, 16 + 1, 151, 151} };
	inactiveWindowBorder22 =   { {16 + 8, 16 + 8 + 1, 151, 151 + 24}, 	{16 + 7, 16 + 8, 151, 151} };
	inactiveWindowBorder23 =   { {16 + 10, 16 + 10 + 6, 151, 151 + 24}, 	{16 + 10, 16 + 10, 151, 151} };
	inactiveWindowBorder31 =   { {16 + 1, 16 + 1 + 6, 176, 176 + 1}, 	{16 + 1, 16 + 1, 175, 176} };
	inactiveWindowBorder32 =   { {16 + 8, 16 + 8 + 1, 176, 176 + 1}, 	{16 + 7, 16 + 8, 175, 176} };
	inactiveWindowBorder33 =   { {16 + 10, 16 + 10 + 6, 176, 176 + 1}, 	{16 + 10, 16 + 10, 175, 176} };
	inactiveWindowBorder41 =   { {16 + 1, 16 + 1 + 6, 178, 178 + 6}, 	{16 + 1, 16 + 1, 178, 178} };
	inactiveWindowBorder42 =   { {16 + 8, 16 + 8 + 1, 178, 178 + 6}, 	{16 + 7, 16 + 8, 178, 178} };
	inactiveWindowBorder43 =   { {16 + 10, 16 + 10 + 6, 178, 178 + 6}, 	{16 + 10, 16 + 10, 178, 178} };
}
