#ifndef IMPLEMENTATION

#define SCANCODE_KEY_RELEASED (1 << 15)
#define SCANCODE_KEY_PRESSED  (0 << 15)

#define LEFT_BUTTON (1)
#define MIDDLE_BUTTON (2)
#define RIGHT_BUTTON (4)

struct Window {
	void Update(bool fromUser);
	void SetCursorStyle(OSCursorStyle style);
	void NeedWMTimer(int hz);
	void Destroy();
	bool Move(OSRectangle &newBounds);
	void ClearImage();

	Mutex mutex; // Mutex for drawing to the window. Also needed when moving the window.
	Surface *surface;
	OSPoint position;
	size_t width, height;
	uintptr_t z;
	Process *owner;
	OSObject apiWindow;
	OSCursorStyle cursorStyle;
	int needsTimerMessagesHz, timerMessageTick;
	bool resizing; // Don't draw the window until the program calls UPDATE_WINDOW.
	Window *menuParent, *modalChild;

	volatile unsigned handles;
};

struct WindowManager {
	void Initialise();
	Window *CreateWindow(Process *process, OSRectangle bounds, OSObject apiWindow, Window *menuParentWindow, Window *modalParentWindow);
	void MoveCursor(int xMovement, int yMovement);
	void ClickCursor(unsigned buttons);
	void UpdateCursor(int xMovement, int yMovement, unsigned buttons);
	void PressKey(unsigned scancode);
	void RefreshCursor(Window *window);
	void Redraw(OSPoint position, int width, int height, Window *except = nullptr);
	void SetActiveWindow(Window *window);

	bool initialised;

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
	bool shift2, alt2, ctrl2;

	uint64_t clickChainStartMs;
	unsigned clickChainCount;
	int clickChainX, clickChainY;
};

struct Clipboard {
	void Copy(void *buffer, OSClipboardHeader *header);
	void PasteText(void *buffer, size_t size);

	Mutex mutex;

	void *buffer;
	OSClipboardHeader header;
};

WindowManager windowManager;
Clipboard clipboard;
Surface uiSheetSurface, wallpaperSurface;

#else

void Clipboard::Copy(void *newBuffer, OSClipboardHeader *newHeader) {
	mutex.Acquire();
	Defer(mutex.Release());

	// TODO UTF-8 validation of the text.

	header = *newHeader;
	if (buffer) OSHeapFree(buffer);

	if (newBuffer) {
		buffer = OSHeapAllocate(header.textBytes + header.customBytes, false);
		CopyMemory(buffer, newBuffer, header.textBytes + header.customBytes);
	}

	// Tell the active window about the new contents of the clipboard.
	{
		windowManager.mutex.Acquire();

		Window *window = windowManager.activeWindow;

		while (window) {
			OSMessage message = {};
			message.type = OS_MESSAGE_CLIPBOARD_UPDATED;
			message.context = window->apiWindow;
			message.clipboard = header;
			window->owner->messageQueue.SendMessage(message);
			window = window->menuParent;
		}

		windowManager.mutex.Release();
	}
}

void Clipboard::PasteText(void *outputBuffer, size_t size) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (size > header.textBytes) {
		size = header.textBytes;
	}

	CopyMemory(outputBuffer, buffer, size);
}

void WindowManager::UpdateCursor(int xMovement, int yMovement, unsigned buttons) {
	if (!initialised) {
		return;
	}

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

		case OS_CURSOR_SPLIT_HORIZONTAL: {
			windowManager.cursorImageX = 143;
			windowManager.cursorImageY = 122;
			windowManager.cursorImageOffsetX = -8;
			windowManager.cursorImageOffsetY = -5;
			windowManager.cursorImageWidth = 20;
			windowManager.cursorImageHeight = 15;
		} break;

		case OS_CURSOR_SPLIT_VERTICAL: {
			windowManager.cursorImageX = 169;
			windowManager.cursorImageY = 120;
			windowManager.cursorImageOffsetX = -5;
			windowManager.cursorImageOffsetY = -8;
			windowManager.cursorImageWidth = 15;
			windowManager.cursorImageHeight = 20;
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

	if (scancode == OS_SCANCODE_NUM_DIVIDE) {
		KernelPanic("WindowManager::PressKey - Panic key pressed.\n");
	}

	// TODO Caps/num lock.

	if (scancode == OS_SCANCODE_LEFT_CTRL) ctrl = true;
	if (scancode == (OS_SCANCODE_LEFT_CTRL | SCANCODE_KEY_RELEASED)) ctrl = false;
	if (scancode == OS_SCANCODE_LEFT_SHIFT) shift = true;
	if (scancode == (OS_SCANCODE_LEFT_SHIFT | SCANCODE_KEY_RELEASED)) shift = false;
	if (scancode == OS_SCANCODE_LEFT_ALT) alt = true;
	if (scancode == (OS_SCANCODE_LEFT_ALT | SCANCODE_KEY_RELEASED)) alt = false;

	if (scancode == OS_SCANCODE_RIGHT_CTRL) ctrl2 = true;
	if (scancode == (OS_SCANCODE_RIGHT_CTRL | SCANCODE_KEY_RELEASED)) ctrl2 = false;
	if (scancode == OS_SCANCODE_RIGHT_SHIFT) shift2 = true;
	if (scancode == (OS_SCANCODE_RIGHT_SHIFT | SCANCODE_KEY_RELEASED)) shift2 = false;
	if (scancode == OS_SCANCODE_RIGHT_ALT) alt2 = true;
	if (scancode == (OS_SCANCODE_RIGHT_ALT | SCANCODE_KEY_RELEASED)) alt2 = false;

	if (activeWindow) {
		Window *window = activeWindow;

		OSMessage message = {};
		message.type = (scancode & SCANCODE_KEY_RELEASED) ? OS_MESSAGE_KEY_RELEASED : OS_MESSAGE_KEY_PRESSED;
		message.context = window->apiWindow;
		message.keyboard.alt = alt | alt2;
		message.keyboard.ctrl = ctrl | ctrl2;
		message.keyboard.shift = shift | shift2;
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

	if (activeWindow == window) {
		return;
	}

	// KernelLog(LOG_VERBOSE, "Set active window to %x\n", window);

	{
		Window *oldActiveWindow = activeWindow;
		activeWindow = window;

		// This is a menu. They don't get activation messages.
		if (window && window->menuParent) return;

		// This is either a top-level window, or no window is active.
		// Deactivate the old window and its menus (to close them).
		// Except don't deactivate the old window if we're just about to activate it again.
		while (oldActiveWindow && oldActiveWindow != window) {
			OSMessage message = {};
			message.type = OS_MESSAGE_WINDOW_DEACTIVATED;
			message.context = oldActiveWindow->apiWindow;
			oldActiveWindow->owner->messageQueue.SendMessage(message);
			oldActiveWindow = oldActiveWindow->menuParent;
		}
	}

	if (!window) {
		return;
	}

	while (window->modalChild) window = window->modalChild;

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
		message.context = window->apiWindow;
		window->owner->messageQueue.SendMessage(message);
	}
}

void WindowManager::ClickCursor(unsigned buttons) {
	mutex.Acquire();

	unsigned delta = lastButtons ^ buttons;
	lastButtons = buttons;

	bool moveCursorNone = false;

	if (delta) {
		// Send a mouse pressed message to the window the cursor is over.
		uint16_t index = graphics.frameBuffer.depthBuffer[graphics.frameBuffer.resX * cursorY + cursorX];
		Window *window;

		if (index) {
			window = windows[index - 1];
		} else {
			window = nullptr;

			// The cursor is not in a window.
		}

		bool activationClick = false;

		if (buttons == delta) {
			if (activeWindow != window) {
				activationClick = true;
			}

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

			if (delta & LEFT_BUTTON) message.type = (buttons & LEFT_BUTTON) ? OS_MESSAGE_MOUSE_LEFT_PRESSED : OS_MESSAGE_MOUSE_LEFT_RELEASED;
			if (delta & RIGHT_BUTTON) message.type = (buttons & RIGHT_BUTTON) ? OS_MESSAGE_MOUSE_RIGHT_PRESSED : OS_MESSAGE_MOUSE_RIGHT_RELEASED;
			if (delta & MIDDLE_BUTTON) message.type = (buttons & MIDDLE_BUTTON) ? OS_MESSAGE_MOUSE_MIDDLE_PRESSED : OS_MESSAGE_MOUSE_MIDDLE_RELEASED;

			if (message.type == OS_MESSAGE_MOUSE_LEFT_PRESSED) {
				pressedWindow = window;
			} else if (message.type == OS_MESSAGE_MOUSE_LEFT_RELEASED) {
				if (pressedWindow) {
					// Always send the messages to the pressed window, if there is one.
					window = pressedWindow;
				}

				pressedWindow = nullptr;
				moveCursorNone = true; // We might have moved outside the window.
			}
			
			if (window) {
				if (window->modalChild) {
					message.type = OS_MESSAGE_MODAL_PARENT_CLICKED;
					window->modalChild->owner->messageQueue.SendMessage(message);
				} else {
					message.mousePressed.positionX = cursorX - window->position.x;
					message.mousePressed.positionY = cursorY - window->position.y;
					message.mousePressed.positionXScreen = cursorX;
					message.mousePressed.positionYScreen = cursorY;
					message.mousePressed.clickChainCount = clickChainCount;
					message.mousePressed.activationClick = activationClick;
					message.mousePressed.alt = alt | alt2;
					message.mousePressed.ctrl = ctrl | ctrl2;
					message.mousePressed.shift = shift | shift2;
					message.context = window->apiWindow;

					RefreshCursor(window);

					window->owner->messageQueue.SendMessage(message);
				}
			} else {
				RefreshCursor(nullptr);
			}
		}
	}

	mutex.Release();

	if (moveCursorNone) {
		MoveCursor(0, 0);
	} else {
		graphics.UpdateScreen();
	}
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
		message.context = hoverWindow->apiWindow;
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
		message.context = hoverWindow->apiWindow;
		hoverWindow->owner->messageQueue.SendMessage(message);
	}

	if (window && !window->modalChild) {
		OSMessage message = {};
		message.type = OS_MESSAGE_MOUSE_MOVED;
		message.context = window->apiWindow;
		message.mouseMoved.newPositionX = cursorX - window->position.x;
		message.mouseMoved.newPositionY = cursorY - window->position.y;
		message.mouseMoved.newPositionXScreen = cursorX;
		message.mouseMoved.newPositionYScreen = cursorY;
		message.mouseMoved.oldPositionX = oldCursorX - window->position.x;
		message.mouseMoved.oldPositionY = oldCursorY - window->position.y;
		window->owner->messageQueue.SendMessage(message);
	}

	if (window && window->modalChild) {
		RefreshCursor(nullptr);
	} else {
		RefreshCursor(window);
	}

	mutex.Release();
	graphics.UpdateScreen();
}

void WMTimerMessages(WindowManager *windowManager) {
	Timer timer = {};
	uint64_t tick = 0;

	while (true) {
		timer.Set(30, true);
		timer.event.Wait(OS_WAIT_NO_TIMEOUT);
		timer.Remove();
		tick++;

		windowManager->mutex.Acquire();

		for (uintptr_t i = 0; i < windowManager->windowsCount; i++) {
			Window *window = windowManager->windows[i];

			if (window->needsTimerMessagesHz) {
				int ticksPerMessage = 1000 / window->needsTimerMessagesHz / 30;
				window->timerMessageTick++;

				if (window->timerMessageTick >= ticksPerMessage) {
					OSMessage message = {};
					message.type = OS_MESSAGE_WM_TIMER;
					message.context = window->apiWindow;
					window->owner->messageQueue.SendMessage(message);
					window->timerMessageTick = 0;
				}
			}
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
	Redraw(OS_MAKE_POINT(0, 0), graphics.resX, graphics.resY);
	mutex.Release();
	graphics.UpdateScreen();

	// Create the window manager timer thread.
	scheduler.SpawnThread((uintptr_t) WMTimerMessages, (uintptr_t) this, kernelProcess, false);

	initialised = true;
}

Window *WindowManager::CreateWindow(Process *process, OSRectangle bounds, OSObject apiWindow, Window *menuParentWindow, Window *modalParentWindow) {
	Window *window;

	{
		mutex.Acquire();
		Defer(mutex.Release());

		static int cx = 20, cy = 20;

		window = (Window *) OSHeapAllocate(sizeof(Window), true);
		window->surface = (Surface *) OSHeapAllocate(sizeof(Surface), true);
		window->apiWindow = apiWindow;
		// window->surface->roundCorners = true;

		size_t width = bounds.right - bounds.left;
		size_t height = bounds.bottom - bounds.top;

		if (!window->surface->Initialise(width, height, false)) {
			OSHeapFree(window->surface, sizeof(Surface));
			OSHeapFree(window, sizeof(Window));
			return nullptr;
		}

		window->surface->handles++;
		window->position = bounds.left == 0 && bounds.top == 0 ? OS_MAKE_POINT(cx += 20, cy += 20) : OS_MAKE_POINT(bounds.left, bounds.top);
		window->width = width;
		window->height = height;
		window->owner = process;
		window->menuParent = menuParentWindow;
		if (modalParentWindow) modalParentWindow->modalChild = window;
		window->handles = 1;

		// KernelLog(LOG_VERBOSE, "Created window %x, handles = %d, menuParent = %x\n", window, window->handles, window->menuParent);

		window->z = windowsCount;
		ArrayAdd(windows, window, false);
		SetActiveWindow(window);
	}

	MoveCursor(0, 0);

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

bool Window::Move(OSRectangle &rectangle) {
	bool result = true;

	windowManager.mutex.Acquire();
	windowManager.SetActiveWindow(this);

	mutex.Acquire();
	Defer(mutex.Release());

	{
		size_t newWidth = rectangle.right - rectangle.left;
		size_t newHeight = rectangle.bottom - rectangle.top;

		if (newWidth < 4 || newHeight < 4 
				|| rectangle.left > rectangle.right 
				|| rectangle.top > rectangle.bottom
				|| newWidth > graphics.resX + 64
				|| newHeight > graphics.resY + 64) {
			result = false;
			goto done;
		}

		ClearImage();
		position = OS_MAKE_POINT(rectangle.left, rectangle.top);

		size_t oldWidth = width;
		size_t oldHeight = height;
		width = newWidth;
		height = newHeight;

		if (oldWidth != width || oldHeight != height) {
			surface->Resize(width, height);
			resizing = true; // Don't draw the window again until the API actually requests it.
		}

		surface->InvalidateRectangle(OS_MAKE_RECTANGLE(0, width, 0, height));
	}

	done:;

	rectangle.left = position.x;
	rectangle.top = position.y;
	rectangle.right = position.x + width;
	rectangle.bottom = position.y + height;

	windowManager.mutex.Release();

	if (result) Update(false);
	return result;
}

void Window::Destroy() {
	Window *updateActiveWindow = nullptr;

	{
		windowManager.mutex.Acquire();
		Defer(windowManager.mutex.Release());

		// KernelLog(LOG_VERBOSE, "Window %x (api %x) is being destroyed...\n", this, apiWindow);

		if (windowManager.pressedWindow == this) windowManager.pressedWindow = nullptr;
		if (windowManager.hoverWindow == this) windowManager.hoverWindow = nullptr; 

		bool findActiveWindow = false;

		if (windowManager.activeWindow == this) {
			windowManager.activeWindow = nullptr; 
			findActiveWindow = true;
		}

		for (uintptr_t i = 0; i < windowManager.windowsCount; i++) {
			if (windowManager.windows[i]->menuParent == this) {
				windowManager.windows[i]->menuParent = nullptr;
			}

			if (windowManager.windows[i]->modalChild == this) {
				windowManager.windows[i]->modalChild = nullptr;
				updateActiveWindow = windowManager.windows[i];
				findActiveWindow = false;
			}
		}

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

		if (findActiveWindow && windowManager.windowsCount) {
			updateActiveWindow = windowManager.windows[windowManager.windowsCount - 1];
		}

		OSMessage message = {};
		message.type = OS_MESSAGE_WINDOW_DESTROYED;
		message.context = apiWindow;
		owner->messageQueue.SendMessage(message); // The last message sent to the window.

		windowManager.Redraw(position, width, height);
		CloseHandleToObject(surface, KERNEL_OBJECT_SURFACE);
		OSHeapFree(this, sizeof(Window));
	}

	if (updateActiveWindow) {
		windowManager.mutex.Acquire();
		windowManager.SetActiveWindow(updateActiveWindow);
		windowManager.mutex.Release();
	}

	{
		windowManager.MoveCursor(0, 0);
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
		graphics.frameBuffer.Copy(wallpaperSurface, OS_MAKE_POINT(background.left, background.top), background, false);
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

		if (!window->resizing) {
			graphics.frameBuffer.Copy(*window->surface, window->position, OS_MAKE_RECTANGLE(0, window->width, 0, window->height), true, window->z + 1);
		}

		window->surface->mutex.Acquire();
		window->surface->ClearModifiedRegion();
		window->surface->mutex.Release();
	}
}

void Window::Update(bool fromUser) {
	mutex.AssertLocked();

	if (fromUser && resizing) {
		return;
	}

	resizing = false;

	graphics.frameBuffer.Copy(*surface, position, OS_MAKE_RECTANGLE(0, width, 0, height), true, z + 1);
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

void Window::NeedWMTimer(int hz) {
	windowManager.mutex.Acquire();
	needsTimerMessagesHz = hz;
	windowManager.mutex.Release();
}

#endif
