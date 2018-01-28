#include "../api/os.h"

char *errorMessages[] = {
	(char *) "INVALID_BUFFER",
	(char *) "UNKNOWN_SYSCALL",
	(char *) "INVALID_MEMORY_REGION",
	(char *) "MEMORY_REGION_LOCKED_BY_KERNEL",
	(char *) "PATH_LENGTH_EXCEEDS_LIMIT",
	(char *) "INVALID_HANDLE",
	(char *) "MUTEX_NOT_ACQUIRED_BY_THREAD",
	(char *) "MUTEX_ALREADY_ACQUIRED",
	(char *) "BUFFER_NOT_ACCESSIBLE",
	(char *) "SHARED_MEMORY_REGION_TOO_LARGE",
	(char *) "SHARED_MEMORY_STILL_MAPPED",
	(char *) "COULD_NOT_LOAD_FONT",
	(char *) "COULD_NOT_DRAW_FONT",
	(char *) "COULD_NOT_ALLOCATE_MEMORY",
	(char *) "INCORRECT_FILE_ACCESS",
	(char *) "TOO_MANY_WAIT_OBJECTS",
	(char *) "INCORRECT_NODE_TYPE",
	(char *) "PROCESSOR_EXCEPTION",
	(char *) "INVALID_PANE_CHILD",
	(char *) "INVALID_PANE_OBJECT",
	(char *) "UNSUPPORTED_CALLBACK",
	(char *) "MISSING_CALLBACK",
	(char *) "UNKNOWN",
};

// TODO Move some of this into the kernel?
// 	- Ideally, the kernel would still allow us to use system calls.

void CloseDialog(OSObject generator, void *argument, OSCallbackData *data) {
	(void) generator;
	(void) data;
	
	OSCloseWindow((OSObject) argument); // TODO This crashes because we try to return with a closed window.
}

extern "C" void ProgramEntry() {
	{
		// Load the UI theme.

		char *themeFile = (char *) "/os/UISheet.png";
		size_t fileSize;
		uint8_t *loadedFile = (uint8_t *) OSReadEntireFile(themeFile, OSCStringLength(themeFile), &fileSize);

		if (!loadedFile) {
			OSPrint("Error: Could not load the UI sheet.\n");
		} else {
			int imageX, imageY, imageChannels;
			uint8_t *image = stbi_load_from_memory(loadedFile, fileSize, &imageX, &imageY, &imageChannels, 4);

			OSHandle surface = OS_SURFACE_UI_SHEET;
			OSLinearBuffer buffer; OSGetLinearBuffer(surface, &buffer);
			void *bitmap = OSMapObject(buffer.handle, 0, buffer.height * buffer.stride);

			for (intptr_t y = 0; y < imageY; y++) {
				for (intptr_t x = 0; x < imageX; x++) {
					uint8_t *destination = (uint8_t *) bitmap + y * buffer.stride + x * 4;
					uint8_t *source = image + y * imageX * 4 + x * 4;
					destination[2] = source[0];
					destination[1] = source[1];
					destination[0] = source[2];
					destination[3] = source[3];
				}
			}

			OSInvalidateRectangle(surface, OSRectangle(0, buffer.width, 0, buffer.height));
			OSHeapFree(image);
			OSHeapFree(loadedFile);
			OSFree(bitmap);
		}

		OSRedrawAll();
	}

	{
		// Load the GUI font.
		// TODO Remove this when we have a proper file cache.

		char *fontFile = (char *) "/os/source_sans/regular.ttf";
		size_t fileSize;
		void *loadedFile = OSReadEntireFile(fontFile, OSCStringLength(fontFile), &fileSize);

		if (!loadedFile) {
			OSPrint("Error: Could not load the font file.\n");
		} else {
			char *fontName = OS_GUI_FONT_REGULAR;
			OSHandle sharedMemory = OSOpenSharedMemory(fileSize, fontName, OSCStringLength(fontName), OS_OPEN_SHARED_MEMORY_FAIL_IF_FOUND);
			OSCopyMemory(OSMapObject(sharedMemory, 0, fileSize), loadedFile, fileSize);
			OSHeapFree(loadedFile);
		}
	}

#if 1
	{
		// Load the wallpaper.

		char *wallpaperPath = (char *) "/os/sample_images/Flower.jpg";
		size_t fileSize;
		uint8_t *loadedFile = (uint8_t *) OSReadEntireFile(wallpaperPath, OSCStringLength(wallpaperPath), &fileSize);

		if (!loadedFile) {
			OSPrint("Error: Could not load the wallpaper.\n");
		} else {
			int imageX, imageY, imageChannels;
			uint8_t *image = stbi_load_from_memory(loadedFile, fileSize, &imageX, &imageY, &imageChannels, 4);

			if (!image) {
				OSPrint("Error: Could not load the wallpaper.\n");
			} else {
				OSHandle surface = OS_SURFACE_WALLPAPER;
				OSLinearBuffer buffer; OSGetLinearBuffer(surface, &buffer);
				void *bitmap = OSMapObject(buffer.handle, 0, buffer.height * buffer.stride);
				int xOffset = 0, yOffset = 0;

				if (imageX > (int) buffer.width) {
					xOffset = imageX / 2 - buffer.width / 2;
				}

				if (imageY > (int) buffer.height) {
					yOffset = imageY / 2 - buffer.height / 2;
				}

				for (uintptr_t y = 0; y < buffer.height; y++) {
					for (uintptr_t x = 0; x < buffer.width; x++) {
						uint8_t *destination = (uint8_t *) bitmap + y * buffer.stride + x * 4;
						uint8_t *source = image + (y + yOffset) * imageX * 4 + (x + xOffset) * 4;
						destination[2] = source[0];
						destination[1] = source[1];
						destination[0] = source[2];
						destination[3] = source[3];
					}
				}

				OSInvalidateRectangle(surface, OSRectangle(0, imageX, 0, imageY));
				OSHeapFree(image);
				OSFree(bitmap);
			}

			OSHeapFree(loadedFile);
		}

		OSRedrawAll();
	}
#else
	OSHandle surface = OS_SURFACE_WALLPAPER;
	OSLinearBuffer buffer; OSGetLinearBuffer(surface, &buffer);
	OSFillRectangle(surface, OSRectangle(0, buffer.width, 0, buffer.height), OSColor(0, 128, 128));
	OSRedrawAll();
#endif

#if 1
	{
		// Start the calculator test program.

		for (int i = 0; i < 1; i++) {
			const char *path = "/os/calculator";
			OSProcessInformation process;
			OSCreateProcess(path, OSCStringLength((char *) path), &process, nullptr);
			OSCloseHandle(process.mainThread.handle);
			OSCloseHandle(process.handle);
		}
	}
#endif

#if 0
	{
		// Start the Odin test program.

		const char *path = "/os/essence_gui";
		OSProcessInformation process;
		OSCreateProcess(path, OSCStringLength((char *) path), &process, nullptr);
	}
#endif

	while (true) {
		OSMessage message;
		OSWaitMessage(OS_WAIT_NO_TIMEOUT);

		if (OSGetMessage(&message) == OS_SUCCESS) {
			if (OS_SUCCESS == OSProcessGUIMessage(&message)) {
				continue;
			} else if (message.type == OS_MESSAGE_PROGRAM_CRASH) {
				int code = message.crash.reason.errorCode;
				OSPrint("The desktop process received a message that another process crashed.\n");
				OSPrint("Error code: %d\n", code);
				OSTerminateProcess(message.crash.process);
				OSPauseProcess(message.crash.process, true);
				OSCloseHandle(message.crash.process);

				OSObject window = OSCreateWindow((char *) "Program Crash", 13, 300, 70, 0);
				OSObject contentPane = OSGetWindowContentPane(window);
				OSConfigurePane(contentPane, 1, 2, 0);

				char crashMessage[256];
				size_t crashMessageLength;
				
				if (code < OS_FATAL_ERROR_COUNT) {
					crashMessageLength = OSFormatString(crashMessage, 256, "Error code: %d (%s)", code, OSCStringLength(errorMessages[code]), errorMessages[code]);
				} else {
					crashMessageLength = OSFormatString(crashMessage, 256, "Error code: %d (user error)", message.crash.reason.errorCode);
				}

				OSObject message = OSCreateControl(OS_CONTROL_STATIC, crashMessage, crashMessageLength, 0);
				OSSetPaneObject(OSGetPane(contentPane, 0, 0), message, OS_SET_PANE_OBJECT_HORIZONTAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_PUSH | OS_SET_PANE_OBJECT_VERTICAL_TOP);

				OSObject button = OSCreateControl(OS_CONTROL_BUTTON, (char *) "OK", 2, 0);
				OSSetPaneObject(OSGetPane(contentPane, 0, 1), button, OS_SET_PANE_OBJECT_HORIZONTAL_CENTER);
				OSSetObjectCallback(button, OS_OBJECT_CONTROL, OS_CALLBACK_ACTION, CloseDialog, window);
			}
		}
	}
}
