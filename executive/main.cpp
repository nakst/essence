#include "../api/os.h"

// TODO Move all of this into the kernel?
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

			OSHandle surface = OSCreateSurface(imageX, imageY);
			OSLinearBuffer buffer; OSGetLinearBuffer(surface, &buffer);

			for (intptr_t y = 0; y < imageY; y++) {
				for (intptr_t x = 0; x < imageX; x++) {
					uint8_t *destination = (uint8_t *) buffer.buffer + y * buffer.stride + x * 4;
					uint8_t *source = image + y * imageX * 4 + x * 4;
					destination[2] = source[0];
					destination[1] = source[1];
					destination[0] = source[2];
					destination[3] = source[3];
				}
			}

			OSInvalidateRectangle(OS_SURFACE_UI_SHEET, OSRectangle(0, imageX, 0, imageY));
			OSCopySurface(OS_SURFACE_UI_SHEET, surface, OSPoint(0, 0));

			free(image);
			OSHeapFree(loadedFile);
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
			// TODO Reference counting of the shared memory doesn't seem to work properly?
			OSHandle sharedMemory = OSCreateSharedMemory(fileSize, fontName, OSCStringLength(fontName));
			OSCopyMemory(OSMapSharedMemory(sharedMemory, 0, fileSize), loadedFile, fileSize);
			OSHeapFree(loadedFile);
		}
	}

#if 1
	{
		// Start the calculator test program.

		for (int i = 0; i < 10; i++) {
			const char *path = "/os/calculator";
			OSProcessInformation process;
			OSCreateProcess(path, OSCStringLength((char *) path), &process, nullptr);
		}
	}
#endif

#if 0
	{
		// Start the Odin test program.

		const char *path = "/os/OdinHello";
		OSProcessInformation process;
		OSCreateProcess(path, OSCStringLength((char *) path), &process, nullptr);
	}
#endif

	OSTerminateThread(OS_CURRENT_THREAD);
}
