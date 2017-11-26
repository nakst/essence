#ifndef IMPLEMENTATION

#define SCANCODE_KEY_RELEASED (1 << 15)
#define SCANCODE_KEY_PRESSED  (0 << 15)

#define LEFT_BUTTON (1)
#define MIDDLE_BUTTON (2)
#define RIGHT_BUTTON (4)

struct Window {
	void Update();
	void SetCursorStyle(OSCursorStyle style);
	void Destroy();
	void Move(OSRectangle &newBounds);
	void Resize(size_t newWidth, size_t newHeight);
	void ClearImage();

	Mutex mutex;
	Surface *surface;
	OSPoint position;
	size_t width, height;
	uintptr_t z;
	Process *owner;
	OSWindow *apiWindow;
	OSCursorStyle cursorStyle;

	volatile unsigned handles;
};

struct WindowManager {
	void Initialise();
	Window *CreateWindow(Process *process, size_t width, size_t height);
	void MoveCursor(int xMovement, int yMovement);
	void ClickCursor(unsigned buttons);
	void UpdateCursor(int xMovement, int yMovement, unsigned buttons);
	void PressKey(unsigned scancode);
	void RefreshCursor(Window *window);
	void Redraw(OSPoint position, int width, int height, uint16_t below, Window *except = nullptr);

	Window **windows; // Sorted by z.
	size_t windowsCount, windowsAllocated;

	Window *focusedWindow, *pressedWindow;

	Mutex mutex;

	int cursorX, cursorY;
	int cursorImageX, cursorImageY;
	int cursorImageOffsetX, cursorImageOffsetY;
	int cursorImageWidth, cursorImageHeight;

	unsigned lastButtons;
	bool shift, alt, ctrl;

	uint64_t clickChainStartMs;
	unsigned clickChainCount;
	int clickChainX, clickChainY;
};

WindowManager windowManager;
Surface uiSheetSurface;

#else

void WindowManager::UpdateCursor(int xMovement, int yMovement, unsigned buttons) {
	if (xMovement || yMovement) {
		if (xMovement * xMovement + yMovement * yMovement < 10 && buttons != lastButtons) {
			// This seems to be movement noise generated when the buttons were pressed/released.
		} else {
			MoveCursor(xMovement, yMovement);
		}
	} 

	ClickCursor(buttons);
}

void WindowManager::RefreshCursor(Window *window) {
	mutex.AssertLocked();

	OSCursorStyle style = OS_CURSOR_NORMAL;

	if (window) {
		style = window->cursorStyle;
	}

	switch (style) {
		case OS_CURSOR_TEXT: {
			windowManager.cursorImageX = 142;
			windowManager.cursorImageY = 96;
			windowManager.cursorImageOffsetX = -3;
			windowManager.cursorImageOffsetY = -7;
			windowManager.cursorImageWidth = 12;
			windowManager.cursorImageHeight = 19;
		} break;

		case OS_CURSOR_RESIZE_VERTICAL: {
			windowManager.cursorImageX = 112;
			windowManager.cursorImageY = 97;
			windowManager.cursorImageOffsetX = -4;
			windowManager.cursorImageOffsetY = -8;
			windowManager.cursorImageWidth = 12;
			windowManager.cursorImageHeight = 19;
		} break;

		case OS_CURSOR_RESIZE_HORIZONTAL: {
			windowManager.cursorImageX = 154;
			windowManager.cursorImageY = 100;
			windowManager.cursorImageOffsetX = -8;
			windowManager.cursorImageOffsetY = -4;
			windowManager.cursorImageWidth = 17;
			windowManager.cursorImageHeight = 19;
		} break;

		case OS_CURSOR_RESIZE_DIAGONAL_1: {
			windowManager.cursorImageX = 193;
			windowManager.cursorImageY = 97;
			windowManager.cursorImageOffsetX = -6;
			windowManager.cursorImageOffsetY = -6;
			windowManager.cursorImageWidth = 17;
			windowManager.cursorImageHeight = 19;
		} break;

		case OS_CURSOR_RESIZE_DIAGONAL_2: {
			windowManager.cursorImageX = 176;
			windowManager.cursorImageY = 97;
			windowManager.cursorImageOffsetX = -6;
			windowManager.cursorImageOffsetY = -6;
			windowManager.cursorImageWidth = 17;
			windowManager.cursorImageHeight = 19;
		} break;

		case OS_CURSOR_NORMAL:
		default: {
			windowManager.cursorImageX = 125;
			windowManager.cursorImageY = 96;
			windowManager.cursorImageOffsetX = 0;
			windowManager.cursorImageOffsetY = 0;
			windowManager.cursorImageWidth = 12;
			windowManager.cursorImageHeight = 19;
		} break;
	}
}

void WindowManager::PressKey(unsigned scancode) {
	mutex.Acquire();
	Defer(mutex.Release());

	// TODO Right alt/ctrl/shift.
	// TODO Caps/num lock.
	if (scancode == OS_SCANCODE_LEFT_CTRL) ctrl = true;
	if (scancode == (OS_SCANCODE_LEFT_CTRL | SCANCODE_KEY_RELEASED)) ctrl = false;
	if (scancode == OS_SCANCODE_LEFT_SHIFT) shift = true;
	if (scancode == (OS_SCANCODE_LEFT_SHIFT | SCANCODE_KEY_RELEASED)) shift = false;
	if (scancode == OS_SCANCODE_LEFT_ALT) alt = true;
	if (scancode == (OS_SCANCODE_LEFT_ALT | SCANCODE_KEY_RELEASED)) alt = false;

	if (focusedWindow) {
		Window *window = focusedWindow;

		OSMessage message = {};
		message.type = (scancode & SCANCODE_KEY_RELEASED) ? OS_MESSAGE_KEY_RELEASED : OS_MESSAGE_KEY_PRESSED;
		message.targetWindow = window->apiWindow;
		message.keyboard.alt = alt;
		message.keyboard.ctrl = ctrl;
		message.keyboard.shift = shift;
		message.keyboard.scancode = scancode & ~SCANCODE_KEY_RELEASED;
		window->owner->SendMessage(message);
	}
}

int MeasureDistance(int x1, int y1, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	return dx * dx + dy * dy;
}

void WindowManager::ClickCursor(unsigned buttons) {
	mutex.Acquire();
	Defer(mutex.Release());

	unsigned delta = lastButtons ^ buttons;

	if (delta & LEFT_BUTTON) {
		// Send a mouse pressed message to the window the cursor is over.
		uint16_t index = graphics.frameBuffer.depthBuffer[graphics.frameBuffer.resX * cursorY + cursorX];
		Window *window;

		if (index) {
			window = windows[index - 1];
		} else {
			window = nullptr;

			// The cursor is not in a window.
		}

		if (buttons & LEFT_BUTTON) {
#define CLICK_CHAIN_TIMEOUT (500)
			if (clickChainStartMs + CLICK_CHAIN_TIMEOUT < scheduler.timeMs
					|| MeasureDistance(cursorX, cursorY, clickChainX, clickChainY) >= 5) {
				// Start a new click chain.
				clickChainStartMs = scheduler.timeMs;
				clickChainCount = 1;
				clickChainX = cursorX;
				clickChainY = cursorY;
			} else {
				clickChainStartMs = scheduler.timeMs;
				clickChainCount++;
			}
		}

		{
			OSMessage message = {};
			message.type = (buttons & LEFT_BUTTON) ? OS_MESSAGE_MOUSE_LEFT_PRESSED : OS_MESSAGE_MOUSE_LEFT_RELEASED;
			Print("message.type = %d\n", message.type);

			if (message.type == OS_MESSAGE_MOUSE_LEFT_PRESSED) {
				pressedWindow = window;
				focusedWindow = window;
			} else if (message.type == OS_MESSAGE_MOUSE_LEFT_RELEASED) {
				if (pressedWindow) {
					// Always send the messages to the pressed window, if there is one.
					window = pressedWindow;
				}

				pressedWindow = nullptr;
			}
			
			if (window) {
				message.mousePressed.positionX = cursorX - window->position.x;
				message.mousePressed.positionY = cursorY - window->position.y;
				message.mousePressed.positionXScreen = cursorX;
				message.mousePressed.positionYScreen = cursorY;
				message.mousePressed.clickChainCount = clickChainCount;
				message.targetWindow = window->apiWindow;

				RefreshCursor(window);
				window->owner->SendMessage(message);
			} else {
				RefreshCursor(nullptr);
			}
		}
	}

	lastButtons = buttons;
}

void WindowManager::MoveCursor(int xMovement, int yMovement) {
	mutex.Acquire();

	int oldCursorX = cursorX;
	int oldCursorY = cursorY;
	int _cursorX = oldCursorX + xMovement;
	int _cursorY = oldCursorY + yMovement;

	if (_cursorX < 0) {
		_cursorX = 0;
	}

	if (_cursorY < 0) {
		_cursorY = 0;
	}

	if (_cursorX >= (int) graphics.resX) {
		_cursorX = graphics.resX - 1;
	}

	if (_cursorY >= (int) graphics.resY) {
		_cursorY = graphics.resY - 1;
	}

	cursorX = _cursorX;
	cursorY = _cursorY;

	// Work out which window the mouse is now over.
	uint16_t index = graphics.frameBuffer.depthBuffer[graphics.frameBuffer.resX * cursorY + cursorX];

	Window *window = pressedWindow ? pressedWindow : (index ? windows[index - 1] : nullptr);

	if (window) {
		OSMessage message = {};
		message.type = OS_MESSAGE_MOUSE_MOVED;
		message.targetWindow = window->apiWindow;
		message.mouseMoved.newPositionX = cursorX - window->position.x;
		message.mouseMoved.newPositionY = cursorY - window->position.y;
		message.mouseMoved.newPositionXScreen = cursorX;
		message.mouseMoved.newPositionYScreen = cursorY;
		message.mouseMoved.oldPositionX = oldCursorX - window->position.x;
		message.mouseMoved.oldPositionY = oldCursorY - window->position.y;
		window->owner->SendMessage(message);

	}

	RefreshCursor(window);
	mutex.Release();
	graphics.UpdateScreen();
}

void CaretBlink(WindowManager *windowManager) {
	Timer timer = {};

	while (true) {
		timer.Set(800, true);
		timer.event.Wait(OS_WAIT_NO_TIMEOUT);
		timer.Remove();

		windowManager->mutex.Acquire();
		Window *window = windowManager->focusedWindow;

		if (window) {
			OSMessage message = {};
			message.type = OS_MESSAGE_WINDOW_BLINK_TIMER;
			message.targetWindow = window->apiWindow;
			window->owner->SendMessage(message);
		}

		windowManager->mutex.Release();
	}
}

void WindowManager::Initialise() {
	mutex.Acquire();

	uiSheetSurface.Initialise(kernelProcess->vmm, 256, 256, false);

	cursorX = graphics.resX / 2;
	cursorY = graphics.resY / 2;

	RefreshCursor(nullptr);

	mutex.Release();

	// Draw the background.
	graphics.frameBuffer.FillRectangle(OSRectangle(0, graphics.resX, 0, graphics.resY), OSColor(83, 114, 166));
	graphics.UpdateScreen();

	// Create the caret blink thread.
	scheduler.SpawnThread((uintptr_t) CaretBlink, (uintptr_t) this, kernelProcess, false);
}

Window *WindowManager::CreateWindow(Process *process, size_t width, size_t height) {
	mutex.Acquire();
	Defer(mutex.Release());

	static int cx = 20, cy = 20;

	Window *window = (Window *) OSHeapAllocate(sizeof(Window), true);
	window->surface = (Surface *) OSHeapAllocate(sizeof(Surface), true);

	if (!window->surface->Initialise(process->vmm, width, height, false)) {
		OSHeapFree(window->surface, sizeof(Surface));
		OSHeapFree(window, sizeof(Window));
		return nullptr;
	}

	window->surface->handles++;
	window->position = OSPoint(cx += 20, cy += 20);
	window->width = width;
	window->height = height;
	window->owner = process;
	window->handles = 1;

	window->z = windowsCount;
	ArrayAdd(windows, window);

	return window;
}

void Window::ClearImage() {
	windowManager.mutex.Acquire();
	Defer(windowManager.mutex.Release());

	{
		graphics.frameBuffer.mutex.Acquire();
		Defer(graphics.frameBuffer.mutex.Release());

		uint16_t thisWindowDepth = z + 1;

		for (int y = position.y; y < position.y + (int) height; y++) {
			for (int x = position.x; x < position.x + (int) width; x++) {
				if (x < 0 || x >= (int) graphics.frameBuffer.resX
						|| y < 0 || y >= (int) graphics.frameBuffer.resY) {
					continue;
				}

				uint16_t *depth = graphics.frameBuffer.depthBuffer + (graphics.frameBuffer.resX * y + x);

				if (*depth == thisWindowDepth) {
					*depth = 0;
				}
			}
		}
	}

	windowManager.Redraw(position, width, height, z, this);
}

void Window::Move(OSRectangle &rectangle) {
	mutex.Acquire();
	Defer(mutex.Release());

	size_t newWidth = rectangle.right - rectangle.left;
	size_t newHeight = rectangle.bottom - rectangle.top;

	if (newWidth < 128 || newHeight < 64 
			|| rectangle.left > rectangle.right 
			|| rectangle.top > rectangle.bottom) return;

	ClearImage();
	position = OSPoint(rectangle.left, rectangle.top);

	size_t oldWidth = width;
	size_t oldHeight = height;
	width = newWidth;
	height = newHeight;

	if (oldWidth != width || oldHeight != height) {
		surface->Resize(width, height);
	}

	surface->InvalidateRectangle(OSRectangle(0, width, 0, height));
}

void Window::Destroy() {
	{
		windowManager.mutex.Acquire();
		Defer(windowManager.mutex.Release());

		if (windowManager.focusedWindow == this) windowManager.focusedWindow = nullptr;
		if (windowManager.pressedWindow == this) windowManager.pressedWindow = nullptr;

		{
			graphics.frameBuffer.mutex.Acquire();
			Defer(graphics.frameBuffer.mutex.Release());

			uint16_t thisWindowDepth = z + 1;

			for (uintptr_t y = 0; y < graphics.frameBuffer.resY; y++) {
				for (uintptr_t x = 0; x < graphics.frameBuffer.resX; x++) {
					uint16_t *depth = graphics.frameBuffer.depthBuffer + (graphics.frameBuffer.resX * y + x);

					if (*depth > thisWindowDepth) {
						*depth = *depth - 1;
					} else if (*depth == thisWindowDepth) {
						*depth = 0;
					}
				}
			}
		}

		CopyMemory(windowManager.windows + z, windowManager.windows + z + 1, sizeof(Window *) * (windowManager.windowsCount - z - 1));
		windowManager.windowsCount--;

		for (uintptr_t index = z; index < windowManager.windowsCount; index++) {
			windowManager.windows[index]->z--;
		}

		windowManager.Redraw(position, width, height, z);
		CloseHandleToObject(surface, KERNEL_OBJECT_SURFACE);
		OSHeapFree(this, sizeof(Window));
	}

	graphics.UpdateScreen();
}

void WindowManager::Redraw(OSPoint position, int width, int height, uint16_t below, Window *except) {
	mutex.AssertLocked();

	{
		OSRectangle background = {position.x, position.x + width, position.y, position.y + height};

		if (background.left < 0) background.left = 0;
		if (background.top < 0) background.top = 0;
		if (background.right > (int) graphics.frameBuffer.resX) background.right = graphics.frameBuffer.resX;
		if (background.bottom > (int) graphics.frameBuffer.resY) background.bottom = graphics.frameBuffer.resY;

		graphics.frameBuffer.FillRectangle(background, OSColor(83, 114, 166));
	}

	for (int index = below - 1; index >= 0; index--) {
		Window *window = windows[index];

		if (window == except) {
			continue;
		}

		if (position.x > window->position.x + (int) window->width
				|| position.x + width < window->position.x
				|| position.y > window->position.y + (int) window->height
				|| position.y + height < window->position.y) {
			continue;
		}

		OSRectangle rectangle = {0, (int) window->width, 0, (int) window->height};

		if (position.x > window->position.x) {
			rectangle.left = position.x - window->position.x;
		}

		if (position.y > window->position.y) {
			rectangle.top = position.y - window->position.y;
		}

		if (position.x + width < window->position.x + (int) window->width) {
			rectangle.right = window->position.x + (int) window->width - position.x - width;
		}

		if (position.y + height < window->position.y + (int) window->height) {
			rectangle.bottom = window->position.y + (int) window->height - position.y - height;
		}

		window->surface->InvalidateRectangle(rectangle);
		graphics.frameBuffer.Copy(*window->surface, window->position, OSRectangle(0, window->width, 0, window->height), true, window->z + 1);

		window->surface->mutex.Acquire();
		window->surface->ClearModifiedRegion();
		window->surface->mutex.Release();
	}
}

void Window::Update() {
	mutex.AssertLocked();

	graphics.frameBuffer.Copy(*surface, position, OSRectangle(0, width, 0, height), true, z + 1);
	graphics.UpdateScreen();

	surface->mutex.Acquire();
	surface->ClearModifiedRegion();
	surface->mutex.Release();
}

void Window::SetCursorStyle(OSCursorStyle style) {
	mutex.Acquire();
	Defer(mutex.Release());

	windowManager.mutex.Acquire();
	cursorStyle = style;
	windowManager.RefreshCursor(this);
	windowManager.mutex.Release();
}

#endif
