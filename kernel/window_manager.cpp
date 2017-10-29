#ifndef IMPLEMENTATION

#define LEFT_BUTTON (1)
#define MIDDLE_BUTTON (2)
#define RIGHT_BUTTON (4)

struct Window {
	void Update();

	Surface surface;
	OSPoint position;
	size_t width, height;
	uintptr_t z;
	Process *owner;
	OSWindow *apiWindow;
	bool keyboardFocus;
};

struct WindowManager {
	void Initialise();
	Window *CreateWindow(Process *process, size_t width, size_t height);
	void MoveCursor(int xMovement, int yMovement);
	void ClickCursor(unsigned buttons);
	void PressKey(unsigned scancode);

	Pool windowPool;

	Window **windows;
	size_t windowsCount, windowsAllocated;

	Mutex mutex;

	int cursorX, cursorY;
	unsigned lastButtons;
};

WindowManager windowManager;
Surface uiSheetSurface;

#else

void WindowManager::PressKey(unsigned scancode) {
	mutex.Acquire();
	Defer(mutex.Release());

	// Send the message to the last window on which we clicked.
	for (uintptr_t i = 0; i < windowsCount; i++) {
		Window *window = windows[i];
		if (window->keyboardFocus) {
			OSMessage message = {};
			message.type = OS_MESSAGE_KEYBOARD;
			message.targetWindow = window->apiWindow;
			message.keyboard.scancode = scancode;
			window->owner->SendMessage(message);
		}
	}
}

void WindowManager::ClickCursor(unsigned buttons) {
	mutex.Acquire();
	Defer(mutex.Release());

	for (uintptr_t i = 0; i < windowsCount; i++) {
		windows[i]->keyboardFocus = false;
	}

	unsigned delta = lastButtons ^ buttons;

	if (delta & LEFT_BUTTON) {
		// TODO Send mouse released messages to the window the cursor was over when the mouse was pressed.
		// 	And do the same thing for mouse movement messages.

		// Send a mouse pressed message to the window the cursor is over.
		uint16_t index = graphics.frameBuffer.depthBuffer[graphics.frameBuffer.resX * cursorY + cursorX];
		if (index) {
			Window *window = windows[index - 1];
			window->keyboardFocus = true;

			OSMessage message = {};
			message.type = (buttons & LEFT_BUTTON) ? OS_MESSAGE_MOUSE_LEFT_PRESSED : OS_MESSAGE_MOUSE_LEFT_RELEASED;
			message.targetWindow = window->apiWindow;
			message.mousePressed.positionX = cursorX - window->position.x;
			message.mousePressed.positionY = cursorY - window->position.y;
			window->owner->SendMessage(message);
		} else {
			// The cursor is not in a window.
		}
	}

	lastButtons = buttons;
}

void WindowManager::MoveCursor(int xMovement, int yMovement) {
	mutex.Acquire();
	Defer(mutex.Release());

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

		OSMessage message = {};
		message.type = OS_MESSAGE_MOUSE_MOVED;
		message.targetWindow = window->apiWindow;
		message.mouseMoved.newPositionX = cursorX - window->position.x;
		message.mouseMoved.newPositionY = cursorY - window->position.y;
		message.mouseMoved.oldPositionX = oldCursorX - window->position.x;
		message.mouseMoved.oldPositionY = oldCursorY - window->position.y;
		window->owner->SendMessage(message);
	} else {
		// The cursor is not in a window.
	}

	graphics.UpdateScreen();
}

void WindowManager::Initialise() {
	windowPool.Initialise(sizeof(Window));
	uiSheetSurface.Initialise(kernelProcess->vmm, 256, 256, false);

	// Draw the background.
	graphics.frameBuffer.FillRectangle(OSRectangle(0, graphics.resX, 0, graphics.resY), OSColor(83, 114, 166));

	cursorX = graphics.resX / 2;
	cursorY = graphics.resY / 2;
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

#endif
