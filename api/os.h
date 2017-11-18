#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#define OS_SCANCODE_KEY_RELEASED (1 << 15)
#define OS_SCANCODE_KEY_PRESSED  (0 << 15)

#define OS_SCANCODE_A (0x1C)
#define OS_SCANCODE_B (0x32)
#define OS_SCANCODE_C (0x21)
#define OS_SCANCODE_D (0x23)
#define OS_SCANCODE_E (0x24)
#define OS_SCANCODE_F (0x2B)
#define OS_SCANCODE_G (0x34)
#define OS_SCANCODE_H (0x33)
#define OS_SCANCODE_I (0x43)
#define OS_SCANCODE_J (0x3B)
#define OS_SCANCODE_K (0x42)
#define OS_SCANCODE_L (0x4B)
#define OS_SCANCODE_M (0x3A)
#define OS_SCANCODE_N (0x31)
#define OS_SCANCODE_O (0x44)
#define OS_SCANCODE_P (0x4D)
#define OS_SCANCODE_Q (0x15)
#define OS_SCANCODE_R (0x2D)
#define OS_SCANCODE_S (0x1B)
#define OS_SCANCODE_T (0x2C)
#define OS_SCANCODE_U (0x3C)
#define OS_SCANCODE_V (0x2A)
#define OS_SCANCODE_W (0x1D)
#define OS_SCANCODE_X (0x22)
#define OS_SCANCODE_Y (0x35)
#define OS_SCANCODE_Z (0x1A)

#define OS_SCANCODE_0 (0x45)
#define OS_SCANCODE_1 (0x16)
#define OS_SCANCODE_2 (0x1E)
#define OS_SCANCODE_3 (0x26)
#define OS_SCANCODE_4 (0x25)
#define OS_SCANCODE_5 (0x2E)
#define OS_SCANCODE_6 (0x36)
#define OS_SCANCODE_7 (0x3D)
#define OS_SCANCODE_8 (0x3E)
#define OS_SCANCODE_9 (0x46)

#define OS_SCANCODE_CAPS_LOCK	(0x58)
#define OS_SCANCODE_SCROLL_LOCK	(0x7E)
#define OS_SCANCODE_NUM_LOCK	(0x77) // Also sent by the pause key.
#define OS_SCANCODE_LEFT_SHIFT	(0x12)
#define OS_SCANCODE_LEFT_CTRL	(0x14)
#define OS_SCANCODE_LEFT_ALT	(0x11)
#define OS_SCANCODE_LEFT_FLAG	(0x11F)
#define OS_SCANCODE_RIGHT_SHIFT	(0x59)
#define OS_SCANCODE_RIGHT_CTRL	(0x114)
#define OS_SCANCODE_RIGHT_ALT	(0x111)
#define OS_SCANCODE_RIGHT_FLAG	(0x127)
#define OS_SCANCODE_PAUSE	(0xE1)
#define OS_SCANCODE_CONTEXT_MENU (0x127)

#define OS_SCANCODE_BACKSPACE	(0x66)
#define OS_SCANCODE_ESCAPE	(0x76)
#define OS_SCANCODE_INSERT	(0x170)
#define OS_SCANCODE_HOME	(0x16C)
#define OS_SCANCODE_PAGE_UP	(0x17D)
#define OS_SCANCODE_DELETE	(0x171)
#define OS_SCANCODE_END		(0x169)
#define OS_SCANCODE_PAGE_DOWN	(0x17A)
#define OS_SCANCODE_UP_ARROW	(0x175)
#define OS_SCANCODE_LEFT_ARROW	(0x16B)
#define OS_SCANCODE_DOWN_ARROW	(0x172)
#define OS_SCANCODE_RIGHT_ARROW	(0x174)

#define OS_SCANCODE_SPACE	(0x29)
#define OS_SCANCODE_TAB		(0x0D)
#define OS_SCANCODE_ENTER	(0x5A)

#define OS_SCANCODE_SLASH	(0x4A)
#define OS_SCANCODE_BACKSLASH	(0x5D)
#define OS_SCANCODE_LEFT_BRACE	(0x54)
#define OS_SCANCODE_RIGHT_BRACE	(0x5B)
#define OS_SCANCODE_EQUALS	(0x55)
#define OS_SCANCODE_BACKTICK	(0x0E)
#define OS_SCANCODE_HYPHEN	(0x4E)
#define OS_SCANCODE_SEMICOLON	(0x4C)
#define OS_SCANCODE_QUOTE	(0x52)
#define OS_SCANCODE_COMMA	(0x41)
#define OS_SCANCODE_PERIOD	(0x49)

#define OS_SCANCODE_NUM_DIVIDE 	 (0x14A)
#define OS_SCANCODE_NUM_MULTIPLY (0x7C)
#define OS_SCANCODE_NUM_SUBTRACT (0x7B)
#define OS_SCANCODE_NUM_ADD	 (0x79)
#define OS_SCANCODE_NUM_ENTER	 (0x15A)
#define OS_SCANCODE_NUM_POINT	 (0x71)
#define OS_SCANCODE_NUM_0	 (0x70)
#define OS_SCANCODE_NUM_1	 (0x69)
#define OS_SCANCODE_NUM_2	 (0x72)
#define OS_SCANCODE_NUM_3	 (0x7A)
#define OS_SCANCODE_NUM_4	 (0x6B)
#define OS_SCANCODE_NUM_5	 (0x73)
#define OS_SCANCODE_NUM_6	 (0x74)
#define OS_SCANCODE_NUM_7	 (0x6C)
#define OS_SCANCODE_NUM_8	 (0x75)
#define OS_SCANCODE_NUM_9	 (0x7D)

#define OS_SCANCODE_PRINT_SCREEN_1 (0x112) // Both are sent when print screen is pressed.
#define OS_SCANCODE_PRINT_SCREEN_2 (0x17C)

#define OS_SCANCODE_F1  (0x05)
#define OS_SCANCODE_F2  (0x06)
#define OS_SCANCODE_F3  (0x04)
#define OS_SCANCODE_F4  (0x0C)
#define OS_SCANCODE_F5  (0x03)
#define OS_SCANCODE_F6  (0x0B)
#define OS_SCANCODE_F7  (0x83)
#define OS_SCANCODE_F8  (0x0A)
#define OS_SCANCODE_F9  (0x01)
#define OS_SCANCODE_F10 (0x09)
#define OS_SCANCODE_F11 (0x78)
#define OS_SCANCODE_F12 (0x07)

#define OS_SCANCODE_ACPI_POWER 	(0x137)
#define OS_SCANCODE_ACPI_SLEEP 	(0x13F)
#define OS_SCANCODE_ACPI_WAKE  	(0x15E)

#define OS_SCANCODE_MM_NEXT	(0x14D)
#define OS_SCANCODE_MM_PREVIOUS	(0x115)
#define OS_SCANCODE_MM_STOP	(0x13B)
#define OS_SCANCODE_MM_PAUSE	(0x134)
#define OS_SCANCODE_MM_MUTE	(0x123)
#define OS_SCANCODE_MM_QUIETER	(0x121)
#define OS_SCANCODE_MM_LOUDER	(0x132)
#define OS_SCANCODE_MM_SELECT	(0x150)
#define OS_SCANCODE_MM_EMAIL	(0x148)
#define OS_SCANCODE_MM_CALC	(0x12B)
#define OS_SCANCODE_MM_FILES	(0x140)

#define OS_SCANCODE_WWW_SEARCH	(0x110)
#define OS_SCANCODE_WWW_HOME	(0x13A)
#define OS_SCANCODE_WWW_BACK	(0x138)
#define OS_SCANCODE_WWW_FORWARD	(0x130)
#define OS_SCANCODE_WWW_STOP	(0x128)
#define OS_SCANCODE_WWW_REFRESH	(0x120)
#define OS_SCANCODE_WWW_STARRED	(0x118)

extern "C" uintptr_t _OSSyscall(uintptr_t argument0, uintptr_t argument1, uintptr_t argument2, 
			        uintptr_t unused,    uintptr_t argument3, uintptr_t argument4);
#define OSSyscall(a, b, c, d, e) _OSSyscall((a), (b), (c), 0, (d), (e))

#define OS_CHECK_ERROR(x) (((intptr_t) (x)) < (OS_SUCCESS))

#define OS_SUCCESS 				(-1)

#define OS_ERROR_INVALID_BUFFER 		(-2)	// Always crash?
#define OS_ERROR_UNKNOWN_SYSCALL 		(-3)	// Always crash?
#define OS_ERROR_INVALID_MEMORY_REGION 		(-4)	// Always crash?
#define OS_ERROR_MEMORY_REGION_LOCKED_BY_KERNEL (-5)	// Always crash?
#define OS_ERROR_PATH_LENGTH_EXCEEDS_LIMIT 	(-6)	// Always crash?
#define OS_ERROR_UNKNOWN_OPERATION_FAILURE 	(-7)
#define OS_ERROR_INVALID_HANDLE			(-8)	// Always crash?
#define OS_ERROR_NO_MESSAGES_AVAILABLE		(-9)
#define OS_ERROR_MESSAGE_QUEUE_FULL		(-10)
#define OS_ERROR_MUTEX_NOT_ACQUIRED_BY_THREAD	(-11)	// Always crash?
#define OS_ERROR_MUTEX_ALREADY_ACQUIRED		(-11)	// Always crash?
#define OS_ERROR_BUFFER_NOT_ACCESSIBLE		(-12)	// Always crash?
#define OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI	(-13)
#define OS_ERROR_SHARED_MEMORY_REGION_TOO_LARGE	(-14)	// Always crash?
#define OS_ERROR_SHARED_MEMORY_STILL_MAPPED	(-15)	// Always crash?
#define OS_ERROR_COULD_NOT_LOAD_FONT		(-16)	// Always crash?
#define OS_ERROR_COULD_NOT_DRAW_FONT		(-17)	// Always crash?
#define OS_ERROR_COULD_NOT_ALLOCATE_MEMORY	(-18)	// Always crash?
#define OS_ERROR_FILE_ALREADY_EXISTS		(-19)
#define OS_ERROR_FILE_DOES_NOT_EXIST		(-20)
#define OS_ERROR_DRIVE_ERROR_FILE_DAMAGED	(-21)
#define OS_ERROR_ACCESS_NOT_WITHIN_FILE_BOUNDS	(-22)
#define OS_ERROR_FILE_PERMISSION_NOT_GRANTED	(-23)
#define OS_ERROR_FILE_IN_EXCLUSIVE_USE		(-24)
#define OS_ERROR_FILE_CANNOT_GET_EXCLUSIVE_USE	(-25)
#define OS_ERROR_INCORRECT_FILE_ACCESS		(-26)	// Always crash?
#define OS_ERROR_EVENT_NOT_SET			(-27)
#define OS_ERROR_TOO_MANY_WAIT_OBJECTS		(-28)   // Always crash?
#define OS_ERROR_TIMEOUT_REACHED		(-29)
#define OS_ERROR_INCORRECT_NODE_TYPE		(-30) 	// Always crash?

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
#define OS_SYSCALL_WAIT				(26)
#define OS_SYSCALL_CREATE_SHARED_MEMORY		(27)
#define OS_SYSCALL_SHARE_MEMORY			(28)
#define OS_SYSCALL_MAP_SHARED_MEMORY		(29)
#define OS_SYSCALL_OPEN_NAMED_SHARED_MEMORY	(30)
#define OS_SYSCALL_TERMINATE_PROCESS		(31)
#define OS_SYSCALL_OPEN_NODE			(32)
#define OS_SYSCALL_READ_FILE_SYNC		(33)
#define OS_SYSCALL_WRITE_FILE_SYNC		(34)
#define OS_SYSCALL_RESIZE_FILE			(35)
#define OS_SYSCALL_CREATE_EVENT			(36)
#define OS_SYSCALL_SET_EVENT			(37)
#define OS_SYSCALL_RESET_EVENT			(38)
#define OS_SYSCALL_POLL_EVENT			(39)
#define OS_SYSCALL_REFRESH_NODE_INFORMATION	(40)

#define OS_INVALID_HANDLE 		((OSHandle) (0))
#define OS_CURRENT_THREAD	 	((OSHandle) (0x1000))
#define OS_CURRENT_PROCESS	 	((OSHandle) (0x1001))
#define OS_SURFACE_UI_SHEET		((OSHandle) (0x2000))

#define OS_WAIT_NO_TIMEOUT (-1)

// Note: If you're using a timeout, then 
#define OS_MAX_WAIT_COUNT 		(16)

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

struct OSUniqueIdentifier {
	uint8_t d[16];
};

enum OSNodeType {
	OS_NODE_FILE,
	OS_NODE_DIRECTORY,
};

struct OSNodeInformation {
	OSHandle handle;
	OSUniqueIdentifier identifier;
	OSNodeType type;

	union {
		uint64_t fileSize;
		uint64_t directoryChildren;
	};
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

enum OSCallbackType {
	OS_CALLBACK_INVALID,
	OS_CALLBACK_ACTION,
	OS_CALLBACK_GET_LABEL,
};

struct OSCallbackData {
	OSCallbackType type;

	union {
		struct {
			char *label;
			size_t labelLength;
			bool freeLabel;
		} getLabel;
	};
};

typedef void (*_OSCallback)(struct OSControl *generator, void *argument, OSCallbackData *data);

struct OSCallback {
	_OSCallback callback;
	void *argument;
};

enum OSControlType {
	OS_CONTROL_BUTTON,
	OS_CONTROL_CHECKBOX,
	OS_CONTROL_RADIOBOX,
	OS_CONTROL_STATIC,
	OS_CONTROL_GROUP,
	OS_CONTROL_TEXTBOX,
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

	OSCallback action;
	OSCallback getLabel;

	char *label;
	size_t labelLength;
	bool freeLabel;
	unsigned textAlign;

	OSRectangle image;
	OSRectangle imageBorder;

	OSControlImageType imageType;
	int fillWidth;

#define OS_CONTROL_NO_CHECK (0)
#define OS_CONTROL_CHECKED (1)
#define OS_CONTROL_RADIO_CHECK (2)
	int checked;

	bool canHaveFocus;
};

struct OSWindow {
	OSHandle handle;
	OSHandle surface;

	OSControl *controls[256];
	size_t controlsCount;

	OSControl *hoverControl;
	OSControl *pressedControl;
	OSControl *focusedControl;

	bool dirty;
};

// TODO Implement separate message queues.
#define OS_MESSAGE_QUEUE_WINDOW_MANAGER (0x01)
#define OS_MESSAGE_QUEUE_DEBUGGER	(0x02)
#define OS_MESSAGE_QUEUE_FILE_IO	(0x04)
#define OS_MESSAGE_QUEUE_SYSTEM_EVENT	(0x08)
#define OS_MESSAGE_QUEUE_IPC		(0x10)

enum OSMessageType {
	OS_MESSAGE_MOUSE_MOVED = 0x1000,
	OS_MESSAGE_MOUSE_LEFT_PRESSED = 0x1001,
	OS_MESSAGE_MOUSE_LEFT_RELEASED = 0x1002,

	OS_MESSAGE_KEYBOARD = 0x1400,
	
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

		struct {
			unsigned scancode; // Check for OS_SCANCODE_KEY_RELEASED.
		} keyboard;
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

#define OS_OPEN_NODE_ACCESS_READ	(0x1)
#define OS_OPEN_NODE_ACCESS_WRITE	(0x2)
#define OS_OPEN_NODE_ACCESS_RESIZE	(0x4)

#define OS_OPEN_NODE_EXCLUSIVE_READ	(0x10)
#define OS_OPEN_NODE_EXCLUSIVE_WRITE	(0x20)
#define OS_OPEN_NODE_EXCLUSIVE_RESIZE	(0x40)

#define OS_OPEN_NODE_FAIL_IF_FOUND	(0x100)
#define OS_OPEN_NODE_FAIL_IF_NOT_FOUND	(0x200)

#define OS_OPEN_NODE_DIRECTORY		(0x2000)
#define OS_OPEN_NODE_CREATE_DIRECTORIES	(0x4000) // Create the directories leading to the file, if they don't already exist.

#ifndef KERNEL
extern "C" OSError OSCreateProcess(const char *executablePath, size_t executablePathLength, OSProcessInformation *information, void *argument);
extern "C" OSError OSCreateThread(OSThreadEntryFunction entryFunction, OSThreadInformation *information, void *argument);
extern "C" OSHandle OSCreateSurface(size_t width, size_t height);
extern "C" OSHandle OSCreateMutex();
extern "C" OSHandle OSCreateEvent(bool autoReset);

extern "C" OSError OSCloseHandle(OSHandle handle);

extern "C" OSError OSOpenNode(char *path, size_t pathLength, uint64_t flags, OSNodeInformation *information);
extern "C" void *OSReadEntireFile(const char *filePath, size_t filePathLength, size_t *fileSize); 
extern "C" size_t OSReadFileSync(OSHandle file, uint64_t offset, size_t size, void *buffer); // If return value >= 0, number of bytes read. Otherwise, OSError.
extern "C" size_t OSWriteFileSync(OSHandle file, uint64_t offset, size_t size, void *buffer); // If return value >= 0, number of bytes written. Otherwise, OSError.
extern "C" OSError OSResizeFile(OSHandle file, uint64_t newSize);

extern "C" OSError OSTerminateThread(OSHandle thread);
extern "C" OSError OSTerminateProcess(OSHandle thread);

extern "C" OSError OSReleaseMutex(OSHandle mutex);
extern "C" OSError OSAcquireMutex(OSHandle mutex);

extern "C" OSError OSSetEvent(OSHandle event);
extern "C" OSError OSResetEvent(OSHandle event);
extern "C" OSError OSPollEvent(OSHandle event);

extern "C" uintptr_t OSWait(OSHandle *objects, size_t objectCount, uintptr_t timeoutMs);

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
extern "C" uint8_t OSGetRandomByte();

// TODO Possibly remove all of these?
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

extern "C" uint64_t osRandomByteSeed;
#endif
