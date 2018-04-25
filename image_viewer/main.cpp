#include "../api/os.h"
#define Defer(x) OSDefer(x)
#include "../api/linked_list.cpp"

#define COMMAND_ROTATE_ANTI_CLOCKWISE (1)
#define COMMAND_ROTATE_CLOCKWISE (2)

#define OS_MANIFEST_DEFINITIONS
#include "../bin/OS/image_viewer.manifest.h"

struct Instance {
	OSObject window,
		 imageDisplay;

	OSHandle imageSurface;
	OSCallback imageDisplayParentCallback;
	int imageWidth, imageHeight;
	float zoom, panX, panY, minimumZoom;
	float minimumPanX, maximumPanX, minimumPanY, maximumPanY;
	float panOffsetX, panOffsetY;
	bool panEnabled;
	bool repaintImageDisplay;

	LinkedItem<Instance> thisItem;

	void Initialise(char *path, size_t pathBytes);

#define ERROR_LOADING_IMAGE (1)
#define ERROR_INTERNAL (2)
#define ERROR_INVALID_IMAGE_FORMAT (3)
#define ERROR_INSUFFICIENT_MEMORY (4)
#define ERROR_ROTATING_IMAGE (5)
	void ReportError(unsigned where, OSError error);
};

struct Global {
	LinkedList<Instance> instances;
};

Global global;

OSCallbackResponse CommandZoom(OSObject object, OSMessage *message) {
	(void) object;

	if (message->type == OS_NOTIFICATION_COMMAND) {
		Instance *instance = (Instance *) OSGetInstanceFromControl(object);

		OSPoint position;
		OSGetMousePosition(nullptr, &position);
		position.x -= 15;
		position.y -= 100 - 14;
		// TODO What happens at the top of the screen?

		OSObject menu = OSCreateMenu(menuZoom, object, position, OS_CREATE_MENU_BLANK);
		OSObject grid = OSCreateGrid(1, 1, OS_GRID_STYLE_BLANK_MENU);

		OSSetRootGrid(menu, grid);

		OSObject slider = OSCreateSlider(0, 100, instance->zoom * 5, OS_SLIDER_MODE_VERTICAL | OS_SLIDER_MODE_OPPOSITE_VALUE, 5, 1);
		OSAddControl(grid, 0, 0, slider, OS_CELL_FILL);
		OSSetObjectNotificationCallback(slider, OS_MAKE_CALLBACK(CommandZoom, instance));
	} else if (message->type == OS_NOTIFICATION_VALUE_CHANGED) {
		Instance *instance = (Instance *) message->context;
		instance->repaintImageDisplay = true;
		instance->zoom = sqrt((float) message->valueChanged.newValue / 5);
		OSRepaintControl(instance->imageDisplay);
	} else {
		return OS_CALLBACK_NOT_HANDLED;
	}

	if (message->type != OS_NOTIFICATION_COMMAND) {
	}

	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse CommandRotate(OSObject object, OSMessage *message) {
	(void) object;

	if (message->type != OS_NOTIFICATION_COMMAND) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	Instance *instance = (Instance *) OSGetInstance(message->command.window);
	instance->repaintImageDisplay = true;
	OSRepaintControl(instance->imageDisplay);

	OSHandle newSurface = OSCreateSurface(instance->imageHeight, instance->imageWidth);

	if (newSurface == OS_INVALID_HANDLE) {
		instance->ReportError(ERROR_ROTATING_IMAGE, ERROR_INSUFFICIENT_MEMORY);
		return OS_CALLBACK_REJECTED;
	}

	OSLinearBuffer buffer; 
	OSGetLinearBuffer(instance->imageSurface, &buffer);
	uint8_t *oldBitmap = (uint8_t *) OSMapObject(buffer.handle, 0, buffer.height * buffer.stride, OS_MAP_OBJECT_READ_WRITE);
	OSCloseHandle(buffer.handle);
	OSCloseHandle(instance->imageSurface);
	size_t oldStride = buffer.stride;
	instance->imageSurface = newSurface;
	OSGetLinearBuffer(newSurface, &buffer);
	uint8_t *newBitmap = (uint8_t *) OSMapObject(buffer.handle, 0, buffer.height * buffer.stride, OS_MAP_OBJECT_READ_WRITE);
	OSCloseHandle(buffer.handle);

	int exchange = instance->imageWidth;
	instance->imageWidth = instance->imageHeight;
	instance->imageHeight = exchange;

	for (int y = 0; y < instance->imageHeight; y++) {
		for (int x = 0; x < instance->imageWidth; x++) {
			uint32_t *destination = (uint32_t *) (newBitmap + y * buffer.stride + x * 4), *source;

			if (message->context == (void *) COMMAND_ROTATE_ANTI_CLOCKWISE) {
				source = (uint32_t *) (oldBitmap + x * oldStride + (instance->imageHeight - y - 1) * 4);
			} else {
				source = (uint32_t *) (oldBitmap + (instance->imageWidth - x - 1) * oldStride + y * 4);
			}

			*destination = *source;
		}
	}

	OSFree(oldBitmap);
	OSFree(newBitmap);

	return OS_CALLBACK_HANDLED;
}

void Instance::ReportError(unsigned where, OSError error) {
	const char *message = "An unknown error occurred.";
	const char *description = "Please try again.";

	switch (where) {
		case ERROR_LOADING_IMAGE: {
			message = "Could not load the image.";
		} break;

		case ERROR_ROTATING_IMAGE: {
			message = "Could not rotate the image.";
		} break;

		case ERROR_INTERNAL: {
			message = "An internal error occurred.";
		} break;
	}

	switch (error) {
		case ERROR_INVALID_IMAGE_FORMAT: {
			message = "The image contained invalid data, or is not supported by Image Viewer.";
		} break;

		case ERROR_INSUFFICIENT_MEMORY: {
			message = "There is not enough memory available.";
		} break;
	}

	OSShowDialogAlert(OSLiteral("Error"), OSLiteral(message), OSLiteral(description), 
			OS_ICON_ERROR, window);
}

OSCallbackResponse DestroyInstance(OSObject object, OSMessage *message) {
	(void) object;
	Instance *instance = (Instance *) message->context;
	global.instances.Remove(&instance->thisItem);
	OSHeapFree(instance);
	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse ProcessImageDisplayMessage(OSObject object, OSMessage *message) {
	(void) object;

	Instance *instance = (Instance *) message->context;
	OSCallbackResponse response = OS_CALLBACK_NOT_HANDLED;

	if (message->type == OS_MESSAGE_PAINT) {
		response = OS_CALLBACK_HANDLED;

		if (message->paint.force || instance->repaintImageDisplay) {
			instance->repaintImageDisplay = false;

			// Fill the background.
			OSFillRectangle(message->paint.surface, message->paint.clip, OSColor(0xE9, 0xF0, 0xFD));

			float zoom = instance->zoom + instance->minimumZoom;

			OSRectangle bounds = OSGetControlBounds(object);
			OSRectangle sourceRegion = OS_MAKE_RECTANGLE(0, instance->imageWidth, 0, instance->imageHeight);
			OSRectangle destinationRegion = OS_MAKE_RECTANGLE(
					0, zoom * instance->imageWidth, 
					0, zoom * instance->imageHeight);

			destinationRegion.left += (bounds.right - bounds.left) / 2;
			destinationRegion.top += (bounds.bottom - bounds.top) / 2;
			destinationRegion.right += (bounds.right - bounds.left) / 2;
			destinationRegion.bottom += (bounds.bottom - bounds.top) / 2;

			destinationRegion.left -= instance->imageWidth * zoom / 2;
			destinationRegion.top -= instance->imageHeight * zoom / 2;
			destinationRegion.right -= instance->imageWidth * zoom / 2;
			destinationRegion.bottom -= instance->imageHeight * zoom / 2;

			instance->minimumPanX = destinationRegion.left;
			instance->minimumPanY = destinationRegion.top;

			if (instance->minimumPanX > 0) instance->minimumPanX = 0;
			if (instance->minimumPanY > 0) instance->minimumPanY = 0;

			instance->panEnabled = instance->minimumPanX || instance->minimumPanY;
			instance->maximumPanX = -instance->minimumPanX;
			instance->maximumPanY = -instance->minimumPanY;

			if (instance->panX < instance->minimumPanX) instance->panX = instance->minimumPanX;
			if (instance->panY < instance->minimumPanY) instance->panY = instance->minimumPanY;
			if (instance->panX > instance->maximumPanX) instance->panX = instance->maximumPanX;
			if (instance->panY > instance->maximumPanY) instance->panY = instance->maximumPanY;

			destinationRegion.left += instance->panX;
			destinationRegion.top += instance->panY;
			destinationRegion.right += instance->panX;
			destinationRegion.bottom += instance->panY;

			destinationRegion.left += bounds.left;
			destinationRegion.top += bounds.top;
			destinationRegion.right += bounds.left;
			destinationRegion.bottom += bounds.top;

			OSDrawSurfaceClipped(message->paint.surface, instance->imageSurface, 
					destinationRegion, sourceRegion, sourceRegion,
					OS_DRAW_MODE_STRECH, 0xFF, message->paint.clip);
		}
	} else if (message->type == OS_MESSAGE_LAYOUT) {
		int displayWidth = message->layout.right - message->layout.left;
		int displayHeight = message->layout.bottom - message->layout.top;

		int imageWidth = instance->imageWidth;
		int imageHeight = instance->imageHeight;

		if (imageWidth <= displayWidth && imageHeight <= displayHeight) {
			instance->minimumZoom = 1.0f;
		} else {
			float aspectRatio = (float) imageHeight / (float) imageWidth;
			int widthToHeight = (int) (displayWidth * aspectRatio);

			if (widthToHeight > displayHeight) {
				instance->minimumZoom = (float) displayHeight / (float) imageHeight;
			} else {
				instance->minimumZoom = (float) displayWidth / (float) imageWidth;
			}
		}
	} else if (message->type == OS_MESSAGE_START_HOVER && instance->panEnabled) {
		OSSetCursor(instance->window, OS_CURSOR_PAN_HOVER);
	} else if (message->type == OS_MESSAGE_START_PRESS && instance->panEnabled) {
		OSSetCursor(instance->window, OS_CURSOR_PAN_DRAG);

		OSPoint mousePosition;
		OSGetMousePosition(instance->window, &mousePosition);

		instance->panOffsetX = instance->panX - mousePosition.x;
		instance->panOffsetY = instance->panY - mousePosition.y;
	} else if (message->type == OS_MESSAGE_MOUSE_DRAGGED && instance->panEnabled) {
		instance->panX = instance->panOffsetX + message->mouseDragged.newPositionX;
		instance->panY = instance->panOffsetY + message->mouseDragged.newPositionY;
		instance->repaintImageDisplay = true;
		OSRepaintControl(instance->imageDisplay);
	} else if (message->type == OS_MESSAGE_END_PRESS && instance->panEnabled) {
		OSSetCursor(instance->window, OS_CURSOR_PAN_HOVER);
	}

	if (response == OS_CALLBACK_NOT_HANDLED) {
		response = OSForwardMessage(object, instance->imageDisplayParentCallback, message);
	}

	return response;
}

void Instance::Initialise(char *path, size_t pathBytes) {
	{
		size_t fileSize;
		uint8_t *loadedFile = (uint8_t *) OSReadEntireFile(path, pathBytes, &fileSize);

		if (!loadedFile) {
			ReportError(ERROR_LOADING_IMAGE, OS_ERROR_UNKNOWN_OPERATION_FAILURE);
			OSHeapFree(this);
			return;
		}

		int imageX, imageY, imageChannels;
		uint8_t *image = stbi_load_from_memory(loadedFile, fileSize, &imageX, &imageY, &imageChannels, 4);
		OSHeapFree(loadedFile);

		imageWidth = imageX;
		imageHeight = imageY;

		if (!image) {
			ReportError(ERROR_LOADING_IMAGE, ERROR_INVALID_IMAGE_FORMAT);
			OSHeapFree(this);
			return;
		}

		imageSurface = OSCreateSurface(imageX, imageY);

		if (imageSurface == OS_INVALID_HANDLE) {
			ReportError(ERROR_LOADING_IMAGE, ERROR_INSUFFICIENT_MEMORY);
			OSHeapFree(this);
			return;
		}

		OSLinearBuffer buffer; 
		OSGetLinearBuffer(imageSurface, &buffer);

		void *bitmap = OSMapObject(buffer.handle, 0, buffer.height * buffer.stride, OS_MAP_OBJECT_READ_WRITE);

		for (int y = 0; y < imageY; y++) {
			for (int x = 0; x < imageX; x++) {
				uint8_t *destination = (uint8_t *) bitmap + y * buffer.stride + x * 4;
				uint8_t *source = image + y * imageX * 4 + x * 4;

				destination[2] = source[0];
				destination[1] = source[1];
				destination[0] = source[2];
				destination[3] = source[3];
			}
		}

		OSFree(bitmap);
		OSCloseHandle(buffer.handle);
		OSHeapFree(image);
	}

	thisItem.thisItem = this;
	global.instances.InsertEnd(&thisItem);

	OSStartGUIAllocationBlock(16384);

	window = OSCreateWindow(mainWindow);
	OSSetInstance(window, this);

	OSSetCommandNotificationCallback(window, osCommandDestroyWindow, OS_MAKE_CALLBACK(DestroyInstance, this));

	OSObject rootLayout = OSCreateGrid(1, 2, OS_GRID_STYLE_LAYOUT);
	OSSetRootGrid(window, rootLayout);

	OSObject toolbar = OSCreateGrid(5, 1, OS_GRID_STYLE_TOOLBAR);
	OSAddControl(rootLayout, 0, 0, toolbar, OS_CELL_H_EXPAND | OS_CELL_V_EXPAND);

	OSAddControl(toolbar, 0, 0, OSCreateButton(commandRotateAntiClockwise, OS_BUTTON_STYLE_TOOLBAR), OS_CELL_V_CENTER | OS_CELL_V_PUSH);
	OSAddControl(toolbar, 1, 0, OSCreateButton(commandRotateClockwise, OS_BUTTON_STYLE_TOOLBAR), OS_CELL_V_CENTER | OS_CELL_V_PUSH);
	OSAddControl(toolbar, 2, 0, OSCreateButton(commandZoom, OS_BUTTON_STYLE_TOOLBAR), OS_CELL_V_CENTER | OS_CELL_V_PUSH);

	imageDisplay = OSCreateBlankControl(0, 0, OS_CURSOR_NORMAL, OS_BLANK_CONTROL_IGNORE_ACTIVATION_CLICKS);
	OSAddControl(rootLayout, 0, 1, imageDisplay, OS_CELL_H_EXPAND | OS_CELL_H_PUSH 
							| OS_CELL_V_EXPAND | OS_CELL_V_PUSH);
	imageDisplayParentCallback = OSSetCallback(imageDisplay, OS_MAKE_CALLBACK(ProcessImageDisplayMessage, this)); 

	OSEnableCommand(window, commandRotateAntiClockwise, true);
	OSEnableCommand(window, commandRotateClockwise, true);
	OSEnableCommand(window, commandZoom, true);

	OSEndGUIAllocationBlock();
}

void ProgramEntry() {
	((Instance *) OSHeapAllocate(sizeof(Instance), true))->Initialise(OSLiteral("/OS/Sample Images/Flower.jpg"));
	OSProcessMessages();
}
