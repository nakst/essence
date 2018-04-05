#ifndef IncludedEssenceAPIHeader
#define IncludedEssenceAPIHeader

// TODO Remove constructors.

#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
#define OS_EXTERN_C extern "C"
#define OS_CONSTRUCTOR(x) x

// Scoped defer: http://www.gingerbill.org/article/defer-in-cpp.html
template <typename F> struct OSprivDefer { F f; OSprivDefer(F f) : f(f) {} ~OSprivDefer() { f(); } };
template <typename F> OSprivDefer<F> OSdefer_func(F f) { return OSprivDefer<F>(f); }
#define OSDEFER_1(x, y) x##y
#define OSDEFER_2(x, y) OSDEFER_1(x, y)
#define OSDEFER_3(x)    OSDEFER_2(x, __COUNTER__)
#define OS_Defer(code)   auto OSDEFER_3(_defer_) = OSdefer_func([&](){code;})
#define OSDefer(code)   OS_Defer(code)

#else
#define OS_EXTERN_C 
#define OS_CONSTRUCTOR(x)
#endif

#define OSLiteral(x) (char *) x, OSCStringLength((char *) x)

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

#define OS_ICON_NONE 		(0)
#define OS_ICON_FILE 		(1)
#define OS_ICON_FOLDER 		(2)
#define OS_ICON_ERROR 		(3)

#define OS_FLAGS_DEFAULT (0)

OS_EXTERN_C uintptr_t _OSSyscall(uintptr_t argument0, uintptr_t argument1, uintptr_t argument2, 
			        uintptr_t unused,    uintptr_t argument3, uintptr_t argument4);
#define OSSyscall(a, b, c, d, e) _OSSyscall((a), (b), (c), 0, (d), (e))

#define OS_CHECK_ERROR(x) (((intptr_t) (x)) < (OS_SUCCESS))

#define OS_SUCCESS 				(-1)

typedef enum OSFatalError {
	OS_FATAL_ERROR_INVALID_BUFFER,
	OS_FATAL_ERROR_UNKNOWN_SYSCALL,
	OS_FATAL_ERROR_INVALID_MEMORY_REGION,
	OS_FATAL_ERROR_MEMORY_REGION_LOCKED_BY_KERNEL,
	OS_FATAL_ERROR_PATH_LENGTH_EXCEEDS_LIMIT,
	OS_FATAL_ERROR_INVALID_HANDLE, // Note: this has to be fatal!! See the linear handle list.
	OS_FATAL_ERROR_MUTEX_NOT_ACQUIRED_BY_THREAD,
	OS_FATAL_ERROR_MUTEX_ALREADY_ACQUIRED,
	OS_FATAL_ERROR_BUFFER_NOT_ACCESSIBLE,
	OS_FATAL_ERROR_SHARED_MEMORY_REGION_TOO_LARGE,
	OS_FATAL_ERROR_SHARED_MEMORY_STILL_MAPPED,
	OS_FATAL_ERROR_COULD_NOT_LOAD_FONT,
	OS_FATAL_ERROR_COULD_NOT_DRAW_FONT,
	OS_FATAL_ERROR_COULD_NOT_ALLOCATE_MEMORY,
	OS_FATAL_ERROR_INCORRECT_FILE_ACCESS,
	OS_FATAL_ERROR_TOO_MANY_WAIT_OBJECTS,
	OS_FATAL_ERROR_INCORRECT_NODE_TYPE,
	OS_FATAL_ERROR_PROCESSOR_EXCEPTION,
	OS_FATAL_ERROR_INVALID_PANE_CHILD,
	OS_FATAL_ERROR_INVALID_PANE_OBJECT,
	OS_FATAL_ERROR_UNSUPPORTED_CALLBACK,
	OS_FATAL_ERROR_MISSING_CALLBACK,
	OS_FATAL_ERROR_UNKNOWN,
	OS_FATAL_ERROR_RECURSIVE_BATCH,
	OS_FATAL_ERROR_CORRUPT_HEAP,
	OS_FATAL_ERROR_BAD_CALLBACK_OBJECT,
	OS_FATAL_ERROR_RESIZE_GRID,
	OS_FATAL_ERROR_OUT_OF_GRID_BOUNDS,
	OS_FATAL_ERROR_OVERWRITE_GRID_OBJECT,
	OS_FATAL_ERROR_CORRUPT_LINKED_LIST,
	OS_FATAL_ERROR_NO_MENU_POSITION,
	OS_FATAL_ERROR_COUNT,
	OS_FATAL_ERROR_BAD_OBJECT_TYPE,
	OS_FATAL_ERROR_MESSAGE_SHOULD_BE_HANDLED,
	OS_FATAL_ERROR_INDEX_OUT_OF_BOUNDS,
} OSFatalError;

// These must be negative.
// See OSReadFileSync.
#define OS_ERROR_BUFFER_TOO_SMALL		(-2)
#define OS_ERROR_UNKNOWN_OPERATION_FAILURE 	(-7)
#define OS_ERROR_NO_MESSAGES_AVAILABLE		(-9)
#define OS_ERROR_MESSAGE_QUEUE_FULL		(-10)
#define OS_ERROR_MESSAGE_NOT_HANDLED_BY_GUI	(-13)
#define OS_ERROR_PATH_NOT_WITHIN_MOUNTED_VOLUME	(-14)
#define OS_ERROR_PATH_NOT_TRAVERSABLE		(-15)
#define OS_ERROR_FILE_ALREADY_EXISTS		(-19)
#define OS_ERROR_FILE_DOES_NOT_EXIST		(-20)
#define OS_ERROR_DRIVE_ERROR_FILE_DAMAGED	(-21) 
#define OS_ERROR_ACCESS_NOT_WITHIN_FILE_BOUNDS	(-22) 
#define OS_ERROR_FILE_PERMISSION_NOT_GRANTED	(-23)
#define OS_ERROR_FILE_IN_EXCLUSIVE_USE		(-24)
#define OS_ERROR_FILE_CANNOT_GET_EXCLUSIVE_USE	(-25)
#define OS_ERROR_INCORRECT_NODE_TYPE		(-26)
#define OS_ERROR_EVENT_NOT_SET			(-27)
#define OS_ERROR_TIMEOUT_REACHED		(-29)
#define OS_ERROR_REQUEST_CLOSED_BEFORE_COMPLETE (-30)
#define OS_ERROR_NO_CHARACTER_AT_COORDINATE	(-31)
#define OS_ERROR_FILE_ON_READ_ONLY_VOLUME	(-32)
#define OS_ERROR_USER_CANCELED_IO		(-33)
#define OS_ERROR_INVALID_DIMENSIONS		(-34)
#define OS_ERROR_DRIVE_CONTROLLER_REPORTED	(-35)
#define OS_ERROR_COULD_NOT_ISSUE_PACKET		(-36)
#define OS_ERROR_HANDLE_TABLE_FULL		(-37)
#define OS_ERROR_COULD_NOT_RESIZE_FILE		(-38)

typedef intptr_t OSError;

typedef enum OSSyscallType {
	OS_SYSCALL_PRINT,
	OS_SYSCALL_ALLOCATE,
	OS_SYSCALL_FREE,
	OS_SYSCALL_CREATE_PROCESS,
	OS_SYSCALL_GET_CREATION_ARGUMENT,
	OS_SYSCALL_CREATE_SURFACE,
	OS_SYSCALL_GET_LINEAR_BUFFER,
	OS_SYSCALL_INVALIDATE_RECTANGLE,
	OS_SYSCALL_COPY_TO_SCREEN,
	OS_SYSCALL_FORCE_SCREEN_UPDATE,
	OS_SYSCALL_FILL_RECTANGLE,
	OS_SYSCALL_COPY_SURFACE,
	OS_SYSCALL_CLEAR_MODIFIED_REGION,
	OS_SYSCALL_GET_MESSAGE,
	OS_SYSCALL_POST_MESSAGE,
	OS_SYSCALL_WAIT_MESSAGE,
	OS_SYSCALL_CREATE_WINDOW,
	OS_SYSCALL_UPDATE_WINDOW,
	OS_SYSCALL_DRAW_SURFACE,
	OS_SYSCALL_CREATE_MUTEX,
	OS_SYSCALL_ACQUIRE_MUTEX, // This (and RELEASE_MUTEX) is the most common system call.
	OS_SYSCALL_RELEASE_MUTEX, // TODO Implement API-side locks?
	OS_SYSCALL_CLOSE_HANDLE,
	OS_SYSCALL_TERMINATE_THREAD,
	OS_SYSCALL_CREATE_THREAD,
	OS_SYSCALL_WAIT,
	OS_SYSCALL_SHARE_MEMORY,
	OS_SYSCALL_MAP_OBJECT,
	OS_SYSCALL_OPEN_SHARED_MEMORY,
	OS_SYSCALL_TERMINATE_PROCESS,
	OS_SYSCALL_OPEN_NODE,
	OS_SYSCALL_READ_FILE_SYNC,
	OS_SYSCALL_WRITE_FILE_SYNC,
	OS_SYSCALL_RESIZE_FILE,
	OS_SYSCALL_CREATE_EVENT,
	OS_SYSCALL_SET_EVENT,
	OS_SYSCALL_RESET_EVENT,
	OS_SYSCALL_POLL_EVENT,
	OS_SYSCALL_REFRESH_NODE_INFORMATION,
	OS_SYSCALL_SET_CURSOR_STYLE,
	OS_SYSCALL_MOVE_WINDOW,
	OS_SYSCALL_GET_WINDOW_BOUNDS,
	OS_SYSCALL_REDRAW_ALL,
	OS_SYSCALL_PAUSE_PROCESS,
	OS_SYSCALL_CRASH_PROCESS,
	OS_SYSCALL_GET_THREAD_ID,
	OS_SYSCALL_ENUMERATE_DIRECTORY_CHILDREN,
	OS_SYSCALL_READ_FILE_ASYNC,
	OS_SYSCALL_WRITE_FILE_ASYNC,
	OS_SYSCALL_GET_IO_REQUEST_PROGRESS,
	OS_SYSCALL_CANCEL_IO_REQUEST,
	OS_SYSCALL_BATCH,
	OS_SYSCALL_NEED_WM_TIMER,
	OS_SYSCALL_RESET_CLICK_CHAIN,
	OS_SYSCALL_GET_CURSOR_POSITION,
	OS_SYSCALL_COPY,
	OS_SYSCALL_GET_CLIPBOARD_HEADER,
	OS_SYSCALL_PASTE_TEXT,
	OS_SYSCALL_REMOVE_NODE_FROM_PARENT,
} OSSyscallType;

#define OS_INVALID_HANDLE 		((OSHandle) (0))
#define OS_CURRENT_THREAD	 	((OSHandle) (0x1000))
#define OS_CURRENT_PROCESS	 	((OSHandle) (0x1001))
#define OS_SURFACE_UI_SHEET		((OSHandle) (0x2000))
#define OS_SURFACE_WALLPAPER		((OSHandle) (0x2001))

#define OS_INVALID_OBJECT		(nullptr)

#define OS_WAIT_NO_TIMEOUT (-1)

// Note: If you're using a timeout, then 
#define OS_MAX_WAIT_COUNT 		(16)

typedef uintptr_t OSHandle;
typedef void *OSObject;

typedef struct OSBatchCall {
	OSSyscallType index; 
	bool stopBatchIfError;
	union { uintptr_t argument0, returnValue; };
	uintptr_t argument1, argument2, argument3;
} OSBatchCall;

typedef struct OSThreadInformation {
	OSHandle handle;
	uintptr_t tid;
} OSThreadInformation;

typedef struct OSProcessInformation {
	OSHandle handle;
	uintptr_t pid;
	OSThreadInformation mainThread;
} OSProcessInformation;

typedef struct OSUniqueIdentifier {
	uint8_t d[16];
} OSUniqueIdentifier;

typedef enum OSNodeType {
	OS_NODE_FILE,
	OS_NODE_DIRECTORY,
	OS_NODE_INVALID,
} OSNodeType;

typedef struct OSNodeInformation {
	union {
		OSHandle handle;
		bool present; // From OSEnumerateDirectoryChildren.
	};

	// OSUniqueIdentifier identifier; // This can change when all handles to the node are closed.
	OSNodeType type;

	union {
		uint64_t fileSize;
		uint64_t directoryChildren;
	};
} OSNodeInformation;

typedef struct OSDirectoryChild {
#define OS_MAX_DIRECTORY_CHILD_NAME_LENGTH (256)
	char name[OS_MAX_DIRECTORY_CHILD_NAME_LENGTH];
	size_t nameLengthBytes;
	OSNodeInformation information; 
} OSDirectoryChild;

typedef struct OSPoint {
	int32_t x;
	int32_t y;
} OSPoint;

typedef struct OSRectangle {
	int32_t left;   // Inclusive.
	int32_t right;  // Exclusive.
	int32_t top;    // Inclusive.
	int32_t bottom; // Exclusive.
} OSRectangle;

#define OS_MAKE_RECTANGLE(l, r, t, b) ((OSRectangle){(int32_t)(l),(int32_t)(r),(int32_t)(t),(int32_t)(b)})
#define OS_MAKE_CALLBACK(a, b) ((OSCallback){(a),(b)})
#define OS_MAKE_POINT(x, y) ((OSPoint){(int32_t)(x),(int32_t)(y)})

typedef struct OSColor {
	OS_CONSTRUCTOR(OSColor() {})

	OS_CONSTRUCTOR(OSColor(uint32_t x) {
		red = (x & 0xFF0000) >> 16;
		green = (x & 0xFF00) >> 8;
		blue = (x & 0xFF) >> 0;
	})

	OS_CONSTRUCTOR(OSColor(uint8_t _red, uint8_t _green, uint8_t _blue) {
		red = _red;
		green = _green;
		blue = _blue;
	})

	uint8_t red;
	uint8_t green;
	uint8_t blue;
} OSColor;

typedef enum OSColorFormat {
	OS_COLOR_FORMAT_32_XRGB,
} OSColorFormat; 

typedef struct OSLinearBuffer {
	size_t width, height, stride;
	OSColorFormat colorFormat;
	OSHandle handle; // A shared memory region. See OSMapSharedMemory.
} OSLinearBuffer;

typedef struct _OSRectangleAndColor {
	OSRectangle rectangle;
	OSColor color;
} _OSRectangleAndColor;

typedef struct _OSDrawSurfaceArguments {
	OSRectangle source, destination, border;
	uint8_t alpha;
} _OSDrawSurfaceArguments;

typedef enum OSCursorStyle {
	OS_CURSOR_NORMAL, 
	OS_CURSOR_TEXT, 
	OS_CURSOR_RESIZE_VERTICAL, 
	OS_CURSOR_RESIZE_HORIZONTAL,
	OS_CURSOR_RESIZE_DIAGONAL_1, // '/'
	OS_CURSOR_RESIZE_DIAGONAL_2, // '\'
	OS_CURSOR_SPLIT_VERTICAL,
	OS_CURSOR_SPLIT_HORIZONTAL,
} OSCursorStyle;

typedef struct OSString {
	char *buffer;
	size_t bytes, characters;
} OSString;

typedef struct OSCaret {
	uintptr_t byte, character;
} OSCaret;

typedef struct OSCrashReason {
	OSError errorCode;
} OSCrashReason;

typedef struct OSIORequestProgress {
	uint64_t accessed;
	uint64_t progress; 
	bool completed, cancelled;
	OSError error;
} OSIORequestProgress;

typedef enum OSClipboardFormat {
	OS_CLIPBOARD_FORMAT_EMPTY,
	OS_CLIPBOARD_FORMAT_TEXT,
} OSClipboardFormat;

typedef struct OSClipboardHeader {
	size_t customBytes;
	OSClipboardFormat format;
	size_t textBytes;
	uintptr_t unused;
} OSClipboardHeader;

typedef enum OSMessageType {
	// GUI messages:

	OS_MESSAGE_LAYOUT			= 0x0200,
	OS_MESSAGE_LAYOUT_TEXT			= 0x0201,
	OS_MESSAGE_MEASURE			= 0x0202,
	OS_MESSAGE_PAINT			= 0x0203,
	OS_MESSAGE_PAINT_BACKGROUND		= 0x0204,
	OS_MESSAGE_CUSTOM_PAINT			= 0x0205,

	OS_MESSAGE_DESTROY			= 0x0210,
	OS_MESSAGE_PARENT_UPDATED		= 0x0211,
	OS_MESSAGE_CHILD_UPDATED		= 0x0212,
	OS_MESSAGE_TEXT_UPDATED			= 0x0213,
	OS_MESSAGE_HIT_TEST			= 0x0214,
	OS_MESSAGE_CARET_BLINK			= 0x0215,

	OS_MESSAGE_CLICKED			= 0x0240,
	OS_MESSAGE_START_HOVER			= 0x0241,
	OS_MESSAGE_END_HOVER			= 0x0242,
	OS_MESSAGE_START_PRESS			= 0x0243,
	OS_MESSAGE_END_PRESS			= 0x0244,
	OS_MESSAGE_START_DRAG			= 0x0245,
	OS_MESSAGE_START_FOCUS			= 0x0246,
	OS_MESSAGE_END_FOCUS			= 0x0247,
	OS_MESSAGE_END_LAST_FOCUS		= 0x0248,
	OS_MESSAGE_MOUSE_DRAGGED		= 0x0249,
	OS_MESSAGE_KEY_TYPED			= 0x024A,

	// Window manager messages:
	OS_MESSAGE_MOUSE_MOVED 			= 0x1000,
	OS_MESSAGE_MOUSE_LEFT_PRESSED 		= 0x1001,
	OS_MESSAGE_MOUSE_LEFT_RELEASED 		= 0x1002,
	OS_MESSAGE_KEY_PRESSED			= 0x1003,
	OS_MESSAGE_KEY_RELEASED			= 0x1004,
	OS_MESSAGE_WINDOW_CREATED 		= 0x1005,
	OS_MESSAGE_WM_TIMER	 		= 0x1006, 
	OS_MESSAGE_WINDOW_ACTIVATED		= 0x1007,
	OS_MESSAGE_WINDOW_DEACTIVATED		= 0x1008,
	OS_MESSAGE_WINDOW_DESTROYED 		= 0x1009,
	OS_MESSAGE_MOUSE_EXIT			= 0x100A, // Sent when the mouse leaves the window's bounds.
	OS_MESSAGE_MOUSE_ENTER			= 0x100B, 
	OS_MESSAGE_MOUSE_RIGHT_PRESSED 		= 0x100C,
	OS_MESSAGE_MOUSE_RIGHT_RELEASED 	= 0x100D,
	OS_MESSAGE_MOUSE_MIDDLE_PRESSED 	= 0x100E,
	OS_MESSAGE_MOUSE_MIDDLE_RELEASED 	= 0x100F,
	OS_MESSAGE_MODAL_PARENT_CLICKED		= 0x1010,

	// Notifications:
	OS_NOTIFICATION_COMMAND			= 0x2000,
	OS_NOTIFICATION_VALUE_CHANGED		= 0x2001,
	OS_NOTIFICATION_GET_ITEM		= 0x2002,
	OS_NOTIFICATION_DESELECT_ALL		= 0x2003,
	OS_NOTIFICATION_SET_ITEM		= 0x2004,
	OS_NOTIFICATION_START_EDIT		= 0x2005,
	OS_NOTIFICATION_END_EDIT		= 0x2006,
	OS_NOTIFICATION_CANCEL_EDIT		= 0x2007,
	OS_NOTIFICATION_CONFIRM_EDIT		= 0x2008,
	OS_NOTIFICATION_CHOOSE_ITEM		= 0x2009,

	// Misc messages:
	OS_MESSAGE_PROGRAM_CRASH		= 0x5000,
	OS_MESSAGE_CLIPBOARD_UPDATED		= 0x5001,
	OS_MESSAGE_SET_PROPERTY			= 0x5002,

	// User messages:
	OS_MESSAGE_USER_START			= 0x8000,
	OS_MESSAGE_USER_END			= 0xBFFF,
} OSMessageType;

typedef struct OSMessage {
	OSMessageType type;
	void *context;

	union {
		void *argument;

		struct {
			int oldPositionX;
			int newPositionX;
			int oldPositionY;
			int newPositionY;
			int newPositionXScreen;
			int newPositionYScreen;
		} mouseMoved;

		struct {
			// Structure must match `mouseMoved`.
			int originalPositionX;
			int newPositionX;
			int originalPositionY;
			int newPositionY;
			int newPositionXScreen;
			int newPositionYScreen;
		} mouseDragged;

		struct {
			int positionX;
			int positionY;
			int positionXScreen;
			int positionYScreen;
			uint8_t clickChainCount;
			uint8_t activationClick : 1;
			uint8_t alt : 1, ctrl : 1, shift : 1;
		} mousePressed;

		struct {
			int positionX;
			int positionY;
			int positionXScreen;
			int positionYScreen;
		} mouseEntered;

		struct {
			unsigned scancode; 
			bool alt, ctrl, shift;
		} keyboard;

		struct {
			OSCrashReason reason;
			OSHandle process;
		} crash;

		struct {
			int left, right, top, bottom;
			OSRectangle clip;
			bool force;
		} layout;

		struct {
			int preferredWidth, preferredHeight;
			int minimumWidth, minimumHeight;
		} measure;

		struct {
			OSHandle surface;
			OSRectangle clip;
			bool force;
		} paint;

		struct {
			OSHandle surface;
			OSRectangle clip;
			int left, right, top, bottom;
		} paintBackground;

		struct {
			OSObject window, grid;
		} parentUpdated;

		struct {
			int positionX, positionY;
			bool result;
		} hitTest;

		struct {
			OSObject window;
			struct OSCommand *command;
			bool checked;
		} command;

		struct {
			int newValue;
		} valueChanged;

#define OS_LIST_VIEW_ITEM_SELECTED	   (0x0001)
#define OS_LIST_VIEW_ITEM_TEXT             (0x10000)
#define OS_LIST_VIEW_ITEM_ICON		   (0x20000)
		struct {
			char *text; 
			size_t textBytes;
			uint32_t mask;
			int32_t index, column;
			uint16_t iconID;
			uint16_t state;
		} listViewItem;

		struct {
			uintptr_t index;
			void *value;
		} setProperty;

		struct OSClipboardHeader clipboard;
	};
} OSMessage;

// Determines how the image is scaled.
typedef enum OSDrawMode {
	OS_DRAW_MODE_STRECH, // Not implemented yet.
	OS_DRAW_MODE_REPEAT, // Not implemented yet.
	OS_DRAW_MODE_REPEAT_FIRST, // The first non-border pixel is repeated.
} OSDrawMode;

typedef void (*OSThreadEntryFunction)(void *argument);

typedef struct OSDebuggerMessage {
	OSHandle process;
	OSCrashReason reason;
} OSDebuggerMessage;

typedef int OSCallbackResponse;
typedef OSCallbackResponse (*OSCallbackFunction)(OSObject object, OSMessage *);

typedef struct OSCallback {
	OSCallbackFunction function;
	void *context;
} OSCallback;

typedef struct OSMenuItem {
	enum {
		COMMAND, SUBMENU, SEPARATOR,
	} type;

	void *value;
} OSMenuItem;

typedef struct OSMenuSpecification {
	char *name;
	size_t nameBytes;

	OSMenuItem *items;
	size_t itemCount;
} OSMenuSpecification;

typedef struct OSCommand {
#define OS_COMMAND_DYNAMIC (-1)
	int32_t identifier;

	char *label;
	size_t labelBytes;

	char *shortcut;
	size_t shortcutBytes;

	uint8_t checkable : 1, 
		defaultCheck : 1, 
		defaultDisabled : 1,
		dangerous : 1;

	OSCallback callback;
} OSCommand;

typedef struct OSWindowSpecification {
	unsigned width, height;
	unsigned minimumWidth, minimumHeight;

	unsigned flags;

	char *title;
	size_t titleBytes;

	OSMenuSpecification *menubar;
} OSWindowSpecification;

typedef struct OSListViewColumn {
	char *title;
	size_t titleBytes;
#define OS_LIST_VIEW_COLUMN_DEFAULT_WIDTH_PRIMARY (300)
#define OS_LIST_VIEW_COLUMN_DEFAULT_WIDTH_SECONDARY (150)
	int width;
#define OS_LIST_VIEW_COLUMN_PRIMARY (1)
#define OS_LIST_VIEW_COLUMN_RIGHT_ALIGNED (2)
#define OS_LIST_VIEW_COLUMN_ICON (4)
	uint32_t flags;
} OSListViewColumn;

#define OS_CALLBACK_NOT_HANDLED (-1)
#define OS_CALLBACK_HANDLED (0)
#define OS_CALLBACK_DEBUGGER_MESSAGES ((OSObject) 0x800)

#define OS_CREATE_WINDOW_MENU (2)
#define OS_CREATE_WINDOW_NORMAL (4)
#define OS_CREATE_WINDOW_WITH_MENUBAR (8)
#define OS_CREATE_WINDOW_DIALOG (16)

#define OS_ORIENTATION_HORIZONTAL (false)
#define OS_ORIENTATION_VERTICAL   (true)

#define OS_CELL_H_PUSH    (0x0001)
#define OS_CELL_H_EXPAND  (0x0002)
#define OS_CELL_H_LEFT    (0x0004)
#define OS_CELL_H_CENTER  (0x0008)
#define OS_CELL_H_RIGHT   (0x0010)
#define OS_CELL_H_ACROSS  (0x0020)
#define OS_CELL_V_PUSH    (0x0100)
#define OS_CELL_V_EXPAND  (0x0200)
#define OS_CELL_V_TOP     (0x0400)
#define OS_CELL_V_CENTER  (0x0800)
#define OS_CELL_V_BOTTOM  (0x1000)
#define OS_CELL_V_ACROSS  (0x2000)

// Some common layouts...
#define OS_CELL_FILL	  (OS_CELL_H_PUSH | OS_CELL_H_EXPAND | OS_CELL_V_PUSH | OS_CELL_V_EXPAND)
#define OS_CELL_CENTER	  (OS_CELL_H_CENTER | OS_CELL_V_CENTER)

#define OS_CREATE_GRID_NO_BORDER            (1)
#define OS_CREATE_GRID_NO_GAP               (2)
#define OS_CREATE_GRID_DRAW_BOX             (4)
#define OS_CREATE_GRID_MENU	            (8)
#define OS_CREATE_GRID_NO_BACKGROUND	    (16)

#define OS_CREATE_SCROLL_PANE_VERTICAL      (1)
#define OS_CREATE_SCROLL_PANE_HORIZONTAL    (2)

#define OS_CREATE_LIST_VIEW_BORDER          (1)
#define OS_CREATE_LIST_VIEW_SINGLE_SELECT   (2)
#define OS_CREATE_LIST_VIEW_MULTI_SELECT    (4)
#define OS_CREATE_LIST_VIEW_ANY_SELECTIONS  (OS_CREATE_LIST_VIEW_SINGLE_SELECT | OS_CREATE_LIST_VIEW_MULTI_SELECT)

#define OS_SHARED_MEMORY_MAXIMUM_SIZE ((size_t) 1024 * 1024 * 1024 * 1024)
#define OS_SHARED_MEMORY_NAME_MAX_LENGTH (32)
#define OS_SHARED_MEMORY_MAP_ALL (0)
#define OS_MAP_OBJECT_ALL (0)

#define OS_GUI_FONT_REGULAR ((char *) "Shell/Font/RegularGUI")

#define OS_DRAW_STRING_HALIGN_LEFT 	(1)
#define OS_DRAW_STRING_HALIGN_RIGHT 	(2)
#define OS_DRAW_STRING_HALIGN_CENTER 	(OS_DRAW_STRING_HALIGN_LEFT | OS_DRAW_STRING_HALIGN_RIGHT)

#define OS_DRAW_STRING_VALIGN_TOP 	(4)
#define OS_DRAW_STRING_VALIGN_BOTTOM 	(8)
#define OS_DRAW_STRING_VALIGN_CENTER 	(OS_DRAW_STRING_VALIGN_TOP | OS_DRAW_STRING_VALIGN_BOTTOM)

#define OS_OPEN_NODE_READ_NONE		(0x0)
#define OS_OPEN_NODE_READ_BLOCK		(0x1)
#define OS_OPEN_NODE_READ_ACCESS	(0x2)
#define OS_OPEN_NODE_READ_EXCLUSIVE	(0x3)
#define OS_OPEN_NODE_READ_MASK(x)	((x) & 0x7)

#define OS_OPEN_NODE_WRITE_NONE		(0x00)
#define OS_OPEN_NODE_WRITE_BLOCK	(0x10)
#define OS_OPEN_NODE_WRITE_ACCESS	(0x20)
#define OS_OPEN_NODE_WRITE_EXCLUSIVE	(0x30)
#define OS_OPEN_NODE_WRITE_MASK(x)	((x) & 0x70)

#define OS_OPEN_NODE_RESIZE_NONE	(0x000)
#define OS_OPEN_NODE_RESIZE_BLOCK	(0x100)
#define OS_OPEN_NODE_RESIZE_ACCESS	(0x200)
#define OS_OPEN_NODE_RESIZE_EXCLUSIVE	(0x300)
#define OS_OPEN_NODE_RESIZE_MASK(x)	((x) & 0x700)

#define OS_OPEN_NODE_FAIL_IF_FOUND	(0x1000)
#define OS_OPEN_NODE_FAIL_IF_NOT_FOUND	(0x2000)
#define OS_OPEN_NODE_DIRECTORY		(0x4000)
#define OS_OPEN_NODE_CREATE_DIRECTORIES	(0x8000) // Create the directories leading to the file, if they don't already exist.

#define OS_OPEN_SHARED_MEMORY_FAIL_IF_FOUND     (0x1000)
#define OS_OPEN_SHARED_MEMORY_FAIL_IF_NOT_FOUND (0x2000)

#define OS_MAP_OBJECT_READ_WRITE        (0)
#define OS_MAP_OBJECT_READ_ONLY         (1)
#define OS_MAP_OBJECT_COPY_ON_WRITE     (2)

#define OS_CREATE_MENUBAR (1)
#define OS_CREATE_SUBMENU (2)
#define OS_CREATE_MENU_AT_SOURCE (OS_MAKE_POINT(-1, -1))
#define OS_CREATE_MENU_AT_CURSOR (OS_MAKE_POINT(-2, -1))

OS_EXTERN_C void OSInitialiseAPI();

OS_EXTERN_C void OSBatch(OSBatchCall *calls, size_t count); 

OS_EXTERN_C OSError OSCreateProcess(const char *executablePath, size_t executablePathLength, OSProcessInformation *information, void *argument);
OS_EXTERN_C OSError OSCreateThread(OSThreadEntryFunction entryFunction, OSThreadInformation *information, void *argument);
OS_EXTERN_C OSHandle OSCreateSurface(size_t width, size_t height);
OS_EXTERN_C OSHandle OSCreateMutex();
OS_EXTERN_C OSHandle OSCreateEvent(bool autoReset);

OS_EXTERN_C OSError OSCloseHandle(OSHandle handle);

OS_EXTERN_C OSError OSOpenNode(char *path, size_t pathLength, uint64_t flags, OSNodeInformation *information);
OS_EXTERN_C void *OSReadEntireFile(const char *filePath, size_t filePathLength, size_t *fileSize); 
OS_EXTERN_C size_t OSReadFileSync(OSHandle file, uint64_t offset, size_t size, void *buffer); // If return value >= 0, number of bytes read. Otherwise, OSError.
OS_EXTERN_C size_t OSWriteFileSync(OSHandle file, uint64_t offset, size_t size, void *buffer); // If return value >= 0, number of bytes written. Otherwise, OSError.
OS_EXTERN_C OSHandle OSReadFileAsync(OSHandle file, uint64_t offset, size_t size, void *buffer); 
OS_EXTERN_C OSHandle OSWriteFileAsync(OSHandle file, uint64_t offset, size_t size, void *buffer); // TODO Message on completion.
OS_EXTERN_C OSError OSResizeFile(OSHandle file, uint64_t newSize); // TODO What happens if we resize a flie after issuing an asynchronous access?
OS_EXTERN_C void OSRefreshNodeInformation(OSNodeInformation *information);
OS_EXTERN_C OSError OSEnumerateDirectoryChildren(OSHandle directory, OSDirectoryChild *buffer, size_t bufferCount);
OS_EXTERN_C void OSGetIORequestProgress(OSHandle ioRequest, OSIORequestProgress *buffer);
OS_EXTERN_C void OSCancelIORequest(OSHandle ioRequest);
OS_EXTERN_C void OSRemoveNodeFromParent(OSHandle node);

OS_EXTERN_C OSError OSTerminateThread(OSHandle thread);
OS_EXTERN_C OSError OSTerminateProcess(OSHandle thread);
OS_EXTERN_C OSError OSTerminateThisProcess();

OS_EXTERN_C void OSPauseProcess(OSHandle process, bool resume);
OS_EXTERN_C void OSCrashProcess(OSError error);

OS_EXTERN_C uintptr_t OSGetThreadID(OSHandle thread);

OS_EXTERN_C OSError OSReleaseMutex(OSHandle mutex);
OS_EXTERN_C OSError OSAcquireMutex(OSHandle mutex);

OS_EXTERN_C OSError OSSetEvent(OSHandle event);
OS_EXTERN_C OSError OSResetEvent(OSHandle event);
OS_EXTERN_C OSError OSPollEvent(OSHandle event);

OS_EXTERN_C uintptr_t OSWait(OSHandle *objects, size_t objectCount, uintptr_t timeoutMs);
#define OSWaitSingle(object) OSWait(&object, 1, OS_WAIT_NO_TIMEOUT)

OS_EXTERN_C OSHandle OSOpenSharedMemory(size_t size, char *name, size_t nameLength, unsigned flags);
OS_EXTERN_C OSHandle OSShareMemory(OSHandle sharedMemoryRegion, OSHandle targetProcess, bool readOnly);
OS_EXTERN_C void *OSMapObject(OSHandle object, uintptr_t offset, size_t size, unsigned flags);

OS_EXTERN_C void *OSAllocate(size_t size);
OS_EXTERN_C OSError OSFree(void *address);

OS_EXTERN_C void *OSGetCreationArgument(OSHandle object);

OS_EXTERN_C OSError OSGetLinearBuffer(OSHandle surface, OSLinearBuffer *linearBuffer);
OS_EXTERN_C OSError OSInvalidateRectangle(OSHandle surface, OSRectangle rectangle);
OS_EXTERN_C OSError OSCopyToScreen(OSHandle source, OSPoint point, uint16_t depth);
OS_EXTERN_C OSError OSForceScreenUpdate();
OS_EXTERN_C OSError OSFillRectangle(OSHandle surface, OSRectangle rectangle, OSColor color);
OS_EXTERN_C OSError OSCopySurface(OSHandle destination, OSHandle source, OSPoint destinationPoint);
OS_EXTERN_C OSError OSDrawSurface(OSHandle destination, OSHandle source, OSRectangle destinationRegion, OSRectangle sourceRegion, OSRectangle borderRegion, OSDrawMode mode, uint8_t alpha);
OS_EXTERN_C OSError OSDrawSurfaceClipped(OSHandle destination, OSHandle source, OSRectangle destinationRegion, OSRectangle sourceRegion, OSRectangle borderRegion, OSDrawMode mode, uint8_t alpha, OSRectangle clipRegion);
OS_EXTERN_C OSError OSClearModifiedRegion(OSHandle surface);
OS_EXTERN_C OSError OSDrawString(OSHandle surface, OSRectangle region, OSString *string, int fontSize, unsigned flags, uint32_t color, int32_t backgroundColor, bool bold, OSRectangle clipRegion);
OS_EXTERN_C OSError OSFindCharacterAtCoordinate(OSRectangle region, OSPoint coordinate, OSString *string, unsigned flags, OSCaret *position, int fontSize);

OS_EXTERN_C void OSRedrawAll();

#define OS_GRID_PROPERTY_BORDER_SIZE (1)
#define OS_GRID_PROPERTY_GAP_SIZE (2)
OS_EXTERN_C void OSSetProperty(OSObject object, uintptr_t index, void *value);

OS_EXTERN_C OSCallbackResponse OSSendMessage(OSObject target, OSMessage *message);
OS_EXTERN_C OSCallbackResponse OSForwardMessage(OSObject target, OSCallback callback, OSMessage *message);
OS_EXTERN_C OSCallback OSSetCallback(OSObject generator, OSCallback callback); // Returns old callback.
OS_EXTERN_C void OSProcessMessages();

OS_EXTERN_C void OSGetText(OSObject control, OSString *string);
OS_EXTERN_C void OSSetText(OSObject control, char *text, size_t textBytes);
OS_EXTERN_C void OSDisableControl(OSObject control, bool disabled);
#define OSEnableControl(_control, _enabled) OSDisableControl((_control), !(_enabled))
OS_EXTERN_C void OSDisableCommand(OSObject window, OSCommand *command, bool disabled);
#define OSEnableCommand(_window, _command, _enabled) OSDisableCommand((_window), (_command), !(_enabled))
OS_EXTERN_C void OSSetCommandNotificationCallback(OSObject _window, OSCommand *_command, OSCallback callback);
OS_EXTERN_C void OSSetObjectNotificationCallback(OSObject object, OSCallback callback);

OS_EXTERN_C void OSSetInstance(OSObject window, void *instance);
OS_EXTERN_C void *OSGetInstance(OSObject window);
OS_EXTERN_C void *OSGetInstanceFromControl(OSObject object);

OS_EXTERN_C void OSDebugGUIObject(OSObject guiObject);

OS_EXTERN_C OSObject OSCreateMenu(OSMenuSpecification *menuSpecification, OSObject sourceControl, OSPoint position, unsigned flags);
OS_EXTERN_C OSObject OSCreateWindow(OSWindowSpecification *specification);
OS_EXTERN_C OSObject OSCreateGrid(unsigned columns, unsigned rows, unsigned flags);
OS_EXTERN_C OSObject OSCreateScrollPane(OSObject content, unsigned flags);
OS_EXTERN_C void OSAddControl(OSObject grid, unsigned column, unsigned row, OSObject control, unsigned layout);
#define OSAddGrid(_grid, _column, _row, _child, _layout) OSAddControl(_grid, _column, _row, _child, _layout)
#define OSSetRootGrid(_window, _grid) OSAddControl(_window, 0, 0, _grid, 0)

OS_EXTERN_C void OSShowDialogAlert(char *title, size_t titleBytes,
				   char *message, size_t messageBytes,
				   char *description, size_t descriptionBytes,
				   uint16_t iconID, OSObject modalParent);

OS_EXTERN_C void OSGetMousePosition(OSObject relativeWindow, OSPoint *position);

OS_EXTERN_C OSObject OSCreateLine(bool orientation);
OS_EXTERN_C OSObject OSCreateButton(OSCommand *command);
OS_EXTERN_C OSObject OSCreateTextbox(unsigned fontSize);
OS_EXTERN_C OSObject OSCreateLabel(char *label, size_t labelBytes);
OS_EXTERN_C OSObject OSCreateIconDisplay(uint16_t iconID);
OS_EXTERN_C OSObject OSCreateProgressBar(int minimum, int maximum, int initialValue);
OS_EXTERN_C OSObject OSCreateScrollbar(bool orientation);
OS_EXTERN_C OSObject OSCreateListView(unsigned flags);
#define OSCreateIndeterminateProgressBar() OSCreateProgressBar(0, 0, 0)

OS_EXTERN_C void OSSetProgressBarValue(OSObject control, int newValue);

OS_EXTERN_C void OSListViewInsert(OSObject listView, int32_t index, int32_t count);
OS_EXTERN_C void OSListViewReset(OSObject listView);
OS_EXTERN_C void OSListViewInvalidate(OSObject listView, int32_t index, int32_t count);
OS_EXTERN_C void OSListViewSetColumns(OSObject listView, OSListViewColumn *columns, int32_t count);

OS_EXTERN_C void OSSetScrollbarMeasurements(OSObject _scrollbar, int contentSize, int viewportSize);
OS_EXTERN_C void OSSetScrollbarPosition(OSObject _scrollbar, int position, bool sendValueChangedNotification);
OS_EXTERN_C int OSGetScrollbarPosition(OSObject _scrollbar);

#ifndef KERNEL
OS_EXTERN_C void *OSHeapAllocate(size_t size, bool zeroMemory);
OS_EXTERN_C void OSHeapFree(void *address);

OS_EXTERN_C size_t OSCStringLength(char *string);
OS_EXTERN_C void OSCopyMemory(void *destination, void *source, size_t bytes);
OS_EXTERN_C void OSMoveMemory(void *_start, void *_end, intptr_t amount, bool zeroEmptySpace);
OS_EXTERN_C void OSCopyMemoryReverse(void *_destination, void *_source, size_t bytes);
OS_EXTERN_C void OSZeroMemory(void *destination, size_t bytes);
OS_EXTERN_C int OSCompareBytes(void *a, void *b, size_t bytes);
OS_EXTERN_C uint8_t OSSumBytes(uint8_t *data, size_t bytes);
OS_EXTERN_C void OSPrint(const char *format, ...);
OS_EXTERN_C void OSPrintDirect(char *string, size_t stringLength);
OS_EXTERN_C size_t OSFormatString(char *buffer, size_t bufferLength, const char *format, ...);
OS_EXTERN_C void OSHelloWorld();
OS_EXTERN_C uint8_t OSGetRandomByte();
OS_EXTERN_C void OSSort(void *_base, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *argument);

// TODO Possibly remove all of these?
// 	Or move into a libc library?
#ifndef OS_NO_CSTDLIB
OS_EXTERN_C void *memset(void *s, int c, size_t n);
OS_EXTERN_C void *memcpy(void *dest, const void *src, size_t n);
OS_EXTERN_C size_t strlen(const char *s);
OS_EXTERN_C void *malloc(size_t size);
OS_EXTERN_C void *calloc(size_t num, size_t size);
OS_EXTERN_C void *memmove(void *dest, const void *src, size_t n);
OS_EXTERN_C void free(void *ptr);
OS_EXTERN_C void *realloc(void *ptr, size_t size);
OS_EXTERN_C double fabs(double x);
OS_EXTERN_C int abs(int n);
OS_EXTERN_C int ifloor(double x);
OS_EXTERN_C int iceil(double x);
OS_EXTERN_C double sqrt(double x);
OS_EXTERN_C char *getenv(const char *name);
OS_EXTERN_C int strcmp(const char *s1, const char *s2);
OS_EXTERN_C int strcasecmp(const char *s1, const char *s2);
OS_EXTERN_C int strncmp(const char *s1, const char *s2, size_t n);
OS_EXTERN_C int strncasecmp(const char *s1, const char *s2, size_t n);
OS_EXTERN_C int toupper(int c);
OS_EXTERN_C int tolower(int c);
OS_EXTERN_C char *strncpy(char *dest, const char *src, size_t n);
OS_EXTERN_C long int strtol(const char *nptr, char **endptr, int base);
OS_EXTERN_C unsigned long int strtoul(const char *nptr, char **endptr, int base);
OS_EXTERN_C char *strcat(char *dest, const char *src);
OS_EXTERN_C char *strsep(char **stringp, const char *delim);
OS_EXTERN_C char *strstr(const char *haystack, const char *needle);
OS_EXTERN_C char *strrchr(const char *s, int c);
OS_EXTERN_C void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
OS_EXTERN_C char *strcpy(char *dest, const char *src);
OS_EXTERN_C char *stpcpy(char *dest, const char *src);
OS_EXTERN_C char *strchr(const char *s, int c);
OS_EXTERN_C size_t strnlen(const char *s, size_t maxlen);
OS_EXTERN_C size_t strspn(const char *s, const char *accept);
OS_EXTERN_C size_t strcspn(const char *s, const char *reject);
OS_EXTERN_C int memcmp(const void *s1, const void *s2, size_t n);
OS_EXTERN_C void *memchr(const void *s, int c, size_t n);
OS_EXTERN_C int isspace(int c);
OS_EXTERN_C void OSAssertionFailure();
#define assert(x) do{if (!(x)) OSAssertionFailure();}while(0)
#ifdef ARCH_X86_64
typedef struct { uintptr_t rsp, rbp, rbx, r12, r13, r14, r15, returnAddress; } jmp_buf;
#endif
OS_EXTERN_C int _setjmp(jmp_buf *env);
OS_EXTERN_C void _longjmp(jmp_buf *env, int val);
#define setjmp(x) _setjmp(&(x))
#define longjmp(x, y) _longjmp(&(x), (y))
#endif

#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_LINEAR
#include "stb_image.h"

OS_EXTERN_C uint64_t osRandomByteSeed;
OS_EXTERN_C OSHandle osMessageMutex;
#endif

OS_EXTERN_C void ProgramEntry();

#endif
