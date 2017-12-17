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

	Mutex mutex; // Mutex for drawing to the window. Also needed when moving the window.
	Surface *surface;
	OSPoint position;
	size_t width, height;
	uintptr_t z;
	Process *owner;
	OSObject apiWindow;
	OSCursorStyle cursorStyle;

	volatile unsigned handles;
};

struct WindowManager {
	void Initialise();
	Window *CreateWindow(Process *process, OSRectangle bounds, OSObject apiWindow);
	void MoveCursor(int xMovement, int yMovement);
	void ClickCursor(unsigned buttons);
	void UpdateCursor(int xMovement, int yMovement, unsigned buttons);
	void PressKey(unsigned scancode);
	void RefreshCursor(Window *window);
	void Redraw(OSPoint position, int width, int height, Window *except = nullptr);
	void SetActiveWindow(Window *window);

	Window **windows; // Sorted by z.
	size_t windowsCount, windowsAllocated;

	Window *pressedWindow, *activeWindow, *hoverWindow;

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
Surface uiSheetSurface, wallpaperSurface;

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

	if (activeWindow) {
		Window *window = activeWindow;

		OSMessage message = {};
		message.type = (scancode & SCANCODE_KEY_RELEASED) ? OS_MESSAGE_KEY_RELEASED : OS_MESSAGE_KEY_PRESSED;
		message.targetWindow = window->apiWindow;
		message.keyboard.alt = alt;
		message.keyboard.ctrl = ctrl;
		message.keyboard.shift = shift;
		message.keyboard.scancode = scancode & ~SCANCODE_KEY_RELEASED;
		window->owner->messageQueue.SendMessage(message);
	}
}

int MeasureDistance(int x1, int y1, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	return dx * dx + dy * dy;
}

void WindowManager::SetActiveWindow(Window *window) {
	mutex.AssertLocked();

	KernelLog(LOG_VERBOSE, "SetActiveWindow - from %x to %x\n", activeWindow, window);

	if (activeWindow == window) {
		return;
	}

	Window *oldActiveWindow = activeWindow;
	activeWindow = window;

	if (oldActiveWindow) {
		OSMessage message = {};
		message.type = OS_MESSAGE_WINDOW_DEACTIVATED;
		message.targetWindow = oldActiveWindow->apiWindow;

		if (window && window->owner == oldActiveWindow->owner) {
			message.windowDeactivated.newWindow = window->apiWindow;
			message.windowDeactivated.positionX = cursorX - window->position.x;
			message.windowDeactivated.positionY = cursorY - window->position.y;
		} else {
			message.windowDeactivated.positionX = -1;
			message.windowDeactivated.positionY = -1;
		}

		oldActiveWindow->owner->messageQueue.SendMessage(message);
	}

	if (!window) {
		return;
	}

	{
		graphics.frameBuffer.mutex.Acquire();
		Defer(graphics.frameBuffer.mutex.Release());

		uint16_t thisWindowDepth = window->z + 1;

		for (uintptr_t y = 0; y < graphics.frameBuffer.resY; y++) {
			for (uintptr_t x = 0; x < graphics.frameBuffer.resX; x++) {
				uint16_t *depth = graphics.frameBuffer.depthBuffer + (graphics.frameBuffer.resX * y + x);

				if (*depth > thisWindowDepth) {
					*depth = *depth - 1;
				} else if (*depth == thisWindowDepth) {
					*depth = windowManager.windowsCount;
				}
			}
		}
	}

	CopyMemory(windowManager.windows + window->z, windowManager.windows + window->z + 1, sizeof(Window *) * (windowManager.windowsCount - window->z - 1));

	for (uintptr_t index = window->z; index < windowManager.windowsCount - 1; index++) {
		windowManager.windows[index]->z--;
	}

	window->z = windowManager.windowsCount - 1;
	windowManager.windows[window->z] = window;

	windowManager.Redraw(window->position, window->width, window->height);

	if (window->apiWindow) {
		OSMessage message = {};
		message.type = OS_MESSAGE_WINDOW_ACTIVATED;
		message.targetWindow = window->apiWindow;
		window->owner->messageQueue.SendMessage(message);
	}

	// TODO Prevent activations clicks interacting with controls in content pane?
}

void WindowManager::ClickCursor(unsigned buttons) {
	mutex.Acquire();

	unsigned delta = lastButtons ^ buttons;
	lastButtons = buttons;

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
			SetActiveWindow(window);

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

			if (message.type == OS_MESSAGE_MOUSE_LEFT_PRESSED) {
				pressedWindow = window;
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
				window->owner->messageQueue.SendMessage(message);
			} else {
				RefreshCursor(nullptr);
			}
		}
	}

	mutex.Release();
	graphics.UpdateScreen();
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

	if (hoverWindow && hoverWindow != window) {
		OSMessage message = {};
		message.type = OS_MESSAGE_MOUSE_EXIT;
		message.targetWindow = hoverWindow->apiWindow;
		hoverWindow->owner->messageQueue.SendMessage(message);
	}

	hoverWindow = window;

	if (hoverWindow != window && window) {
		OSMessage message = {};
		message.type = OS_MESSAGE_MOUSE_ENTER;
		message.mouseEntered.positionX = cursorX - window->position.x;
		message.mouseEntered.positionY = cursorY - window->position.y;
		message.mouseEntered.positionXScreen = cursorX;
		message.mouseEntered.positionYScreen = cursorY;
		message.targetWindow = hoverWindow->apiWindow;
		hoverWindow->owner->messageQueue.SendMessage(message);
	}

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
		window->owner->messageQueue.SendMessage(message);
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
		Window *window = windowManager->activeWindow;

		if (window) {
			OSMessage message = {};
			message.type = OS_MESSAGE_WINDOW_BLINK_TIMER;
			message.targetWindow = window->apiWindow;
			window->owner->messageQueue.SendMessage(message);
		}

		windowManager->mutex.Release();
	}
}

void WindowManager::Initialise() {
	mutex.Acquire();

	uiSheetSurface.Initialise(256, 256, false);
	wallpaperSurface.Initialise(graphics.resX, graphics.resY, false);

	cursorX = graphics.resX / 2;
	cursorY = graphics.resY / 2;

	RefreshCursor(nullptr);

	// Draw the background.
	Redraw(OSPoint(0, 0), graphics.resX, graphics.resY);
	mutex.Release();
	graphics.UpdateScreen();

	// Create the caret blink thread.
	scheduler.SpawnThread((uintptr_t) CaretBlink, (uintptr_t) this, kernelProcess, false);
}

Window *WindowManager::CreateWindow(Process *process, OSRectangle bounds, OSObject apiWindow) {
	mutex.Acquire();
	Defer(mutex.Release());

	static int cx = 20, cy = 20;

	Window *window = (Window *) OSHeapAllocate(sizeof(Window), true);
	window->surface = (Surface *) OSHeapAllocate(sizeof(Surface), true);
	window->apiWindow = apiWindow;

	size_t width = bounds.right - bounds.left;
	size_t height = bounds.bottom - bounds.top;

	if (!window->surface->Initialise(width, height, false)) {
		OSHeapFree(window->surface, sizeof(Surface));
		OSHeapFree(window, sizeof(Window));
		return nullptr;
	}

	window->surface->handles++;
	window->position = bounds.left == 0 && bounds.top == 0 ? OSPoint(cx += 20, cy += 20) : OSPoint(bounds.left, bounds.top);
	window->width = width;
	window->height = height;
	window->owner = process;
	window->handles = 1;

	KernelLog(LOG_VERBOSE, "Created window %x, handles = %d\n", window, window->handles);

	window->z = windowsCount;
	ArrayAdd(windows, window);
	SetActiveWindow(window);

	return window;
}

void Window::ClearImage() {
	windowManager.mutex.AssertLocked();

	{
		graphics.frameBuffer.mutex.Acquire();
		Defer(graphics.frameBuffer.mutex.Release());

		for (int y = position.y; y < position.y + (int) height; y++) {
			for (int x = position.x; x < position.x + (int) width; x++) {
				if (x < 0 || x >= (int) graphics.frameBuffer.resX
						|| y < 0 || y >= (int) graphics.frameBuffer.resY) {
					continue;
				}

				uint16_t *depth = graphics.frameBuffer.depthBuffer + (graphics.frameBuffer.resX * y + x);
				*depth = 0;
			}
		}
	}

	windowManager.Redraw(position, width, height, this);
}

void Window::Move(OSRectangle &rectangle) {
	windowManager.mutex.Acquire();

	windowManager.SetActiveWindow(this);

	mutex.Acquire();
	Defer(mutex.Release());

	{
		size_t newWidth = rectangle.right - rectangle.left;
		size_t newHeight = rectangle.bottom - rectangle.top;

		if (newWidth < 16 || newHeight < 16 
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

	windowManager.mutex.Release();
	Update();
}

void Window::Destroy() {
	{
		windowManager.mutex.Acquire();
		Defer(windowManager.mutex.Release());

		KernelLog(LOG_VERBOSE, "Window %x (api %x) is being destroyed...\n", this, apiWindow);

		if (windowManager.pressedWindow == this) windowManager.pressedWindow = nullptr;
		if (windowManager.activeWindow == this) windowManager.activeWindow = nullptr; 
		if (windowManager.hoverWindow == this) windowManager.hoverWindow = nullptr; 

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

		OSMessage message = {};
		message.type = OS_MESSAGE_WINDOW_DESTROYED;
		message.targetWindow = apiWindow;
		owner->messageQueue.SendMessage(message); // THe last message sent to the window.

		windowManager.Redraw(position, width, height);
		CloseHandleToObject(surface, KERNEL_OBJECT_SURFACE);
		OSHeapFree(this, sizeof(Window));
	}

	graphics.UpdateScreen();
}

void WindowManager::Redraw(OSPoint position, int width, int height, Window *except) {
	mutex.AssertLocked();

	{
		OSRectangle background = {position.x, position.x + width, position.y, position.y + height};

		if (background.left < 0) background.left = 0;
		if (background.top < 0) background.top = 0;
		if (background.right > (int) graphics.frameBuffer.resX) background.right = graphics.frameBuffer.resX;
		if (background.bottom > (int) graphics.frameBuffer.resY) background.bottom = graphics.frameBuffer.resY;

#if 0
		graphics.frameBuffer.FillRectangle(background, OSColor(83, 114, 166));
#else
		graphics.frameBuffer.Copy(wallpaperSurface, OSPoint(background.left, background.top), background, false);
#endif
	}

	int index = windowsCount - 1;

	for (; index >= 0; index--) {
		Window *window = windows[index];

		if ((int) window->z != index) {
			KernelPanic("WindowManager::Redraw - Z and index mismatch.\n");
		}

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
			rectangle.right = position.x - window->position.x + width;
		}

		if (position.y + height < window->position.y + (int) window->height) {
			rectangle.bottom = position.y - window->position.y + height;
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
	windowManager.mutex.Acquire();
	cursorStyle = style;
	windowManager.RefreshCursor(this);
	windowManager.mutex.Release();
}

#endif
