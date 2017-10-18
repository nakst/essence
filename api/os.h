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
#define OS_ERROR_SHARED_MEMORY_REGION_TOO_LARGE	(-14)
#define OS_ERROR_SHARED_MEMORY_STILL_MAPPED	(-15)
#define OS_ERROR_COULD_NOT_LOAD_FONT		(-16)
#define OS_ERROR_COULD_NOT_DRAW_FONT		(-17)
#define OS_ERROR_COULD_NOT_ALLOCATE_MEMORY	(-18)
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
#define OS_SYSCALL_READ_ENTIRE_FILE		(26)
#define OS_SYSCALL_CREATE_SHARED_MEMORY		(27)
#define OS_SYSCALL_SHARE_MEMORY			(28)
#define OS_SYSCALL_MAP_SHARED_MEMORY		(29)
#define OS_SYSCALL_OPEN_NAMED_SHARED_MEMORY	(30)
#define OS_SYSCALL_TERMINATE_PROCESS		(31)

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
	size_t width, height, stride;
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

enum OSEventType {
	OS_EVENT_INVALID,
	OS_EVENT_ACTION,
	OS_EVENT_GET_LABEL,
};

struct OSEvent {
	OSEventType type;

	union {
		struct {
			char *label;
			size_t labelLength;
			bool freeLabel;
		} getLabel;
	};
};

typedef void (*_OSEventCallback)(struct OSControl *generator, void *argument, OSEvent *event);

struct OSEventCallback {
	_OSEventCallback callback;
	void *argument;
};

enum OSControlType {
	OS_CONTROL_BUTTON,
	OS_CONTROL_CHECKBOX,
	OS_CONTROL_RADIOBOX,
	OS_CONTROL_STATIC,
};

enum OSControlImageType {
	OS_CONTROL_IMAGE_FILL,
	OS_CONTROL_IMAGE_CENTER_LEFT,
	OS_CONTROL_IMAGE_NONE,
};

struct OSControl {
	OSRectangle bounds;

	OSControlType type;

	bool disabled;
	struct OSWindow *parent;

	OSEventCallback action;
	OSEventCallback getLabel;

	char *label;
	size_t labelLength;
	bool freeLabel;

	OSRectangle image;

	OSControlImageType imageType;
	int fillWidth;

#define OS_CONTROL_NO_CHECK (0)
#define OS_CONTROL_CHECKED (1)
#define OS_CONTROL_RADIO_CHECK (2)
	int checked;
};

struct OSWindow {
	OSHandle handle;
	OSHandle surface;

	OSControl *controls[256];
	size_t controlsCount;

	OSControl *hoverControl;
	OSControl *pressedControl;

	bool dirty;
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
		void *argument;

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

#define OS_CONTROL_ENABLED  (false)
#define OS_CONTROL_DISABLED (true)

#define OS_SHARED_MEMORY_MAXIMUM_SIZE (1073742824)
#define OS_SHARED_MEMORY_NAME_MAX_LENGTH (32)
#define OS_SHARED_MEMORY_MAP_ALL (0)

#define OS_GUI_FONT_REGULAR ((char *) "Shell/Font/RegularGUI")

#define OS_DRAW_STRING_HALIGN_LEFT 	(1)
#define OS_DRAW_STRING_HALIGN_RIGHT 	(2)
#define OS_DRAW_STRING_HALIGN_CENTER 	(OS_DRAW_STRING_HALIGN_LEFT | OS_DRAW_STRING_HALIGN_RIGHT)

#define OS_DRAW_STRING_VALIGN_TOP 	(4)
#define OS_DRAW_STRING_VALIGN_BOTTOM 	(8)
#define OS_DRAW_STRING_VALIGN_CENTER 	(OS_DRAW_STRING_VALIGN_TOP | OS_DRAW_STRING_VALIGN_BOTTOM)

#ifndef KERNEL
extern "C" OSError OSCreateProcess(const char *executablePath, size_t executablePathLength, OSProcessInformation *information, void *argument);
extern "C" OSError OSCreateThread(OSThreadEntryFunction entryFunction, OSThreadInformation *information, void *argument);
extern "C" OSHandle OSCreateSurface(size_t width, size_t height);
extern "C" OSHandle OSCreateMutex();

extern "C" OSError OSCloseHandle(OSHandle handle);

extern "C" void *OSReadEntireFile(const char *filePath, size_t filePathLength, size_t *fileSize); // TODO Temporary. Replace with a proper file I/O API.

extern "C" OSError OSTerminateThread(OSHandle thread);
extern "C" OSError OSTerminateProcess(OSHandle thread);

extern "C" OSError OSReleaseMutex(OSHandle mutex);
extern "C" OSError OSAcquireMutex(OSHandle mutex);

extern "C" OSHandle OSCreateSharedMemory(size_t size, char *name, size_t nameLength);
extern "C" OSHandle OSShareMemory(OSHandle sharedMemoryRegion, OSHandle targetProcess, bool readOnly);
extern "C" void *OSMapSharedMemory(OSHandle sharedMemoryRegion, uintptr_t offset, size_t size);
extern "C" OSHandle OSOpenNamedSharedMemory(char *name, size_t nameLength);

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
extern "C" OSError OSDrawString(OSHandle surface, OSRectangle region, char *string, size_t stringLength, unsigned flags, uint32_t color, int32_t backgroundColor);

extern "C" OSError OSGetMessage(OSMessage *message);
extern "C" OSError OSSendMessage(OSHandle process, OSMessage *message);
extern "C" OSError OSWaitMessage(uintptr_t timeoutMs);

extern "C" OSWindow *OSCreateWindow(size_t width, size_t height);
extern "C" OSError OSUpdateWindow(OSWindow *window);
extern "C" OSControl *OSCreateControl(OSControlType type, char *label, size_t labelLength, bool cloneLabel);
extern "C" OSError OSAddControl(OSWindow *window, OSControl *control, int x, int y);
extern "C" OSError OSProcessGUIMessage(OSMessage *message);
extern "C" void OSDisableControl(OSControl *control, bool disabled);
extern "C" void OSCheckControl(OSControl *control, bool checked);
extern "C" OSError OSSetControlLabel(OSControl *control, char *label, size_t labelLength, bool clone);
extern "C" OSError OSInvalidateControl(OSControl *control);

extern "C" void *OSHeapAllocate(size_t size, bool zeroMemory);
extern "C" void OSHeapFree(void *address);

extern "C" size_t OSCStringLength(char *string);
extern "C" void OSCopyMemory(void *destination, void *source, size_t bytes);
extern "C" void OSZeroMemory(void *destination, size_t bytes);
extern "C" int OSCompareBytes(void *a, void *b, size_t bytes);
extern "C" uint8_t OSSumBytes(uint8_t *data, size_t bytes);
extern "C" void OSPrint(const char *format, ...);
extern "C" size_t OSFormatString(char *buffer, size_t bufferLength, const char *format, ...);
extern "C" void OSHelloWorld();

extern "C" void *memset(void *s, int c, size_t n);
extern "C" void *memcpy(void *dest, const void *src, size_t n);
extern "C" size_t strlen(const char *s);
extern "C" void *malloc(size_t size);
extern "C" void *calloc(size_t num, size_t size);
extern "C" void *memmove(void *dest, const void *src, size_t n);
extern "C" void free(void *ptr);
extern "C" void *realloc(void *ptr, size_t size);
extern "C" double fabs(double x);
extern "C" int abs(int n);
extern "C" int ifloor(double x);
extern "C" int iceil(double x);
extern "C" double sqrt(double x);
extern "C" void OSAssertionFailure();
#define assert(x) do{if (!(x)) OSAssertionFailure();}while(0)

#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#include "stb_image.h"
#endif
