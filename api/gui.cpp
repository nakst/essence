#define UI_IMAGE(_image) _image.region, _image.border
#define DIMENSION_PUSH (-1)

#define CARET_BLINK_HZ (2)

#define RESIZE_LEFT 		(1)
#define RESIZE_RIGHT 		(2)
#define RESIZE_TOP 		(4)
#define RESIZE_BOTTOM 		(8)
#define RESIZE_TOP_LEFT 	(5)
#define RESIZE_TOP_RIGHT 	(6)
#define RESIZE_BOTTOM_LEFT 	(9)
#define RESIZE_BOTTOM_RIGHT 	(10)
#define RESIZE_MOVE		(0)

#define STANDARD_BACKGROUND_COLOR (0xFFF0F0F5)

// TODO Prevent flickering during window resize.
// TODO Cell layouts for child grids.
// TODO Disabling textbox with last focus and selection
// TODO Gain focus - make what selection with textbox, with/without last focus, with/without previous selection?

struct UIImage {
	OSRectangle region;
	OSRectangle border;
};

static UIImage activeWindowBorder11	= {{1, 1 + 6, 144, 144 + 6}, 	{1, 1, 144, 144}};
static UIImage activeWindowBorder12	= {{8, 8 + 1, 144, 144 + 6}, 	{7, 8, 144, 144}};
static UIImage activeWindowBorder13	= {{10, 10 + 6, 144, 144 + 6}, 	{10, 10, 144, 144}};
static UIImage activeWindowBorder21	= {{1, 1 + 6, 151, 151 + 24}, 	{1, 1, 151, 151}};
static UIImage activeWindowBorder22	= {{8, 8 + 1, 151, 151 + 24}, 	{7, 8, 151, 151}};
static UIImage activeWindowBorder23	= {{10, 10 + 6, 151, 151 + 24},	{10, 10, 151, 151}};
static UIImage activeWindowBorder31	= {{1, 1 + 6, 176, 176 + 1}, 	{1, 1, 175, 176}};
static UIImage activeWindowBorder33	= {{10, 10 + 6, 176, 176 + 1}, 	{10, 10, 175, 176}};
static UIImage activeWindowBorder41	= {{1, 1 + 6, 178, 178 + 6}, 	{1, 1, 178, 178}};
static UIImage activeWindowBorder42	= {{8, 8 + 1, 178, 178 + 6}, 	{7, 8, 178, 178}};
static UIImage activeWindowBorder43	= {{10, 10 + 6, 178, 178 + 6}, 	{10, 10, 178, 178}};

static UIImage inactiveWindowBorder11	= {{16 + 1, 16 + 1 + 6, 144, 144 + 6}, 	{16 + 1, 16 + 1, 144, 144}};
static UIImage inactiveWindowBorder12	= {{16 + 8, 16 + 8 + 1, 144, 144 + 6}, 	{16 + 7, 16 + 8, 144, 144}};
static UIImage inactiveWindowBorder13	= {{16 + 10, 16 + 10 + 6, 144, 144 + 6},{16 + 10, 16 + 10, 144, 144}};
static UIImage inactiveWindowBorder21	= {{16 + 1, 16 + 1 + 6, 151, 151 + 24}, {16 + 1, 16 + 1, 151, 151}};
static UIImage inactiveWindowBorder22	= {{16 + 8, 16 + 8 + 1, 151, 151 + 24}, {16 + 7, 16 + 8, 151, 151}};
static UIImage inactiveWindowBorder23	= {{16 + 10, 16 + 10 + 6, 151, 151 + 24},{16 + 10, 16 + 10, 151, 151}};
static UIImage inactiveWindowBorder31	= {{16 + 1, 16 + 1 + 6, 176, 176 + 1}, 	{16 + 1, 16 + 1, 175, 176}};
static UIImage inactiveWindowBorder33	= {{16 + 10, 16 + 10 + 6, 176, 176 + 1}, {16 + 10, 16 + 10, 175, 176}};
static UIImage inactiveWindowBorder41	= {{16 + 1, 16 + 1 + 6, 178, 178 + 6}, 	{16 + 1, 16 + 1, 178, 178}};
static UIImage inactiveWindowBorder42	= {{16 + 8, 16 + 8 + 1, 178, 178 + 6}, 	{16 + 7, 16 + 8, 178, 178}};
static UIImage inactiveWindowBorder43	= {{16 + 10, 16 + 10 + 6, 178, 178 + 6}, {16 + 10, 16 + 10, 178, 178}};

static UIImage progressBarBackground 	= {{1, 8, 122, 143}, {3, 6, 125, 139}};
static UIImage progressBarDisabled   	= {{9, 16, 122, 143}, {11, 14, 125, 139}};
static UIImage progressBarPellet     	= {{18, 26, 69, 84}, {18, 18, 69, 69}};

static UIImage buttonNormal		= {{51, 59, 88, 109}, {51 + 3, 51 + 5, 88 + 10, 88 + 11}};
static UIImage buttonDragged		= {{9 + 51, 9 + 59, 88, 109}, {9 + 54, 9 + 56, 98, 99}};
static UIImage buttonHover		= {{-9 + 51, -9 + 59, 88, 109}, {-9 + 54, -9 + 56, 98, 99}};
static UIImage buttonDisabled		= {{18 + 51, 18 + 59, 88, 109}, {18 + 54, 18 + 56, 98, 99}};

static UIImage checkboxNormal		= {{95, 108, 120, 133}, {95, 95, 120, 120}};
static UIImage checkboxDragged		= {{14 + 95, 14 + 108, 120, 133}, {14 + 95, 14 + 95, 120, 120}};
static UIImage checkboxHover		= {{-14 + 95, -14 + 108, 120, 133}, {-14 + 95, -14 + 95, 120, 120}};
static UIImage checkboxDisabled		= {{28 + 95, 28 + 108, 120, 133}, {28 + 95, 28 + 95, 120, 120}};
static UIImage checkboxNormalChecked	= {{95, 108, 14 + 120, 14 + 133}, {95, 95, 14 + 120, 14 + 120}};
static UIImage checkboxDraggedChecked	= {{14 + 95, 14 + 108, 14 + 120, 14 + 133}, {14 + 95, 14 + 95, 14 + 120, 14 + 120}};
static UIImage checkboxHoverChecked	= {{-14 + 95, -14 + 108, 14 + 120, 14 + 133}, {-14 + 95, -14 + 95, 14 + 120, 14 + 120}};
static UIImage checkboxDisabledChecked	= {{28 + 95, 28 + 108, 14 + 120, 14 + 133}, {28 + 95, 28 + 95, 14 + 120, 14 + 120}};

static UIImage textboxNormal		= {{52, 61, 166, 189}, {55, 58, 169, 186}};
static UIImage textboxFocus		= {{11 + 52, 11 + 61, 166, 189}, {11 + 55, 11 + 58, 169, 186}};
static UIImage textboxHover		= {{-11 + 52, -11 + 61, 166, 189}, {-11 + 55, -11 + 58, 169, 186}};
static UIImage textboxDisabled		= {{22 + 52, 22 + 61, 166, 189}, {22 + 55, 22 + 58, 169, 186}};

static const int totalBorderWidth = 6 + 6;
static const int totalBorderHeight = 6 + 24 + 6;

struct Control : APIObject {
	// Current size: ~320 bytes.
	// Is this too big?
	
	struct Window *window;
		
	OSRectangle bounds, cellBounds;
	uint16_t layout;

	uint32_t backgroundColor;

	UIImage *background, *disabledBackground, *hoverBackground, *dragBackground;
	UIImage *icon, *disabledIcon, *hoverIcon, *dragIcon;

	OSAction *action;
	OSCursorStyle cursor;

	OSString text;
	OSRectangle textBounds;
	uint32_t textColor;
	uint8_t textSize, textAlign;

	uint32_t textShadow : 1, 
		textBold : 1,
		repaint : 1, 
		relayout : 1,
		noAnimations : 1,
		focusable : 1,
		customTextRendering : 1,
		drawParentBackground : 1,
		noDisabledTextColorChange : 1,
		keepCustomCursorWhenDisabled : 1;

	uint16_t preferredWidth, preferredHeight;
	uint16_t minimumWidth, minimumHeight; // Used by OSSetText.

	bool disabled;

	LinkedItem<Control> timerControlItem;
	uint16_t timerHz, timerStep;

	uint8_t animationStep, finalAnimationStep;
	uint8_t from1, from2, from3, from4;
	uint8_t current1, current2, current3, current4;
};

struct Textbox : Control {
	OSCaret caret, caret2;
	bool caretBlink;
	OSCaret wordSelectionAnchor, wordSelectionAnchor2;
};

struct ProgressBar : Control {
	int minimum, maximum, value;
};

struct WindowResizeControl : Control {
	unsigned direction;
};

struct Grid : APIObject {
	struct Window *window;
	unsigned columns, rows;
	OSObject *objects;
	OSRectangle bounds;
	int *widths, *heights;
	int width, height;
	bool relayout, repaint;
	int borderSize, gapSize;
};

struct Window : APIObject {
	OSHandle window, surface;
	Grid *root;

	unsigned flags;
	OSCursorStyle cursor, cursorOld;
	struct Control *drag, *hover, *focus, *lastFocus;
	bool destroyed, created;

	int width, height;
	int minimumWidth, minimumHeight;

	LinkedList<Control> timerControls;
	int timerHz, caretBlinkStep, caretBlinkPause;
};

static void OSRepaintControl(OSObject _control) {
	Control *control = (Control *) _control;
	control->repaint = true;
	SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);
}

static bool IsPointInRectangle(OSRectangle rectangle, int x, int y) {
	if (rectangle.left > x || rectangle.right <= x || rectangle.top > y || rectangle.bottom <= y) {
		return false;
	}
	
	return true;
}

static void UpdateCheckboxIcons(Control *control) {
	OSAction *action = control->action;
	control->icon = action->isChecked ? &checkboxNormalChecked : &checkboxNormal;
	control->disabledIcon = action->isChecked ? &checkboxDisabledChecked : &checkboxDisabled;
	control->hoverIcon = action->isChecked ? &checkboxHoverChecked : &checkboxHover;
	control->dragIcon = action->isChecked ? &checkboxDraggedChecked : &checkboxDragged;
}

void OSAnimateControl(OSObject _control, bool fast) {
	Control *control = (Control *) _control;

	if (!control->noAnimations) {
		control->from1 = control->current1;
		control->from2 = control->current2;
		control->from3 = control->current3;
		control->from4 = control->current4;
		control->animationStep = 0;
		control->finalAnimationStep = fast ? 4 : 16;

		if (!control->timerControlItem.list) {
			control->timerHz = 30;
			control->window->timerControls.InsertStart(&control->timerControlItem);
			control->timerControlItem.thisItem = control;
		}
	} else {
		control->animationStep = 16;
		control->finalAnimationStep = 16;
	}

	OSRepaintControl(control);
}

static OSCallbackResponse ProcessControlMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Control *control = (Control *) _object;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			if (control->cellBounds.left == message->layout.left
					&& control->cellBounds.right == message->layout.right
					&& control->cellBounds.top == message->layout.top
					&& control->cellBounds.bottom == message->layout.bottom
					&& !control->relayout) {
				break;
			}

			control->cellBounds = control->bounds = OS_MAKE_RECTANGLE(
					message->layout.left, message->layout.right,
					message->layout.top, message->layout.bottom);

			int width = control->bounds.right - control->bounds.left;
			int height = control->bounds.bottom - control->bounds.top;

			if (width > control->preferredWidth) {
				if (control->layout & OS_CELL_H_EXPAND) {
				} else if (control->layout & OS_CELL_H_LEFT) {
					control->bounds.right = control->bounds.left + control->preferredWidth;
				} else if (control->layout & OS_CELL_H_RIGHT) {
					control->bounds.left = control->bounds.right - control->preferredWidth;
				} else {
					control->bounds.left = control->bounds.left + width / 2 - control->preferredWidth / 2;
					control->bounds.right = control->bounds.left + control->preferredWidth;
				}
			}

			if (height > control->preferredHeight) {
				if (control->layout & OS_CELL_V_EXPAND) {
				} else if (control->layout & OS_CELL_V_TOP) {
					control->bounds.bottom = control->bounds.top + control->preferredHeight;
				} else if (control->layout & OS_CELL_V_BOTTOM) {
					control->bounds.top = control->bounds.bottom - control->preferredHeight;
				} else {
					control->bounds.top = control->bounds.top + height / 2 - control->preferredHeight / 2;
					control->bounds.bottom = control->bounds.top + control->preferredHeight;
				}
			}

			control->relayout = false;
			OSRepaintControl(control);
			SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);

			{
				OSMessage m = *message;
				m.type = OS_MESSAGE_LAYOUT_TEXT;
				OSSendMessage(control, &m);
			}
		} break;

		case OS_MESSAGE_LAYOUT_TEXT: {
			control->textBounds = control->bounds;

			if (control->icon) {
				control->textBounds.left += control->icon->region.right - control->icon->region.left + 4;
			}
		} break;

		case OS_MESSAGE_MEASURE: {
			message->measure.width = control->preferredWidth;
			message->measure.height = control->preferredHeight;

			if (control->layout & OS_CELL_H_PUSH) message->measure.width = DIMENSION_PUSH;
			if (control->layout & OS_CELL_V_PUSH) message->measure.height = DIMENSION_PUSH;
		} break;

		case OS_MESSAGE_PAINT: {
			if (control->repaint || message->paint.force) {
				if (control->drawParentBackground) {
					OSMessage m = *message;
					m.type = OS_MESSAGE_PAINT_BACKGROUND;
					m.paintBackground.surface = message->paint.surface;
					m.paintBackground.left = control->bounds.left;
					m.paintBackground.right = control->bounds.right;
					m.paintBackground.top = control->bounds.top;
					m.paintBackground.bottom = control->bounds.bottom;
					OSSendMessage(control->parent, &m);
				}

				if (control->backgroundColor) {
					OSFillRectangle(message->paint.surface, control->bounds, 
							OSColor(control->backgroundColor));
				}

				bool disabled = control->disabled,
				     drag = ((control->window->drag == control && control->window->hover == control) || control->window->focus == control) && !disabled,
				     hover = (control->window->hover == control || control->window->drag == control) && !drag && !disabled,
				     normal = !hover && !drag && !disabled;

				control->current1 = ((normal   ? 15 : 0) - control->from1) * control->animationStep / control->finalAnimationStep + control->from1;
				control->current2 = ((hover    ? 15 : 0) - control->from2) * control->animationStep / control->finalAnimationStep + control->from2;
				control->current3 = ((drag     ? 15 : 0) - control->from3) * control->animationStep / control->finalAnimationStep + control->from3;
				control->current4 = ((disabled ? 15 : 0) - control->from4) * control->animationStep / control->finalAnimationStep + control->from4;

				if (control->background) {
					OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->background->region, 
							control->background->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF);
				}

				if (control->hoverBackground && control->current2) {
					OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->hoverBackground->region, 
							control->hoverBackground->border, OS_DRAW_MODE_REPEAT_FIRST, control->current2 == 15 ? 0xFF : 0xF * control->current2);
				}

				if (control->dragBackground && control->current3) {
					OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->dragBackground->region, 
							control->dragBackground->border, OS_DRAW_MODE_REPEAT_FIRST, control->current3 == 15 ? 0xFF : 0xF * control->current3);
				}

				if (control->disabledBackground && control->current4) {
					OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->disabledBackground->region, 
							control->disabledBackground->border, OS_DRAW_MODE_REPEAT_FIRST, control->current4 == 15 ? 0xFF : 0xF * control->current4);
				}

				if (control->icon) {
					OSRectangle bounds = control->bounds;
					bounds.right = bounds.left + control->icon->region.right - control->icon->region.left;
					bounds.top += (bounds.bottom - bounds.top) / 2 - (control->icon->region.bottom - control->icon->region.top) / 2;
					bounds.bottom = bounds.top + control->icon->region.bottom - control->icon->region.top;

					if (control->current1) {
						OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->icon->region, 
								control->icon->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF);
					}

					if (control->current2) {
						OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->hoverIcon->region, 
								control->hoverIcon->border, OS_DRAW_MODE_REPEAT_FIRST, control->current2 == 15 ? 0xFF : 0xF * control->current2);
					}

					if (control->current3) {
						OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->dragIcon->region, 
								control->dragIcon->border, OS_DRAW_MODE_REPEAT_FIRST, control->current3 == 15 ? 0xFF : 0xF * control->current3);
					}

					if (control->current4) {
						OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->disabledIcon->region, 
								control->disabledIcon->border, OS_DRAW_MODE_REPEAT_FIRST, control->current4 == 15 ? 0xFF : 0xF * control->current4);
					}
				}

				uint32_t textColor = control->textColor;
				uint32_t textShadowColor = 0xFFFFFF - textColor;

				if (control->disabled && !control->noDisabledTextColorChange) {
					textColor = 0x777777;
					textShadowColor = 0xEEEEEE;
				}

				if (!control->customTextRendering) {
					if (control->textShadow || control->disabled) {
						OSRectangle bounds = control->textBounds;
						bounds.top++; bounds.bottom++; bounds.left++; bounds.right++;

						OSDrawString(message->paint.surface, bounds, &control->text, control->textSize,
								control->textAlign, textShadowColor, -1, control->textBold);
					}

					OSDrawString(message->paint.surface, control->textBounds, &control->text, control->textSize,
							control->textAlign, textColor, -1, control->textBold);
				}

				{
					OSMessage m = *message;
					m.type = OS_MESSAGE_CUSTOM_PAINT;
					OSSendMessage(control, &m);
				}

				control->repaint = false;
			}
		} break;

		case OS_MESSAGE_PARENT_UPDATED: {
			control->window = (Window *) message->parentUpdated.window;
			control->animationStep = 16;
			control->finalAnimationStep = 16;
			OSRepaintControl(control);
		} break;

		case OS_MESSAGE_DESTROY: {
			if (control->timerControlItem.list) {
				control->window->timerControls.Remove(&control->timerControlItem);
			}

			if (control->window->hover == control) control->window->hover = nullptr;
			if (control->window->drag == control) control->window->drag = nullptr;
			if (control->window->focus == control) control->window->focus = nullptr;
			if (control->window->lastFocus == control) control->window->lastFocus = nullptr;

			OSHeapFree(control->text.buffer);
			OSHeapFree(control);
		} break;

		case OS_MESSAGE_HIT_TEST: {
			message->hitTest.result = IsPointInRectangle(control->bounds, message->hitTest.positionX, message->hitTest.positionY);
		} break;

		case OS_MESSAGE_START_HOVER:
		case OS_MESSAGE_END_HOVER:
		case OS_MESSAGE_START_FOCUS:
		case OS_MESSAGE_END_FOCUS:
		case OS_MESSAGE_END_LAST_FOCUS:
		case OS_MESSAGE_START_DRAG:
		case OS_MESSAGE_END_DRAG: {
			OSMessage m;
			OSAnimateControl(control, message->type == OS_MESSAGE_START_DRAG || message->type == OS_MESSAGE_START_FOCUS);

			if (message->type == OS_MESSAGE_START_DRAG) {
				if (control->window->focus) {
					m.type = OS_MESSAGE_END_FOCUS;
					OSSendMessage(control->window->focus, &m);
					control->window->focus = nullptr;
				}

				if (control->focusable) {
					if (control->window->lastFocus) {
						m.type = OS_MESSAGE_END_LAST_FOCUS;
						OSSendMessage(control->window->lastFocus, &m);
					}

					control->window->focus = control;
					control->window->lastFocus = control;
					m.type = OS_MESSAGE_START_FOCUS;
					OSSendMessage(control, &m);
				}
			}
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (!IsPointInRectangle(control->bounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				break;
			}

			if (!control->disabled || control->keepCustomCursorWhenDisabled) {
				control->window->cursor = control->cursor;
			}

			control->window->hover = control;

			{
				OSMessage message;
				message.type = OS_MESSAGE_START_HOVER;
				OSSendMessage(control, &message);
			}
		} break;

		case OS_MESSAGE_WM_TIMER: {
			if (control->animationStep == control->finalAnimationStep) {
				control->window->timerControls.Remove(&control->timerControlItem);
			} else {
				control->animationStep++;
				OSRepaintControl(control);
			}
		} break;

		case OS_MESSAGE_CLICKED: {
			if (control->action) {
				OSMessage message;
				message.type = OS_NOTIFICATION_ACTION;
				OSForwardMessage(control, control->action->callback, &message);

				if (control->action->checkable) {
					control->action->isChecked = !control->action->isChecked;
					UpdateCheckboxIcons(control);
				}
			}
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
	string->characters = 0;
	OSCopyMemory(string->buffer, text, textBytes);

	char *m = string->buffer;

	while (m < string->buffer + textBytes) {
		m = utf8_advance(m);
		string->characters++;
	}
}

static OSCallbackResponse ProcessWindowResizeHandleMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	WindowResizeControl *control = (WindowResizeControl *) _object;

	if (message->type == OS_MESSAGE_MOUSE_DRAGGED) {
		Window *window = control->window;
		OSRectangle bounds, bounds2;
		OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, window->window, (uintptr_t) &bounds, 0, 0);

		bounds2 = bounds;

		int oldWidth = bounds.right - bounds.left;
		int oldHeight = bounds.bottom - bounds.top;

		if (control->direction & RESIZE_LEFT) bounds.left = message->mouseMoved.newPositionXScreen;
		if (control->direction & RESIZE_RIGHT) bounds.right = message->mouseMoved.newPositionXScreen;
		if (control->direction & RESIZE_TOP) bounds.top = message->mouseMoved.newPositionYScreen;
		if (control->direction & RESIZE_BOTTOM) bounds.bottom = message->mouseMoved.newPositionYScreen;

		int newWidth = bounds.right - bounds.left;
		int newHeight = bounds.bottom - bounds.top;

		if (newWidth < window->minimumWidth && control->direction & RESIZE_LEFT) bounds.left = bounds.right - window->minimumWidth;
		if (newWidth < window->minimumWidth && control->direction & RESIZE_RIGHT) bounds.right = bounds.left + window->minimumWidth;
		if (newHeight < window->minimumHeight && control->direction & RESIZE_TOP) bounds.top = bounds.bottom - window->minimumHeight;
		if (newHeight < window->minimumHeight && control->direction & RESIZE_BOTTOM) bounds.bottom = bounds.top + window->minimumHeight;

		if (control->direction == RESIZE_MOVE) {
			bounds.left = message->mouseDragged.newPositionXScreen - message->mouseDragged.originalPositionX;
			bounds.top = message->mouseDragged.newPositionYScreen - message->mouseDragged.originalPositionY;
			bounds.right = bounds.left + oldWidth;
			bounds.bottom = bounds.top + oldHeight;
		}
		
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
	} else if (message->type == OS_MESSAGE_LAYOUT_TEXT) {
		control->textBounds = control->bounds;
		control->textBounds.top -= 3;
		control->textBounds.bottom -= 3;
	} else {
		response = OSForwardMessage(_object, OSCallback(ProcessControlMessage, nullptr), message);
	}

	return response;
}

static OSObject CreateWindowResizeHandle(UIImage *image, UIImage *disabledImage, unsigned direction) {
	WindowResizeControl *control = (WindowResizeControl *) OSHeapAllocate(sizeof(WindowResizeControl), true);
	control->type = API_OBJECT_CONTROL;
	control->background = image;
	control->hoverBackground = image;
	control->dragBackground = image;
	control->disabledBackground = disabledImage;
	control->preferredWidth = image->region.right - image->region.left;
	control->preferredHeight = image->region.bottom - image->region.top;
	control->direction = direction;
	control->backgroundColor = STANDARD_BACKGROUND_COLOR;
	control->noAnimations = true;
	control->noDisabledTextColorChange = true;
	control->keepCustomCursorWhenDisabled = true;
	OSSetCallback(control, OSCallback(ProcessWindowResizeHandleMessage, nullptr));

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

		case RESIZE_MOVE: {
			control->textColor = 0xFFFFFF;
			control->textShadow = true;
			control->textBold = true;
			control->textSize = 11;
		} break;
	}

	return control;
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

	// TODO Treat escape-sequence-likes in the textbox as words?
	return CHARACTER_OTHER;
}

static void MoveCaret(OSString *string, OSCaret *caret, bool right, bool word, bool strongWhitespace = false) {
	if (!string->bytes) return;

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

static void FindCaret(Textbox *control, int positionX, int positionY, bool secondCaret, unsigned clickChainCount) {
	if (clickChainCount >= 3) {
		control->caret.byte = 0;
		control->caret.character = 0;
		control->caret2.byte = control->text.bytes;
		control->caret2.character = control->text.characters;
	} else {
		OSFindCharacterAtCoordinate(control->textBounds, OSPoint(positionX, positionY), 
				&control->text, control->textAlign, &control->caret2, control->textSize);

		if (!secondCaret) {
			control->caret = control->caret2;

			if (clickChainCount == 2) {
				MoveCaret(&control->text, &control->caret, false, true, true);
				MoveCaret(&control->text, &control->caret2, true, true, true);
				control->wordSelectionAnchor  = control->caret;
				control->wordSelectionAnchor2 = control->caret2;
			}
		} else {
			if (clickChainCount == 2) {
				if (control->caret2.byte < control->caret.byte) {
					MoveCaret(&control->text, &control->caret2, false, true);
					control->caret = control->wordSelectionAnchor2;
				} else {
					MoveCaret(&control->text, &control->caret2, true, true);
					control->caret = control->wordSelectionAnchor;
				}
			}
		}
	}

	control->window->caretBlinkPause = 2;
}

static void RemoveSelectedText(Textbox *control) {
	if (control->caret.byte == control->caret2.byte) return;
	if (control->caret.byte < control->caret2.byte) {
		OSCaret temp = control->caret2;
		control->caret2 = control->caret;
		control->caret = temp;
	}
	int bytes = control->caret.byte - control->caret2.byte;
	OSCopyMemory(control->text.buffer + control->caret2.byte, control->text.buffer + control->caret.byte, control->text.bytes - control->caret.byte);
	control->text.characters -= control->caret.character - control->caret2.character;
	control->text.bytes -= bytes;
	control->caret = control->caret2;
}

OSCallbackResponse ProcessTextboxMessage(OSObject object, OSMessage *message) {
	Textbox *control = (Textbox *) object;
	OSCallbackResponse result = OS_CALLBACK_NOT_HANDLED;
	static int lastClickChainCount = 1;

	if (message->type == OS_MESSAGE_CUSTOM_PAINT) {
		DrawString(message->paint.surface, control->textBounds, 
				&control->text, control->textAlign, control->textColor, 0xFFFFFF, control->window->focus == control ? 0xFFC4D9F9 : 0xFFDDDDDD,
				{0, 0}, nullptr, control->caret.character, control->window->lastFocus == control ? control->caret2.character : control->caret.character, 
				control->window->focus != control || control->caretBlink,
				control->textSize, fontRegular);

		result = OS_CALLBACK_HANDLED;
	} else if (message->type == OS_MESSAGE_LAYOUT_TEXT) {
		control->textBounds = control->bounds;
		control->textBounds.left += 6;
		control->textBounds.right -= 6;
		result = OS_CALLBACK_HANDLED;
	} else if (message->type == OS_MESSAGE_CARET_BLINK) {
		control->caretBlink = !control->caretBlink;
		result = OS_CALLBACK_HANDLED;
		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_START_FOCUS) {
		control->caretBlink = false;
		control->window->caretBlinkPause = 2;
	} else if (message->type == OS_MESSAGE_START_DRAG) {
		FindCaret(control, message->mousePressed.positionX, message->mousePressed.positionY, false, message->mousePressed.clickChainCount);
		lastClickChainCount = message->mousePressed.clickChainCount;
		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_MOUSE_DRAGGED) {
		FindCaret(control, message->mouseDragged.newPositionX, message->mouseDragged.newPositionY, true, lastClickChainCount);
		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_KEY_TYPED) {
		control->caretBlink = false;
		control->window->caretBlinkPause = 2;

		int ic = -1, isc = -1;

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
			case OS_SCANCODE_SPACE: 	ic = ' ';  isc = ' '; break;

			case OS_SCANCODE_BACKSPACE: {
				if (control->caret.byte == control->caret2.byte && control->caret.byte) {
					MoveCaret(&control->text, &control->caret2, false, message->keyboard.ctrl);
					int bytes = control->caret.byte - control->caret2.byte;
					OSCopyMemory(control->text.buffer + control->caret2.byte, control->text.buffer + control->caret.byte, control->text.bytes - control->caret.byte);
					control->text.characters -= 1;
					control->text.bytes -= bytes;
					control->caret = control->caret2;
				} else {
					RemoveSelectedText(control);
				}
			} break;

			case OS_SCANCODE_DELETE: {
				if (control->caret.byte == control->caret2.byte && control->caret.byte != control->text.bytes) {
					MoveCaret(&control->text, &control->caret, true, message->keyboard.ctrl);
					int bytes = control->caret.byte - control->caret2.byte;
					OSCopyMemory(control->text.buffer + control->caret2.byte, control->text.buffer + control->caret.byte, control->text.bytes - control->caret.byte);
					control->text.characters -= 1;
					control->text.bytes -= bytes;
					control->caret = control->caret2;
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
						control->caret = control->caret2;
					}
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
						control->caret = control->caret2;
					}
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

		if (message->keyboard.ctrl && !message->keyboard.alt && !message->keyboard.shift) {
			if (message->keyboard.scancode == OS_SCANCODE_A) {
				control->caret.byte = 0;
				control->caret.character = 0;

				control->caret2.byte = control->text.bytes;
				control->caret2.character = control->text.characters;
			}
		}

		if (ic != -1 && !message->keyboard.alt && !message->keyboard.ctrl) {
			RemoveSelectedText(control);

			char data[4];
			int bytes = utf8_encode(message->keyboard.shift ? isc : ic, data);
			char *old = control->text.buffer;
			control->text.buffer = (char *) OSHeapAllocate(control->text.bytes + bytes, false);
			OSCopyMemory(control->text.buffer, old, control->caret.byte);
			OSCopyMemory(control->text.buffer + control->caret.byte + bytes, old + control->caret.byte, control->text.bytes - control->caret.byte);
			OSHeapFree(old);
			OSCopyMemory(control->text.buffer + control->caret.byte, data, bytes);
			control->text.characters += 1;
			control->caret.character++;
			control->caret.byte += bytes;
			control->text.bytes += bytes;
			control->caret2 = control->caret;
		}

		OSRepaintControl(control);
	}

	if (result == OS_CALLBACK_NOT_HANDLED) {
		result = ProcessControlMessage(object, message);
	}

	return result;
}

OSObject OSCreateTextbox() {
	Textbox *control = (Textbox *) OSHeapAllocate(sizeof(Textbox), true);
	control->type = API_OBJECT_CONTROL;

	control->preferredWidth = 160;
	control->preferredHeight = 23;

	control->drawParentBackground = true;

	control->background = &textboxNormal;
	control->disabledBackground = &textboxDisabled;
	control->hoverBackground = &textboxHover;
	control->dragBackground = &textboxFocus;

	control->cursor = OS_CURSOR_TEXT;
	control->focusable = true;

	control->customTextRendering = true;
	control->textAlign = OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT;
	control->textSize = FONT_SIZE;
	control->textColor = 0x000000;

	OSSetCallback(control, OSCallback(ProcessTextboxMessage, nullptr));

	return control;
}

OSObject OSCreateButton(OSAction *action) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;

	control->preferredWidth = 80;
	control->preferredHeight = 21;

	control->action = action;
	control->drawParentBackground = true;

	if (action->checkable) {
		UpdateCheckboxIcons(control);
		control->textAlign = OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT;
		control->minimumHeight = control->icon->region.bottom - control->icon->region.top;
	} else {
		control->background = &buttonNormal;
		control->disabledBackground = &buttonDisabled;
		control->hoverBackground = &buttonHover;
		control->dragBackground = &buttonDragged;
		control->minimumWidth = 80;
		control->minimumHeight = 21;
	}

	OSSetCallback(control, OSCallback(ProcessControlMessage, nullptr));
	OSSetText(control, action->label, action->labelBytes);

	return control;
}

OSObject OSCreateLabel(char *text, size_t textBytes) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;
	control->backgroundColor = STANDARD_BACKGROUND_COLOR;

	OSSetText(control, text, textBytes);
	OSSetCallback(control, OSCallback(ProcessControlMessage, nullptr));

	return control;
}

static OSCallbackResponse ProcessProgressBarMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	ProgressBar *control = (ProgressBar *) _object;

	if (message->type == OS_MESSAGE_PAINT) {
		if (control->repaint || message->paint.force) {
			{
				OSMessage m = *message;
				m.type = OS_MESSAGE_PAINT_BACKGROUND;
				m.paintBackground.surface = message->paint.surface;
				m.paintBackground.left = control->bounds.left;
				m.paintBackground.right = control->bounds.right;
				m.paintBackground.top = control->bounds.top;
				m.paintBackground.bottom = control->bounds.bottom;
				OSSendMessage(control->parent, &m);
			}

			if (control->disabled) {
				OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, 
						control->disabledBackground->region, control->disabledBackground->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF);
			} else {
				OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, 
						control->background->region, control->background->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF);

				int pelletCount = (control->bounds.right - control->bounds.left - 6) / 9;

				if (control->maximum) {
					float progress = (float) (control->value - control->minimum) / (float) (control->maximum - control->minimum);

					pelletCount *= progress;

					for (int i = 0; i < pelletCount; i++) {
						OSRectangle bounds = control->bounds;
						bounds.top += 3;
						bounds.bottom = bounds.top + 15;
						bounds.left += 3 + i * 9;
						bounds.right = bounds.left + 8;
						OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, bounds,
								progressBarPellet.region, progressBarPellet.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF);
					}
				} else {
					if (control->value >= pelletCount) control->value -= pelletCount;

					for (int i = 0; i < 4; i++) {
						int j = i + control->value;
						if (j >= pelletCount) j -= pelletCount;
						OSRectangle bounds = control->bounds;
						bounds.top += 3;
						bounds.bottom = bounds.top + 15;
						bounds.left += 3 + j * 9;
						bounds.right = bounds.left + 8;
						OSDrawSurface(message->paint.surface, OS_SURFACE_UI_SHEET, bounds,
								progressBarPellet.region, progressBarPellet.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF);
					}
				}
			}

			control->repaint = false;
		}
	} else if (message->type == OS_MESSAGE_PARENT_UPDATED) {
		OSForwardMessage(_object, OSCallback(ProcessControlMessage, nullptr), message);
		control->timerHz = 5;
		control->window->timerControls.InsertStart(&control->timerControlItem);
	} else if (message->type == OS_MESSAGE_WM_TIMER) {
		OSSetProgressBarValue(control, control->value + 1);
	} else {
		response = OSForwardMessage(_object, OSCallback(ProcessControlMessage, nullptr), message);
	}

	return response;
}

void OSSetProgressBarValue(OSObject _control, int newValue) {
	ProgressBar *control = (ProgressBar *) _control;
	control->value = newValue;
	OSRepaintControl(control);
}

OSObject OSCreateProgressBar(int minimum, int maximum, int initialValue) {
	ProgressBar *control = (ProgressBar *) OSHeapAllocate(sizeof(ProgressBar), true);

	control->type = API_OBJECT_CONTROL;
	control->background = &progressBarBackground;
	control->disabledBackground = &progressBarDisabled;
	control->hoverBackground = &progressBarBackground;
	control->dragBackground = &progressBarBackground;

	control->minimum = minimum;
	control->maximum = maximum;
	control->value = initialValue;

	control->preferredWidth = 168;
	control->preferredHeight = 21;

	control->drawParentBackground = true;

	if (!control->maximum) {
		// Indeterminate progress bar.
		control->timerControlItem.thisItem = control;
	}

	OSSetCallback(control, OSCallback(ProcessProgressBarMessage, nullptr));

	return control;
}

void OSSetText(OSObject _control, char *text, size_t textBytes) {
	Control *control = (Control *) _control;
	CreateString(text, textBytes, &control->text);

	int suggestedWidth = MeasureStringWidth(text, textBytes, FONT_SIZE, fontRegular) + 8;
	int suggestedHeight = FONT_SIZE + 8;

	if (control->icon) {
		suggestedWidth += control->icon->region.right - control->icon->region.left + 4;
	}

	if (suggestedWidth > control->minimumWidth) control->preferredWidth = suggestedWidth;
	if (suggestedHeight > control->minimumHeight) control->preferredHeight = suggestedHeight;

	control->relayout = true;
	OSRepaintControl(control);

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
		column = 1;
		row = 2;
		if (layout != OS_ADD_CHILD_GRID) OSCrashProcess(OS_FATAL_ERROR_INVALID_PANE_OBJECT);
		((Grid *) _control)->borderSize = 8;
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
		message.parentUpdated.window = grid->window;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(control, &message);
	}
}

static OSCallbackResponse ProcessGridMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Grid *grid = (Grid *) _object;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			if (grid->relayout || message->layout.force) {
				grid->relayout = false;

				grid->bounds = OS_MAKE_RECTANGLE(
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

				grid->repaint = true;
				SetParentDescendentInvalidationFlags(grid, DESCENDENT_REPAINT);
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
			if (grid->descendentInvalidationFlags & DESCENDENT_REPAINT || grid->repaint || message->paint.force) {
				grid->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;

				OSMessage m = *message;
				m.paint.force = message->paint.force || grid->repaint;

				if (m.paint.force) {
					OSFillRectangle(message->paint.surface, grid->bounds, OSColor(STANDARD_BACKGROUND_COLOR));
				}

				for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
					if (grid->objects[i]) {
						OSSendMessage(grid->objects[i], &m);
					}
				}

				grid->repaint = false;
			}
		} break;

		case OS_MESSAGE_PAINT_BACKGROUND: {
			OSFillRectangle(message->paint.surface, OS_MAKE_RECTANGLE(message->paintBackground.left, message->paintBackground.right, 
						message->paintBackground.top, message->paintBackground.bottom), OSColor(STANDARD_BACKGROUND_COLOR));
		} break;

		case OS_MESSAGE_PARENT_UPDATED: {
			grid->window = (Window *) message->parentUpdated.window;
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

	OSSetCallback(grid, OSCallback(ProcessGridMessage, nullptr));

	return grid;
}

void OSDisableControl(OSObject _control, bool disabled) {
	Control *control = (Control *) _control;
	control->disabled = disabled;

	OSAnimateControl(control, false);

	if (control->window->focus == control) {
		OSMessage message;
		message.type = OS_MESSAGE_END_FOCUS;
		OSSendMessage(control, &message);
		control->window->focus = nullptr;
	}
}

static OSCallbackResponse ProcessWindowMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Window *window = (Window *) _object;

	static int lastClickX = 0, lastClickY = 0;

	if (window->destroyed && message->type != OS_MESSAGE_WINDOW_DESTROYED) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	switch (message->type) {
		case OS_MESSAGE_WINDOW_CREATED: {
			window->created = true;
		} break;

		case OS_MESSAGE_WINDOW_ACTIVATED: {
			if (!window->created) break;

			for (int i = 0; i < 12; i++) {
				if (i == 7) continue;
				OSDisableControl(window->root->objects[i], false);
			}
		} break;

		case OS_MESSAGE_WINDOW_DEACTIVATED: {
			if (!window->created) break;

			for (int i = 0; i < 12; i++) {
				if (i == 7) continue;
				OSDisableControl(window->root->objects[i], true);
			}

			if (window->focus) {
				OSMessage message;
				message.type = OS_MESSAGE_END_FOCUS;
				OSSendMessage(window->focus, &message);
				window->focus = nullptr;
			}
		} break;

		case OS_MESSAGE_WINDOW_DESTROYED: {
			OSHeapFree(window);
			return response;
		} break;

		case OS_MESSAGE_KEY_PRESSED: {
			if (message->keyboard.scancode == OS_SCANCODE_F4 && message->keyboard.alt) {
				message->type = OS_MESSAGE_DESTROY;
				OSSendMessage(window, message);
			} else if (window->focus) {
				message->type = OS_MESSAGE_KEY_TYPED;
				OSSendMessage(window->focus, message);
			}
		} break;

		case OS_MESSAGE_DESTROY: {
			OSSendMessage(window->root, message);
			OSCloseHandle(window->surface);
			OSCloseHandle(window->window);
			window->destroyed = true;
		} break;

		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			if (window->hover && !window->hover->disabled) {
				window->drag = window->hover;
			}

			lastClickX = message->mousePressed.positionX;
			lastClickY = message->mousePressed.positionY;

			if (window->drag) {
				OSMessage m = *message;
				m.type = OS_MESSAGE_START_DRAG;
				OSSendMessage(window->drag, &m);
			} else if (window->focus) {
				OSMessage message;
				message.type = OS_MESSAGE_END_FOCUS;
				OSSendMessage(window->focus, &message);
				window->focus = nullptr;
			}
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			if (window->drag) {
				OSRepaintControl(window->drag);

				if (window->drag == window->hover) {
					OSMessage clicked;
					clicked.type = OS_MESSAGE_CLICKED;
					OSSendMessage(window->drag, &clicked);
				}

				OSMessage message;
				message.type = OS_MESSAGE_END_DRAG;
				OSSendMessage(window->drag, &message);
				window->drag = nullptr;
			}

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

		case OS_MESSAGE_MOUSE_EXIT: {
			if (window->hover) {
				OSMessage message;
				message.type = OS_MESSAGE_END_HOVER;
				OSSendMessage(window->hover, &message);
				window->hover = nullptr;
			}
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (window->drag) {
				OSMessage hitTest;
				hitTest.type = OS_MESSAGE_HIT_TEST;
				hitTest.hitTest.positionX = message->mouseMoved.newPositionX;
				hitTest.hitTest.positionY = message->mouseMoved.newPositionY;
				OSCallbackResponse response = OSSendMessage(window->drag, &hitTest);

				if (response == OS_CALLBACK_HANDLED && !hitTest.hitTest.result && window->hover) {
					OSMessage message;
					message.type = OS_MESSAGE_END_HOVER;
					OSSendMessage(window->hover, &message);
					window->hover = nullptr;
				} else if (response == OS_CALLBACK_HANDLED && hitTest.hitTest.result && !window->hover) {
					OSMessage message;
					message.type = OS_MESSAGE_START_HOVER;
					window->hover = window->drag;
					OSSendMessage(window->hover, &message);
				}

				message->type = OS_MESSAGE_MOUSE_DRAGGED;
				message->mouseDragged.originalPositionX = lastClickX;
				message->mouseDragged.originalPositionY = lastClickY;
				OSSendMessage(window->drag, message);
			} else {
				Control *old = window->hover;

				OSMessage hitTest;
				hitTest.type = OS_MESSAGE_HIT_TEST;
				hitTest.hitTest.positionX = message->mouseMoved.newPositionX;
				hitTest.hitTest.positionY = message->mouseMoved.newPositionY;
				OSCallbackResponse response = OSSendMessage(old, &hitTest);

				if (!hitTest.hitTest.result || response == OS_CALLBACK_NOT_HANDLED) {
					window->hover = nullptr;
					OSSendMessage(window->root, message);
				}

				if (!window->hover) {
					window->cursor = OS_CURSOR_NORMAL;
				}

				if (window->hover != old && old) {
					OSMessage message;
					message.type = OS_MESSAGE_END_HOVER;
					OSSendMessage(old, &message);
				}
			}
		} break;

		case OS_MESSAGE_WM_TIMER: {
			LinkedItem<Control> *item = window->timerControls.firstItem;

			while (item) {
				int ticksPerMessage = window->timerHz / item->thisItem->timerHz;
				item->thisItem->timerStep++;

				if (item->thisItem->timerStep >= ticksPerMessage) {
					item->thisItem->timerStep = 0;
					OSSendMessage(item->thisItem, message);
				}

				item = item->nextItem;
			}

			if (window->focus) {
				window->caretBlinkStep++;

				if (window->caretBlinkStep >= window->timerHz / CARET_BLINK_HZ) {
					window->caretBlinkStep = 0;

					if (window->caretBlinkPause) {
						window->caretBlinkPause--;
					} else {
						OSMessage message;
						message.type = OS_MESSAGE_CARET_BLINK;
						OSSendMessage(window->focus, &message);
					}
				}
			}
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	if (window->destroyed) {
		return response;
	}

	{
		// TODO Only do this if the list or focus has changed.

		LinkedItem<Control> *item = window->timerControls.firstItem;
		int largestHz = window->focus ? CARET_BLINK_HZ : 0;

		while (item) {
			int thisHz = item->thisItem->timerHz;
			if (thisHz > largestHz) largestHz = thisHz;
			item = item->nextItem;
		}

		if (window->timerHz != largestHz) {
			window->timerHz = largestHz;
			OSSyscall(OS_SYSCALL_NEED_WM_TIMER, window->window, largestHz, 0, 0);
		}
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

OSObject OSCreateWindow(OSWindowSpecification *specification) {
	specification->width += totalBorderWidth;
	specification->minimumWidth += totalBorderWidth;
	specification->height += totalBorderHeight;
	specification->minimumHeight += totalBorderHeight;

	Window *window = (Window *) OSHeapAllocate(sizeof(Window), true);
	window->type = API_OBJECT_WINDOW;

	OSRectangle bounds;
	bounds.left = 0;
	bounds.right = specification->width;
	bounds.top = 0;
	bounds.bottom = specification->height;

	OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) &window->window, (uintptr_t) &bounds, (uintptr_t) window, 0);
	OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, window->window, (uintptr_t) &bounds, 0, 0);

	window->width = bounds.right - bounds.left;
	window->height = bounds.bottom - bounds.top;
	window->flags = specification->flags;
	window->cursor = OS_CURSOR_NORMAL;
	window->minimumWidth = specification->minimumWidth;
	window->minimumHeight = specification->minimumHeight;

	OSSetCallback(window, OSCallback(ProcessWindowMessage, nullptr));

	window->root = (Grid *) OSCreateGrid(3, 4, OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_GAP);
	window->root->parent = window;

	{
		OSMessage message;
		message.parentUpdated.window = window;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(window->root, &message);
	}

	{
		OSObject titlebar = CreateWindowResizeHandle(&activeWindowBorder22, &inactiveWindowBorder22, RESIZE_MOVE);
		OSAddControl(window->root, 1, 1, titlebar, OS_CELL_H_PUSH | OS_CELL_H_EXPAND | OS_CELL_V_EXPAND);
		OSSetText(titlebar, specification->title, specification->titleBytes);

		OSAddControl(window->root, 0, 0, CreateWindowResizeHandle(&activeWindowBorder11, &inactiveWindowBorder11, RESIZE_TOP_LEFT), 0);
		OSAddControl(window->root, 1, 0, CreateWindowResizeHandle(&activeWindowBorder12, &inactiveWindowBorder12, RESIZE_TOP), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSAddControl(window->root, 2, 0, CreateWindowResizeHandle(&activeWindowBorder13, &inactiveWindowBorder13, RESIZE_TOP_RIGHT), 0);
		OSAddControl(window->root, 0, 1, CreateWindowResizeHandle(&activeWindowBorder21, &inactiveWindowBorder21, RESIZE_LEFT), 0);
		OSAddControl(window->root, 2, 1, CreateWindowResizeHandle(&activeWindowBorder23, &inactiveWindowBorder23, RESIZE_RIGHT), 0);
		OSAddControl(window->root, 0, 2, CreateWindowResizeHandle(&activeWindowBorder31, &inactiveWindowBorder31, RESIZE_LEFT), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
		OSAddControl(window->root, 2, 2, CreateWindowResizeHandle(&activeWindowBorder33, &inactiveWindowBorder33, RESIZE_RIGHT), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
		OSAddControl(window->root, 0, 3, CreateWindowResizeHandle(&activeWindowBorder41, &inactiveWindowBorder41, RESIZE_BOTTOM_LEFT), 0);
		OSAddControl(window->root, 1, 3, CreateWindowResizeHandle(&activeWindowBorder42, &inactiveWindowBorder42, RESIZE_BOTTOM), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSAddControl(window->root, 2, 3, CreateWindowResizeHandle(&activeWindowBorder43, &inactiveWindowBorder43, RESIZE_BOTTOM_RIGHT), 0);
	}

	return window;
}

void OSInitialiseGUI() {
}
