#include "../api/os.h"

extern "C" void ProgramEntry() {
	{
		// Load the UI theme.

		char *themeFile = (char *) "/os/UISheet.png";
		size_t fileSize;
		uint8_t *loadedFile = (uint8_t *) OSReadEntireFile(themeFile, OSCStringLength(themeFile), &fileSize);
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
		OSFree(loadedFile);
	}

	{
		// Start the test program.
		const char *testProgram = "/os/test";
		OSProcessInformation testProcess;
		OSCreateProcess(testProgram, OSCStringLength((char *) testProgram), &testProcess, nullptr);
	}

	OSTerminateThread(OS_CURRENT_THREAD);
}
