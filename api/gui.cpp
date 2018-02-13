#define UI_IMAGE(_image) _image.region, _image.border
#define DIMENSION_PUSH (-1)

struct UIImage {
	OSRectangle region;
	OSRectangle border;
};

UIImage activeWindowBorder;

// TODO Notification callbacks.
//	- Simplifying the target/generator mess.

struct Control : APIObject {
	unsigned layout;
	OSString text;
	OSRectangle bounds, cellBounds;
	int preferredWidth, preferredHeight;
	bool repaint, relayout;
};

struct Grid : APIObject {
	unsigned columns, rows;
	OSObject *objects;
	OSRectangle bounds;
	int *widths, *heights;
	int width, height;
	bool relayout;
};

struct Window : APIObject {
	OSHandle window, surface;
	unsigned width, height;
	Grid *root;
	bool destroyed;
	unsigned flags;
};

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
				OSFillRectangle(message->paint.surface, control->bounds, OSColor(255, 255, 255));
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
					int usedWidth = 4; 
					for (uintptr_t i = 0; i < grid->columns; i++) if (grid->widths[i] != DIMENSION_PUSH) usedWidth += grid->widths[i] + 4;
					int widthPerPush = (grid->bounds.right - grid->bounds.left - usedWidth) / pushH - 4;
					for (uintptr_t i = 0; i < grid->columns; i++) if (grid->widths[i] == DIMENSION_PUSH) grid->widths[i] = widthPerPush;
				}

				if (pushV) {
					int usedHeight = 4; 
					for (uintptr_t j = 0; j < grid->rows; j++) if (grid->heights[j] != DIMENSION_PUSH) usedHeight += grid->heights[j] + 4;
					int heightPerPush = (grid->bounds.bottom - grid->bounds.top - usedHeight) / pushV - 4;
					for (uintptr_t j = 0; j < grid->rows; j++) if (grid->heights[j] == DIMENSION_PUSH) grid->heights[j] = heightPerPush;
				}

				int posX = grid->bounds.left + 4;

				for (uintptr_t i = 0; i < grid->columns; i++) {
					int posY = grid->bounds.top + 4;

					for (uintptr_t j = 0; j < grid->rows; j++) {
						OSObject *object = grid->objects + (j * grid->columns + i);

						message->type = OS_MESSAGE_LAYOUT;
						message->layout.left = posX;
						message->layout.right = posX + grid->widths[i];
						message->layout.top = posY;
						message->layout.bottom = posY + grid->heights[j];
						message->layout.force = true;

						OSSendMessage(*object, message);

						posY += grid->heights[j] + 4;
					}

					posX += grid->widths[i] + 4;
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

			int width = pushH ? DIMENSION_PUSH : 4, height = pushV ? DIMENSION_PUSH : 4;

			if (!pushH) for (uintptr_t i = 0; i < grid->columns; i++) width += grid->widths[i] + 4;
			if (!pushV) for (uintptr_t j = 0; j < grid->rows; j++) height += grid->heights[j] + 4;

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

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

OSObject OSCreateGrid(unsigned columns, unsigned rows) {
	uint8_t *memory = (uint8_t *) OSHeapAllocate(sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * (columns + rows), true);

	Grid *grid = (Grid *) memory;
	grid->type = API_OBJECT_GRID;

	grid->columns = columns;
	grid->rows = rows;
	grid->objects = (OSObject *) (memory + sizeof(Grid));
	grid->widths = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows);
	grid->heights = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns);

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

	if (window->descendentInvalidationFlags & DESCENDENT_REPAINT) {
		window->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;

		OSMessage message;
		message.type = OS_MESSAGE_PAINT;
		message.paint.surface = window->surface;
		message.paint.force = false;
		OSSendMessage(window->root, &message);
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

	OSSetCallback(window, OSCallback(ProcessWindowMessage, window));

	window->root = (Grid *) OSCreateGrid(1, 3);
	window->root->parent = window;

	{
		OSMessage message;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(window->root, &message);
	}

	(void) title;
	(void) titleBytes;

	return window;
}

void OSInitialiseGUI() {
	activeWindowBorder = { {96, 105, 42, 77}, {96 + 3, 96 + 5, 42 + 29, 42 + 31} };
}
