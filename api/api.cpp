#include "os.h"

// Scoped defer: http://www.gingerbill.org/article/defer-in-cpp.html
template <typename F> struct privDefer { F f; privDefer(F f) : f(f) {} ~privDefer() { f(); } };
template <typename F> privDefer<F> defer_func(F f) { return privDefer<F>(f); }
#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define _Defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})
#define Defer(code)   _Defer(code)

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

#ifndef CF
#define CF(x) OS ## x
#endif

enum APIObjectType {
	API_OBJECT_WINDOW,
	API_OBJECT_GRID,
	API_OBJECT_CONTROL,
};

struct APIObject {
	APIObjectType type;
	OSCallback callback;
	APIObject *parent;
};

#include "utf8.h"
#include "heap.cpp"
#include "linked_list.cpp"
#include "font.cpp"
#include "gui.cpp"
#include "common.cpp"
#include "syscall.cpp"

extern "C" void ProgramEntry();

extern "C" void OSInitialiseAPI() {
	// TODO Seed random number generator.

	void OSFPInitialise();
	OSFPInitialise();

	heapMutex = OSCreateMutex();
	printMutex = OSCreateMutex();

	OSInitialiseGUI();
}

extern "C" void _start() {
	OSInitialiseAPI();
	ProgramEntry();
	OSTerminateThread(OS_CURRENT_THREAD);
}

static OSCallback debuggerMessageCallback;

OSCallbackResponse OSSendMessage(OSObject target, OSMessage *message) {
	APIObject *object = (APIObject *) target;
	OSCallback to;

	if (!object) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	if (object == OS_CALLBACK_DEBUGGER_MESSAGES) {
		to = debuggerMessageCallback;
	} else {
		to = object->callback;
	}
	
	if (!to.function) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	message->context = to.context;
	return to.function(target, message);
}

OSCallbackResponse OSForwardMessage(OSObject target, OSCallback callback, OSMessage *message) {
	if (!callback.function) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	message->context = callback.context;
	return callback.function(target, message);
}

void OSProcessMessages() {
	while (true) {
		OSMessage message;
		OSWaitMessage(OS_WAIT_NO_TIMEOUT);

		if (OSGetMessage(&message) == OS_SUCCESS) {
			if (message.context) {
				OSSendMessage(message.context, &message);
				continue;
			}

			switch (message.type) {
				case OS_MESSAGE_PROGRAM_CRASH: {
					message.context = OS_CALLBACK_DEBUGGER_MESSAGES;
					OSSendMessage(message.context, &message);
				} break;

				default: {
					// We don't handle this message.
				} break;
			}
		}
	}
}

OSCallback OSSetCallback(OSObject generator, OSCallback callback) {
	OSCallback old;

	if (generator == OS_CALLBACK_DEBUGGER_MESSAGES) {
		old = debuggerMessageCallback;
		debuggerMessageCallback = callback;
	} else {
		APIObject *object = (APIObject *) generator;
		old = object->callback;
		object->callback = callback;
	}

	return old;
}
