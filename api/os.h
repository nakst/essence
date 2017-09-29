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
#define OS_ERROR_MUTEX_ACQUIRED_BY_THREAD	(-11)
typedef int OSError;

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

#define OS_INVALID_HANDLE 		((OSHandle) (0))
#define OS_CURRENT_THREAD	 	((OSHandle) (0x1000))
#define OS_CURRENT_PROCESS	 	((OSHandle) (0x1001))
#define OS_SURFACE_UI_SHEET		((OSHandle) (0x2000))

#define OS_WAIT_NO_TIMEOUT (-1)

typedef uintptr_t OSHandle;

struct OSProcessInformation {
	OSHandle handle;
	uintptr_t pid;
};

struct OSPoint {
	OSPoint() {}

	OSPoint(uintptr_t _x, uintptr_t _y) {
		x = _x;
		y = _y;
	}

	uintptr_t x;
	uintptr_t y;
};

struct OSRectangle {
	OSRectangle() {}

	OSRectangle(uintptr_t _left, uintptr_t _right, uintptr_t _top, uintptr_t _bottom) {
		left = _left;
		right = _right;
		top = _top;
		bottom = _bottom;
	}

	uintptr_t left;
	uintptr_t right;
	uintptr_t top;
	uintptr_t bottom;
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

struct OSWindow {
	OSHandle handle;
	OSHandle surface;
};

enum OSMessageType {
};

struct OSMessage {
	OSMessageType type;

	union {
		uint8_t data[16];
	};
};

enum OSDrawMode {
	OS_DRAW_MODE_STRECH, // Not implemented yet.
	OS_DRAW_MODE_REPEAT, // Not implemented yet.
	OS_DRAW_MODE_REPEAT_FIRST,
};

#ifndef KERNEL
extern "C" OSError OSCreateProcess(const char *executablePath, size_t executablePathLength, OSProcessInformation *information, void *argument);
extern "C" OSHandle OSCreateSurface(size_t width, size_t height);
extern "C" OSHandle OSCreateMutex();

extern "C" OSError OSReleaseMutex(OSHandle handle);
extern "C" OSError OSAcquireMutex(OSHandle handle);

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

extern "C" OSError OSCreateWindow(OSWindow *window, size_t width, size_t height);
extern "C" OSError OSUpdateWindow(OSWindow *window);

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
