#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

// ----- SYSTEM CALL DEFINITIONS

extern "C" uintptr_t _OSSyscall(uintptr_t argument0, uintptr_t argument1, uintptr_t argument2, 
			        uintptr_t unused,    uintptr_t argument3, uintptr_t argument4);
#define OSSyscall(a, b, c, d, e) _OSSyscall((a), (b), (c), 0, (d), (e))

#define OS_SUCCESS 				(-1)
#define OS_ERROR_INVALID_BUFFER 		(-2)
#define OS_ERROR_UNKNOWN_SYSCALL 		(-3)
#define OS_ERROR_INVALID_MEMORY_REGION 		(-4)
#define OS_ERROR_MEMORY_REGION_LOCKED_BY_KERNEL (-5)
#define OS_ERROR_PATH_LENGTH_EXCEEDS_LIMIT 	(-6)
#define OS_ERROR_UNKNOWN_OPERATION_FAILURE 	(-7)
#define OS_ERROR_INVALID_HANDLE			(-8)
#define OS_ERROR_NO_MESSAGES_AVAILABLE		(-9)
#define OS_ERROR_MESSAGE_QUEUE_FULL		(-10)
#define OS_ERROR_MUTEX_NOT_ACQUIRED_BY_THREAD	(-11)
#define OS_ERROR_MUTEX_ALREADY_ACQUIRED		(-11)
#define OS_ERROR_BUFFER_NOT_ACCESSIBLE		(-12)
#define OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI	(-13)
typedef intptr_t OSError;

#define OS_SYSCALL_PRINT			(0)
#define OS_SYSCALL_ALLOCATE 			(1)
#define OS_SYSCALL_FREE 			(2)
#define OS_SYSCALL_CREATE_PROCESS 		(3)
#define OS_SYSCALL_GET_CREATION_ARGUMENT	(4)
#define OS_SYSCALL_CREATE_SURFACE		(5)
#define OS_SYSCALL_GET_LINEAR_BUFFER		(6)
#define OS_SYSCALL_INVALIDATE_RECTANGLE		(7)
#define OS_SYSCALL_COPY_TO_SCREEN		(8)
#define OS_SYSCALL_FORCE_SCREEN_UPDATE		(9)
#define OS_SYSCALL_FILL_RECTANGLE		(10)
#define OS_SYSCALL_COPY_SURFACE			(11)
#define OS_SYSCALL_CLEAR_MODIFIED_REGION	(12)
#define OS_SYSCALL_GET_MESSAGE			(13)
#define OS_SYSCALL_SEND_MESSAGE			(14)
#define OS_SYSCALL_WAIT_MESSAGE			(15)
#define OS_SYSCALL_CREATE_WINDOW		(16)
#define OS_SYSCALL_OPEN_WINDOW_SURFACE		(17)
#define OS_SYSCALL_UPDATE_WINDOW		(18)
#define OS_SYSCALL_DRAW_SURFACE			(19)
#define OS_SYSCALL_CREATE_MUTEX			(20)
#define OS_SYSCALL_ACQUIRE_MUTEX		(21)
#define OS_SYSCALL_RELEASE_MUTEX		(22)
#define OS_SYSCALL_CLOSE_HANDLE			(23)
#define OS_SYSCALL_TERMINATE_THREAD		(24)
#define OS_SYSCALL_CREATE_THREAD		(25)

#define OS_INVALID_HANDLE 		((OSHandle) (0))
#define OS_CURRENT_THREAD	 	((OSHandle) (0x1000))
#define OS_CURRENT_PROCESS	 	((OSHandle) (0x1001))
#define OS_SURFACE_UI_SHEET		((OSHandle) (0x2000))

#define OS_WAIT_NO_TIMEOUT (-1)

typedef uintptr_t OSHandle;

struct OSThreadInformation {
	OSHandle handle;
	uintptr_t tid;
};

struct OSProcessInformation {
	OSHandle handle;
	uintptr_t pid;
	OSThreadInformation mainThread;
};

struct OSPoint {
	OSPoint() {}

	OSPoint(intptr_t _x, intptr_t _y) {
		x = _x;
		y = _y;
	}

	intptr_t x;
	intptr_t y;
};

struct OSRectangle {
	OSRectangle() {}

	OSRectangle(intptr_t _left, intptr_t _right, intptr_t _top, intptr_t _bottom) {
		left = _left;
		right = _right;
		top = _top;
		bottom = _bottom;
	}

	intptr_t left;
	intptr_t right;
	intptr_t top;
	intptr_t bottom;
};

struct OSColor {
	OSColor() {}

	OSColor(uint8_t _red, uint8_t _green, uint8_t _blue) {
		red = _red;
		green = _green;
		blue = _blue;
	}

	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

enum OSColorFormat {
	OS_COLOR_FORMAT_32_XRGB,
}; 

struct OSLinearBuffer {
	size_t width, height;
	OSColorFormat colorFormat;
	void *buffer;
};

struct _OSRectangleAndColor {
	OSRectangle rectangle;
	OSColor color;
};

struct _OSDrawSurfaceArguments {
	OSRectangle source, destination, border;
};

typedef void (*_OSEventCallback)(struct OSControl *generator, void *argument);

struct OSEventCallback {
	_OSEventCallback callback;
	void *argument;
};

struct OSControl {
	int x;
	int y;
	int width;
	int height;
	OSEventCallback action;
};

struct OSWindow {
	OSHandle handle;
	OSHandle surface;

	OSControl *controls[256];
	size_t controlsCount;

	OSControl *hoverControl;
	OSControl *pressedControl;
};

enum OSMessageType {
	OS_MESSAGE_MOUSE_MOVED = 0x1000,
	OS_MESSAGE_MOUSE_LEFT_PRESSED = 0x1001,
	OS_MESSAGE_MOUSE_LEFT_RELEASED = 0x1002,
	
	OS_MESSAGE_WINDOW_CREATED = 0x2000,
};

struct OSMessage {
	OSMessageType type;
	OSWindow *targetWindow;

	union {
		uint8_t data[32];

		struct {
			int oldPositionX;
			int newPositionX;
			int oldPositionY;
			int newPositionY;
		} mouseMoved;

		struct {
			int positionX;
			int positionY;
		} mousePressed;
	};
};

enum OSDrawMode {
	OS_DRAW_MODE_STRECH, // Not implemented yet.
	OS_DRAW_MODE_REPEAT, // Not implemented yet.
	OS_DRAW_MODE_REPEAT_FIRST,
};

typedef void (*OSThreadEntryFunction)(void *argument);

#ifndef KERNEL
extern "C" OSError OSCreateProcess(const char *executablePath, size_t executablePathLength, OSProcessInformation *information, void *argument);
extern "C" OSError OSCreateThread(OSThreadEntryFunction entryFunction, OSThreadInformation *information, void *argument);
extern "C" OSHandle OSCreateSurface(size_t width, size_t height);
extern "C" OSHandle OSCreateMutex();

extern "C" OSError OSCloseHandle(OSHandle handle);

extern "C" OSError OSTerminateThread(OSHandle thread);

extern "C" OSError OSReleaseMutex(OSHandle mutex);
extern "C" OSError OSAcquireMutex(OSHandle mutex);

extern "C" void *OSAllocate(size_t size);
extern "C" OSError OSFree(void *address);

extern "C" void *OSGetCreationArgument(OSHandle process);

extern "C" OSError OSGetLinearBuffer(OSHandle surface, OSLinearBuffer *linearBuffer);
extern "C" OSError OSInvalidateRectangle(OSHandle surface, OSRectangle rectangle);
extern "C" OSError OSCopyToScreen(OSHandle source, OSPoint point, uint16_t depth);
extern "C" OSError OSForceScreenUpdate();
extern "C" OSError OSFillRectangle(OSHandle surface, OSRectangle rectangle, OSColor color);
extern "C" OSError OSCopySurface(OSHandle destination, OSHandle source, OSPoint destinationPoint);
extern "C" OSError OSDrawSurface(OSHandle destination, OSHandle source, OSRectangle destinationRegion, OSRectangle sourceRegion, OSRectangle borderRegion, OSDrawMode mode);
extern "C" OSError OSClearModifiedRegion(OSHandle surface);

extern "C" OSError OSGetMessage(OSMessage *message);
extern "C" OSError OSSendMessage(OSHandle process, OSMessage *message);
extern "C" OSError OSWaitMessage(uintptr_t timeoutMs);

extern "C" OSWindow *OSCreateWindow(size_t width, size_t height);
extern "C" OSError OSUpdateWindow(OSWindow *window);
extern "C" OSControl *OSCreateControl();
extern "C" OSError OSAddControl(OSWindow *window, OSControl *control, int x, int y);
extern "C" OSError OSProcessGUIMessage(OSMessage *message);

extern "C" void *OSHeapAllocate(size_t size);
extern "C" void OSHeapFree(void *address);

extern "C" size_t OSCStringLength(char *string);
extern "C" void OSCopyMemory(void *destination, void *source, size_t bytes);
extern "C" void OSZeroMemory(void *destination, size_t bytes);
extern "C" int OSCompareBytes(void *a, void *b, size_t bytes);
extern "C" uint8_t OSSumBytes(uint8_t *data, size_t bytes);
extern "C" void OSPrint(const char *format, ...);
extern "C" size_t OSFormatString(char *buffer, size_t bufferLength, const char *format, ...);
#endif
