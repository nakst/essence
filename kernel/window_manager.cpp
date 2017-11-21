#ifndef IMPLEMENTATION

#define SCANCODE_KEY_RELEASED (1 << 15)
#define SCANCODE_KEY_PRESSED  (0 << 15)

#define LEFT_BUTTON (1)
#define MIDDLE_BUTTON (2)
#define RIGHT_BUTTON (4)

struct Window {
	void Update();
	void SetCursorStyle(OSCursorStyle style);

	Surface surface;
	OSPoint position;
	size_t width, height;
	uintptr_t z;
	Process *owner;
	OSWindow *apiWindow;
	bool keyboardFocus;
	OSCursorStyle cursorStyle;
};

struct WindowManager {
	void Initialise();
	Window *CreateWindow(Process *process, size_t width, size_t height);
	void MoveCursor(int xMovement, int yMovement);
	void ClickCursor(unsigned buttons);
	void UpdateCursor(int xMovement, int yMovement, unsigned buttons);
	void PressKey(unsigned scancode);
	void RefreshCursor(Window *window);

	Pool windowPool;

	Window **windows;
	size_t windowsCount, windowsAllocated;

	Window *focusedWindow, *pressedWindow;

	Mutex mutex;

	int cursorX, cursorY;
	int cursorImageX, cursorImageY;
	int cursorImageOffsetX, cursorImageOffsetY;

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
		} break;

		default: {
			windowManager.cursorImageX = 125;
			windowManager.cursorImageY = 96;
			windowManager.cursorImageOffsetX = 0;
			windowManager.cursorImageOffsetY = 0;
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
		for (uintptr_t i = 0; i < windowsCount; i++) {
			windows[i]->keyboardFocus = false;
		}

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
				message.mousePressed.clickChainCount = clickChainCount;

				message.targetWindow = window->apiWindow;
				window->keyboardFocus = true;

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
	if (index) {
		Window *window = windows[index - 1];

		if (pressedWindow) {
			// Always send the messages to the pressed window, if there is one.
			window = pressedWindow;
		}

		OSMessage message = {};
		message.type = OS_MESSAGE_MOUSE_MOVED;
		message.targetWindow = window->apiWindow;
		message.mouseMoved.newPositionX = cursorX - window->position.x;
		message.mouseMoved.newPositionY = cursorY - window->position.y;
		message.mouseMoved.oldPositionX = oldCursorX - window->position.x;
		message.mouseMoved.oldPositionY = oldCursorY - window->position.y;
		window->owner->SendMessage(message);

		RefreshCursor(window);
	} else {
		RefreshCursor(pressedWindow);

		// The cursor is not in a window.
	}

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

	windowPool.Initialise(sizeof(Window));
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

	Window *window = (Window *) windowPool.Add();

	if (!window->surface.Initialise(process->vmm, width, height, false)) {
		windowPool.Remove(window);
		return nullptr;
	}

	window->position = OSPoint(cx += 20, cy += 20);
	window->width = width;
	window->height = height;
	window->z = windowsCount;
	window->owner = process;

	ArrayAdd(windows, window);

	return window;
}

void Window::Update() {
	graphics.frameBuffer.Copy(surface, position, OSRectangle(0, width, 0, height), true, z + 1);
	graphics.UpdateScreen();

	surface.mutex.Acquire();
	surface.ClearModifiedRegion();
	surface.mutex.Release();
}

void Window::SetCursorStyle(OSCursorStyle style) {
	windowManager.mutex.Acquire();
	cursorStyle = style;
	windowManager.RefreshCursor(this);
	windowManager.mutex.Release();
}

#endif
