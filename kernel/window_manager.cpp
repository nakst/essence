#ifndef IMPLEMENTATION

struct Window {
	void Update();

	Surface surface;
	OSPoint position;
	size_t width, height;
	uintptr_t z;
};

struct WindowManager {
	void Initialise();
	Window *CreateWindow(Process *process, size_t width, size_t height);

	Pool windowPool;

	Window **windows;
	size_t windowsCount, windowsAllocated;

	Mutex mutex;
};

WindowManager windowManager;

#else

#include "../res/UISheet.c_bmp"
Surface uiSheetSurface;

void WindowManager::Initialise() {
	windowPool.Initialise(sizeof(Window));

	uiSheetSurface.Initialise(kernelProcess->vmm, uiSheetWidth, uiSheetHeight, false);
	CopyMemory(uiSheetSurface.linearBuffer, uiSheet, uiSheetWidth * uiSheetHeight * 4);

	graphics.frameBuffer.FillRectangle(OSRectangle(0, graphics.resX, 0, graphics.resY), OSColor(83, 114, 166));
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

	ArrayAdd(windows, window);

	return window;
}

void Window::Update() {
	graphics.frameBuffer.Copy(surface, position, OSRectangle(0, width, 0, height), true, z);
	graphics.UpdateScreen();

	surface.mutex.Acquire();
	surface.ClearModifiedRegion();
	surface.mutex.Release();
}

#endif
