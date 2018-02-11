#include "os.h"

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
};

#include "utf8.h"
#include "heap.cpp"
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
	if (target == nullptr) {
		target = message->generator;
	} 

	APIObject *object = (APIObject *) target;
	OSCallback to;

	if (object == OS_CALLBACK_DEBUGGER_MESSAGES) {
		to = debuggerMessageCallback;
	} else {
		to = object->callback;
	}
	
	if (!to.function) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	message->context = to.context;
	return to.function(message);
}

void OSProcessMessages() {
	while (true) {
		OSMessage message;
		OSWaitMessage(OS_WAIT_NO_TIMEOUT);

		if (OSGetMessage(&message) == OS_SUCCESS) {
			if (message.generator) {
				OSSendMessage(nullptr, &message);
				continue;
			}

			switch (message.type) {
				case OS_MESSAGE_PROGRAM_CRASH: {
					message.generator = OS_CALLBACK_DEBUGGER_MESSAGES;
					OSSendMessage(nullptr, &message);
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
