#include "../api/os.h"

// TODO Move some of this into the kernel?
// 	- Ideally, the kernel would still allow us to use system calls.

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
			void *bitmap = OSMapSharedMemory(buffer.handle, 0, buffer.height * buffer.stride);

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
			OSHandle sharedMemory = OSCreateSharedMemory(fileSize, fontName, OSCStringLength(fontName));
			OSCopyMemory(OSMapSharedMemory(sharedMemory, 0, fileSize), loadedFile, fileSize);
			OSHeapFree(loadedFile);
		}
	}

#if 1
	{
		// Load the wallpaper.

		char *wallpaperPath = (char *) "/os/sample_images/Nebula.jpg";
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
				void *bitmap = OSMapSharedMemory(buffer.handle, 0, buffer.height * buffer.stride);

				for (uintptr_t y = 0; y < buffer.height; y++) {
					for (uintptr_t x = 0; x < buffer.width; x++) {
						uint8_t *destination = (uint8_t *) bitmap + y * buffer.stride + x * 4;
						uint8_t *source = image + y * imageX * 4 + x * 4;
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

#if 1
	{
		// Start the Odin test program.

		const char *path = "/os/OdinHello";
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
				OSPrint("The desktop process received a message that another process crashed.\n");
				OSPrint("Error code: %d\n", message.crash.reason.errorCode);
				OSTerminateProcess(message.crash.process);
				OSPauseProcess(message.crash.process, true);
				OSCloseHandle(message.crash.process);
			}
		}
	}
}
