#include "../api/os.h"

#define COMMAND_NAVIGATE_BACKWARDS (1)
#define COMMAND_NAVIGATE_FORWARDS  (2)
#define COMMAND_NAVIGATE_PARENT    (3)

#define OS_MANIFEST_DEFINITIONS
#include "../bin/os/file_manager.manifest.h"

struct FolderChild {
	OSDirectoryChild data;
	bool selected;
};

struct Instance {
	OSObject folderListing,
		 folderPath,
		 window;

	FolderChild *folderChildren;
	size_t folderChildCount;

	char *path;
	size_t pathBytes;

#define PATH_HISTORY_MAX (64)
	char *pathBackwardHistory[PATH_HISTORY_MAX];
	size_t pathBackwardHistoryBytes[PATH_HISTORY_MAX];
	uintptr_t pathBackwardHistoryPosition;
	char *pathForwardHistory[PATH_HISTORY_MAX];
	size_t pathForwardHistoryBytes[PATH_HISTORY_MAX];
	uintptr_t pathForwardHistoryPosition;

#define LOAD_FOLDER_BACKWARDS (1)
#define LOAD_FOLDER_FORWARDS (2)
	void LoadFolder(char *path, size_t pathBytes, 
			char *path2 = nullptr, size_t pathBytes2 = 0,
			unsigned historyMode = 0);

#define ERROR_CANNOT_LOAD_FOLDER (0)
	void ReportError(unsigned where, OSError error);
};

OSListViewColumn folderListingColumns[] = {
#define COLUMN_NAME (0)
	{ OSLiteral("Name"), 270, OS_LIST_VIEW_COLUMN_PRIMARY | OS_LIST_VIEW_COLUMN_ICON, },
#define COLUMN_DATE_MODIFIED (1)
	{ OSLiteral("Date modified"), 120, OS_FLAGS_DEFAULT, },
#define COLUMN_TYPE (2)
	{ OSLiteral("Type"), 120, OS_FLAGS_DEFAULT, },
#define COLUMN_SIZE (3)
	{ OSLiteral("Size"), 100, OS_LIST_VIEW_COLUMN_RIGHT_ALIGNED, },
};

int SortFolder(const void *_a, const void *_b, void *argument) {
	(void) argument;

	FolderChild *a = (FolderChild *) _a;
	FolderChild *b = (FolderChild *) _b;

	if (a->data.information.type == OS_NODE_FILE && b->data.information.type == OS_NODE_DIRECTORY) {
		return 1;
	} else if (b->data.information.type == OS_NODE_FILE && a->data.information.type == OS_NODE_DIRECTORY) {
		return -1;
	}

	char *s1 = a->data.name;
	char *s2 = b->data.name;
	size_t length1 = a->data.nameLengthBytes;
	size_t length2 = b->data.nameLengthBytes;

	while (length1 || length2) {
		if (!length1) return -1;
		if (!length2) return 1;

		char c1 = *s1;
		char c2 = *s2;

		if (c1 >= 'a' && c1 <= 'z') c1 = c1 - 'a' + 'A';
		if (c2 >= 'a' && c2 <= 'z') c2 = c2 - 'a' + 'A';

		if (c1 != c2) {
			return c1 - c2;
		}

		s1++;
		s2++;
		length1--;
		length2--;
	}

	return 0;
}

OSCallbackResponse CommandNavigate(OSObject object, OSMessage *message) {
	Instance *instance = (Instance *) OSGetInstance(message->command.window);
	(void) object;

	switch ((uintptr_t) message->context) {
		case COMMAND_NAVIGATE_BACKWARDS: {
			instance->pathBackwardHistoryPosition--;
			instance->LoadFolder(instance->pathBackwardHistory[instance->pathBackwardHistoryPosition],
					instance->pathBackwardHistoryBytes[instance->pathBackwardHistoryPosition],
					nullptr, 0, LOAD_FOLDER_BACKWARDS);
		} break;

		case COMMAND_NAVIGATE_FORWARDS: {
			instance->pathForwardHistoryPosition--;
			instance->LoadFolder(instance->pathForwardHistory[instance->pathForwardHistoryPosition],
					instance->pathForwardHistoryBytes[instance->pathForwardHistoryPosition],
					nullptr, 0, LOAD_FOLDER_FORWARDS);
		} break;

		case COMMAND_NAVIGATE_PARENT: {
			size_t s = instance->pathBytes;

			while (true) {
				if (instance->path[--s] == '/') {
					break;
				}
			}

			if (!s) s++;

			instance->LoadFolder(instance->path, s);
		} break;
	}

	OSEnableCommand(instance->window, commandNavigateBackwards, instance->pathBackwardHistoryPosition);
	OSEnableCommand(instance->window, commandNavigateForwards, instance->pathForwardHistoryPosition);

	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse ProcessFolderPathNotification(OSObject object, OSMessage *message) {
	Instance *instance = (Instance *) message->context;
	(void) object;
	
	switch (message->type) {
		case OS_NOTIFICATION_CANCEL_EDIT: {
			OSSetText(object, instance->path, instance->pathBytes);
			return OS_CALLBACK_HANDLED;
		} break;

		case OS_NOTIFICATION_CONFIRM_EDIT: {
			OSString string;
			OSGetText(object, &string);

			if (!string.bytes) {
				OSSetText(object, instance->path, instance->pathBytes);
			} else {
				instance->LoadFolder(string.buffer, string.bytes);
			}

			return OS_CALLBACK_HANDLED;
		} break;

		default: {
			return OS_CALLBACK_NOT_HANDLED;
		} break;
	}
}

OSCallbackResponse ProcessFolderListingNotification(OSObject object, OSMessage *message) {
	Instance *instance = (Instance *) message->context;
	(void) object;
	
	switch (message->type) {
		case OS_NOTIFICATION_GET_ITEM: {
			uintptr_t index = message->listViewItem.index;
			FolderChild *child = instance->folderChildren + index;
			OSDirectoryChild *data = &child->data;

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_TEXT) {
#define BUFFER_SIZE (1024)
				static char buffer[BUFFER_SIZE];

				switch (message->listViewItem.column) {
					case COLUMN_NAME: {
						message->listViewItem.textBytes = OSFormatString(buffer, BUFFER_SIZE, 
								"%s", data->nameLengthBytes, data->name);
					} break;

					case COLUMN_DATE_MODIFIED: {
						message->listViewItem.textBytes = 0;
					} break;

					case COLUMN_TYPE: {
						message->listViewItem.textBytes = OSFormatString(buffer, BUFFER_SIZE, 
								"%z", data->information.type == OS_NODE_FILE ? "File" : "Directory");
					} break;

					case COLUMN_SIZE: {
						message->listViewItem.textBytes = 0;

						if (data->information.type == OS_NODE_FILE) {
							int fileSize = data->information.fileSize;

							if (fileSize < 1000) {
								message->listViewItem.textBytes = OSFormatString(buffer, BUFFER_SIZE, 
										"%d bytes", fileSize);
							} else if (fileSize < 1000000) {
								message->listViewItem.textBytes = OSFormatString(buffer, BUFFER_SIZE, 
										"%d.%d KB", fileSize / 1000, (fileSize / 100) % 10);
							} else if (fileSize < 1000000000) {
								message->listViewItem.textBytes = OSFormatString(buffer, BUFFER_SIZE, 
										"%d.%d MB", fileSize / 1000000, (fileSize / 1000000) % 10);
							} else {
								message->listViewItem.textBytes = OSFormatString(buffer, BUFFER_SIZE, 
										"%d.%d GB", fileSize / 1000000000, (fileSize / 1000000000) % 10);
							}
						} else if (data->information.type == OS_NODE_DIRECTORY) {
							message->listViewItem.textBytes = OSFormatString(buffer, BUFFER_SIZE, 
									"%d files", data->information.directoryChildren);
						}
					} break;
				}
#undef BUFFER_SIZE

				message->listViewItem.text = buffer;
			}

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_SELECTED) {
				if (child->selected) {
					message->listViewItem.state |= OS_LIST_VIEW_ITEM_SELECTED;
				}
			}

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_ICON) {
				message->listViewItem.iconID = data->information.type == OS_NODE_DIRECTORY ? OS_ICON_FOLDER : OS_ICON_FILE;
			}

			return OS_CALLBACK_HANDLED;
		} break;

		case OS_NOTIFICATION_DESELECT_ALL: {
			for (uintptr_t i = 0; i < instance->folderChildCount; i++) {
				instance->folderChildren[i].selected = false;
			}

			return OS_CALLBACK_HANDLED;
		} break;

		case OS_NOTIFICATION_SET_ITEM: {
			uintptr_t index = message->listViewItem.index;
			FolderChild *child = instance->folderChildren + index;

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_SELECTED) {
				child->selected = message->listViewItem.state & OS_LIST_VIEW_ITEM_SELECTED;
			}

			return OS_CALLBACK_HANDLED;
		} break;

		case OS_NOTIFICATION_CHOOSE_ITEM: {
			uintptr_t index = message->listViewItem.index;
			FolderChild *child = instance->folderChildren + index;
			OSDirectoryChild *data = &child->data;

			if (data->information.type == OS_NODE_FILE) {
				// TODO Opening files.
			} else if (data->information.type == OS_NODE_DIRECTORY) {
				char *existingPath = instance->path;
				size_t existingPathBytes = instance->pathBytes;

				char *folderName = data->name;
				size_t folderNameBytes = data->nameLengthBytes;

				instance->LoadFolder(existingPath, existingPathBytes, folderName, folderNameBytes);
			}

			return OS_CALLBACK_HANDLED;
		} break;

		default: {
			return OS_CALLBACK_NOT_HANDLED;
		} break;
	}
}

void Instance::ReportError(unsigned where, OSError error) {
#if 0
#define OS_ERROR_PATH_NOT_TRAVERSABLE		(-15)
#define OS_ERROR_FILE_DOES_NOT_EXIST		(-20)
#define OS_ERROR_FILE_PERMISSION_NOT_GRANTED	(-23)
#define OS_ERROR_INCORRECT_NODE_TYPE		(-26)
#define OS_ERROR_DRIVE_CONTROLLER_REPORTED	(-35)
#endif

	OSPrint("%d, %d\n", where, error);
	// TODO Error dialog boxes.
}

void Instance::LoadFolder(char *path1, size_t pathBytes1, char *path2, size_t pathBytes2, unsigned historyMode) {
	char *oldPath = path;
	size_t oldPathBytes = pathBytes;

	{
		goto normal;
		fail:;
		OSSetText(folderPath, oldPath, oldPathBytes);
		return;
		normal:;
	}

	// Fix the paths.
	if (path2 || pathBytes1 != 1) {
		if (path1[pathBytes1 - 1] == '/') pathBytes1--;
		if (path2 && path2[pathBytes2 - 1] == '/') pathBytes2--;
	}

	// Create the path.
	char *newPath = (char *) OSHeapAllocate(pathBytes1 + (path2 ? (pathBytes2 + 1) : 0), false);
	OSCopyMemory(newPath, path1, pathBytes1);
	size_t newPathBytes = pathBytes1;

	if (path2) {
		newPath[newPathBytes] = '/';
		OSCopyMemory(newPath + newPathBytes + 1, path2, pathBytes2);
		newPathBytes += pathBytes2 + 1;
	}

	// Is there a parent?
	OSEnableCommand(window, commandNavigateParent, pathBytes1 != 1);

	OSNodeInformation node;
	OSError error;

	// Open the directory.
	error = OSOpenNode(newPath, newPathBytes, OS_OPEN_NODE_DIRECTORY | OS_OPEN_NODE_FAIL_IF_NOT_FOUND, &node);

	if (error != OS_SUCCESS) {
		ReportError(ERROR_CANNOT_LOAD_FOLDER, error);
		goto fail;
	}

	OSDefer(OSCloseHandle(node.handle));

	// Get the directory's children.
	size_t childCount = node.directoryChildren + 1024 /*Just in case extra files are created between OSOpenNode and here.*/;
	OSDirectoryChild *children = (OSDirectoryChild *) OSHeapAllocate(childCount * sizeof(OSDirectoryChild), true);
	OSDefer(OSHeapFree(children));
	error = OSEnumerateDirectoryChildren(node.handle, children, childCount);

	if (error != OS_SUCCESS) {
		ReportError(ERROR_CANNOT_LOAD_FOLDER, error);
		goto fail;
	}

	// Work out how many there were.
	for (uintptr_t i = 0; i < childCount; i++) {
		if (!children[i].information.present) {
			childCount = i;
			break;
		}
	}

	// Allocate memory to store the children.
	OSHeapFree(folderChildren);
	folderChildren = (FolderChild *) OSHeapAllocate(childCount * sizeof(FolderChild), true);
	folderChildCount = childCount;

	// Copy across the data.
	for (uintptr_t i = 0; i < childCount; i++) {
		OSCopyMemory(&folderChildren[i].data, children + i, sizeof(OSDirectoryChild));
	}

	// Sort the folder.
	OSSort(folderChildren, folderChildCount, sizeof(FolderChild), SortFolder, nullptr);

	// Confirm the new path.
	path = newPath;
	pathBytes = newPathBytes;

	// Update the UI.
	OSListViewReset(folderListing);
	OSListViewInsert(folderListing, 0, childCount);
	OSSetText(folderPath, path, pathBytes);

	// Add the previous folder to the history.
	if (oldPath) {
		char **history = historyMode == LOAD_FOLDER_BACKWARDS ? pathForwardHistory : pathBackwardHistory;
		size_t *historyBytes = historyMode == LOAD_FOLDER_BACKWARDS ? pathForwardHistoryBytes : pathBackwardHistoryBytes;
		uintptr_t &historyPosition = historyMode == LOAD_FOLDER_BACKWARDS ? pathForwardHistoryPosition : pathBackwardHistoryPosition;

		if (historyPosition == PATH_HISTORY_MAX) {
			OSHeapFree(history[0]);
			OSCopyMemoryReverse(history, history + 1, sizeof(char *) * (PATH_HISTORY_MAX - 1));
		}

		history[historyPosition] = oldPath;
		historyBytes[historyPosition] = oldPathBytes;
		historyPosition++;

		OSEnableCommand(window, historyMode == LOAD_FOLDER_BACKWARDS ? commandNavigateForwards : commandNavigateBackwards, true);

		// If this was a normal navigation, clear the forward history.
		if (!historyMode) {
			while (pathForwardHistoryPosition) {
				OSHeapFree(pathForwardHistory[--pathForwardHistoryPosition]);
			}

			OSEnableCommand(window, commandNavigateForwards, false);
		}
	}
}

void ProgramEntry() {
	Instance *instance = (Instance *) OSHeapAllocate(sizeof(Instance), true);

	OSObject window = OSCreateWindow(mainWindow);
	OSSetInstance(window, instance);
	instance->window = window;

	OSObject layout1 = OSCreateGrid(1, 3, OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_GAP);
	OSObject layout2 = OSCreateGrid(2, 1, OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_GAP);
	OSObject layout3 = OSCreateGrid(4, 1, OS_FLAGS_DEFAULT);

	OSSetRootGrid(window, layout1);
	OSAddGrid(layout1, 0, 2, layout2, OS_CELL_FILL);
	OSAddGrid(layout1, 0, 0, layout3, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);

	OSSetProperty(layout3, OS_GRID_PROPERTY_BORDER_SIZE, (void *) 2);
	OSSetProperty(layout3, OS_GRID_PROPERTY_GAP_SIZE, (void *) 2);

	instance->folderListing = OSCreateListView(OS_CREATE_LIST_VIEW_MULTI_SELECT);
	OSAddControl(layout2, 1, 0, instance->folderListing, OS_CELL_FILL);
	OSListViewSetColumns(instance->folderListing, folderListingColumns, sizeof(folderListingColumns) / sizeof(folderListingColumns[0]));
	OSSetObjectNotificationCallback(instance->folderListing, OS_MAKE_CALLBACK(ProcessFolderListingNotification, instance));

	OSAddControl(layout1, 0, 1, OSCreateLine(OS_ORIENTATION_HORIZONTAL), OS_CELL_H_EXPAND | OS_CELL_H_PUSH);

	OSObject backButton = OSCreateButton(commandNavigateBackwards);
	OSAddControl(layout3, 0, 0, backButton, OS_FLAGS_DEFAULT);
	OSObject forwardButton = OSCreateButton(commandNavigateForwards);
	OSAddControl(layout3, 1, 0, forwardButton, OS_FLAGS_DEFAULT);
	OSObject parentButton = OSCreateButton(commandNavigateParent);
	OSAddControl(layout3, 2, 0, parentButton, OS_FLAGS_DEFAULT);

	instance->folderPath = OSCreateTextbox(0);
	OSAddControl(layout3, 3, 0, instance->folderPath, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);
	OSSetObjectNotificationCallback(instance->folderPath, OS_MAKE_CALLBACK(ProcessFolderPathNotification, instance));

	instance->LoadFolder(OSLiteral("/os/"));

	OSProcessMessages();
}
