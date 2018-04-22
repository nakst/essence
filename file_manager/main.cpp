#include "../api/os.h"
#define Defer(x) OSDefer(x)
#include "../api/linked_list.cpp"

#define COMMAND_NAVIGATE_BACKWARDS (1)
#define COMMAND_NAVIGATE_FORWARDS  (2)
#define COMMAND_NAVIGATE_PARENT    (3)
#define COMMAND_NAVIGATE_PATH	   (4)

#define COMMAND_NEW_FOLDER         (1)

#define OS_MANIFEST_DEFINITIONS
#include "../bin/OS/file_manager.manifest.h"

// TODO Wrong size reported for Sample Images' contents?

struct FolderChild {
	OSDirectoryChild data;
	bool selected;
};

struct Instance {
	OSObject folderListing,
		 folderPath,
		 statusLabel,
		 window,
		 bookmarkList;

	FolderChild *folderChildren;
	size_t folderChildCount;

	char *path;
	size_t pathBytes;

	LinkedItem<Instance> thisItem;

#define PATH_HISTORY_MAX (64)
	char *pathBackwardHistory[PATH_HISTORY_MAX];
	size_t pathBackwardHistoryBytes[PATH_HISTORY_MAX];
	uintptr_t pathBackwardHistoryPosition;
	char *pathForwardHistory[PATH_HISTORY_MAX];
	size_t pathForwardHistoryBytes[PATH_HISTORY_MAX];
	uintptr_t pathForwardHistoryPosition;

	void Initialise();

#define LOAD_FOLDER_BACKWARDS (1)
#define LOAD_FOLDER_FORWARDS (2)
#define LOAD_FOLDER_NO_HISTORY (3)
	bool LoadFolder(char *path, size_t pathBytes, 
			char *path2 = nullptr, size_t pathBytes2 = 0,
			unsigned historyMode = 0);

#define ERROR_CANNOT_LOAD_FOLDER (0)
#define ERROR_CANNOT_CREATE_FOLDER (1)
#define ERROR_INTERNAL (2)
	void ReportError(unsigned where, OSError error);
};

struct Bookmark {
	char *path;
	size_t pathBytes;
};

struct Global {
	Bookmark *bookmarks;
	size_t bookmarkCount, bookmarkAllocated;

	LinkedList<Instance> instances;

	void AddBookmark(char *path, size_t pathBytes);
	bool RemoveBookmark(char *path, size_t pathBytes);
};

Global global;

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

#define GUI_STRING_BUFFER_LENGTH (1024)
char guiStringBuffer[GUI_STRING_BUFFER_LENGTH];

int64_t ParseIntegerFromString(char **string, size_t *length, int base) {
	int64_t value = 0;
	bool overflow = false;

	while (*length) {
		char c = (*string)[0];

		if (c >= '0' && c <= '9') {
			int64_t digit = c - '0';
			int64_t oldValue = value;

			value *= base;
			value += digit;

			if (value / base != oldValue) {
				overflow = true;
			}

			(*string)++;
			(*length)--;
		} else {
			break;
		}
	}

	if (overflow) value = LONG_MAX;
	return value;
}

int CompareStrings(char *s1, char *s2, size_t length1, size_t length2) {
	while (length1 || length2) {
		if (!length1) return -1;
		if (!length2) return 1;

		char c1 = *s1;
		char c2 = *s2;

		if (c1 >= '0' && c1 <= '9' && c2 >= '0' && c2 <= '9') {
			int64_t n1 = ParseIntegerFromString(&s1, &length1, 10);
			int64_t n2 = ParseIntegerFromString(&s2, &length2, 10);

			if (n1 != n2) {
				return n1 - n2;
			}
		} else {
			if (c1 >= 'a' && c1 <= 'z') c1 = c1 - 'a' + 'A';
			if (c2 >= 'a' && c2 <= 'z') c2 = c2 - 'a' + 'A';
			if (c1 == '.') c1 = ' '; else if (c1 == ' ') c1 = '.';
			if (c2 == '.') c2 = ' '; else if (c2 == ' ') c2 = '.';

			if (c1 != c2) {
				return c1 - c2;
			}

			length1--;
			length2--;
			s1++;
			s2++;
		}
	}

	return 0;
}

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

	return CompareStrings(s1, s2, length1, length2);
}

OSCallbackResponse CommandNew(OSObject object, OSMessage *message) {
	(void) object;

	if (message->type != OS_NOTIFICATION_COMMAND) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	Instance *instance = (Instance *) OSGetInstance(message->command.window);

	switch ((uintptr_t) message->context) {
		case COMMAND_NEW_FOLDER: {
			size_t length;
			uintptr_t attempt = 1;

			while (attempt < 1000) {
				if (attempt == 1) {
					length = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "New folder");
				} else {
					length = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "New folder %d", attempt);
				}

				for (uintptr_t i = 0; i < instance->folderChildCount; i++) {
					FolderChild *child = instance->folderChildren + i;

					if (CompareStrings(child->data.name, guiStringBuffer, child->data.nameLengthBytes, length) == 0) {
						goto nextAttempt;
					}
				}

				break;
				nextAttempt:;
				attempt++;
			}

			bool addSeparator = instance->pathBytes > 1;
			size_t fullPathLength = instance->pathBytes + length + (addSeparator ? 1 : 0);
			char *fullPath = (char *) OSHeapAllocate(fullPathLength, false);
			OSCopyMemory(fullPath, instance->path, instance->pathBytes);
			if (addSeparator) fullPath[instance->pathBytes] = '/';
			OSCopyMemory(fullPath + instance->pathBytes + (addSeparator ? 1 : 0), guiStringBuffer, length);

			OSNodeInformation node;
			OSError error = OSOpenNode(fullPath, fullPathLength, OS_OPEN_NODE_DIRECTORY | OS_OPEN_NODE_FAIL_IF_FOUND, &node);

			if (error != OS_SUCCESS) {
				instance->ReportError(ERROR_CANNOT_CREATE_FOLDER, error);
			} else {
				OSCloseHandle(node.handle);
				instance->LoadFolder(instance->path, instance->pathBytes, nullptr, 0, LOAD_FOLDER_NO_HISTORY);
			}

			OSHeapFree(fullPath);
		} break;
	}

	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse CommandNavigate(OSObject object, OSMessage *message) {
	(void) object;

	if (message->type != OS_NOTIFICATION_COMMAND) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	Instance *instance = (Instance *) OSGetInstance(message->command.window);

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
	
		case COMMAND_NAVIGATE_PATH: {
			OSString string;
			OSGetText(object, &string);

			if (!instance->LoadFolder(string.buffer, string.bytes)) {
				return OS_CALLBACK_REJECTED;
			}
		} break;
	}

	OSEnableCommand(instance->window, commandNavigateBackwards, instance->pathBackwardHistoryPosition);
	OSEnableCommand(instance->window, commandNavigateForwards, instance->pathForwardHistoryPosition);

	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse CallbackBookmarkFolder(OSObject object, OSMessage *message) {
	(void) object;

	if (message->type != OS_NOTIFICATION_COMMAND) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	Instance *instance = (Instance *) OSGetInstance(message->command.window);

	bool checked = message->command.checked;

	if (checked) {
		// Add bookmark.
		global.AddBookmark(instance->path, instance->pathBytes);
	} else {
		// Remove bookmark.
		if (!global.RemoveBookmark(instance->path, instance->pathBytes)) {
			instance->ReportError(ERROR_INTERNAL, OS_ERROR_UNKNOWN_OPERATION_FAILURE);
		}
	}

	return OS_CALLBACK_HANDLED;
}

OSCallbackResponse ProcessBookmarkListingNotification(OSObject object, OSMessage *message) {
	Instance *instance = (Instance *) message->context;
	(void) object;
	
	switch (message->type) {
		case OS_NOTIFICATION_GET_ITEM: {
			uintptr_t index = message->listViewItem.index;
			Bookmark *bookmark = global.bookmarks + index;

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_TEXT) {
				size_t length = 0;

				if (bookmark->pathBytes != 1) {
					while (bookmark->path[bookmark->pathBytes - ++length] != '/');
					length--;
				} else {
					length = 1;
				}

				message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
						"%s", length, bookmark->path + bookmark->pathBytes - length);
				message->listViewItem.text = guiStringBuffer;
			}

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_SELECTED) {
				if (!CompareStrings(bookmark->path, instance->path, bookmark->pathBytes, instance->pathBytes)) {
					message->listViewItem.state |= OS_LIST_VIEW_ITEM_SELECTED;
				}
			}

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_ICON) {
				message->listViewItem.iconID = OS_ICON_FOLDER;
			}

			return OS_CALLBACK_HANDLED;
		} break;

		case OS_NOTIFICATION_DESELECT_ALL: {
			return OS_CALLBACK_REJECTED;
		} break;

		case OS_NOTIFICATION_SET_ITEM: {
			uintptr_t index = message->listViewItem.index;
			Bookmark *bookmark = global.bookmarks + index;

			if (message->listViewItem.mask & OS_LIST_VIEW_ITEM_SELECTED) {
				if (message->listViewItem.state & OS_LIST_VIEW_ITEM_SELECTED) {
					instance->LoadFolder(bookmark->path, bookmark->pathBytes);
				}
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
				switch (message->listViewItem.column) {
					case COLUMN_NAME: {
						message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
								"%s", data->nameLengthBytes, data->name);
					} break;

					case COLUMN_DATE_MODIFIED: {
						message->listViewItem.textBytes = 0;
					} break;

					case COLUMN_TYPE: {
						message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
								"%z", data->information.type == OS_NODE_FILE ? "File" : "Directory");
					} break;

					case COLUMN_SIZE: {
						message->listViewItem.textBytes = 0;

						if (data->information.type == OS_NODE_FILE) {
							int fileSize = data->information.fileSize;

							if (fileSize == 0) {
								message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
										"(empty)");
							} else if (fileSize == 1) {
								message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
										"1 byte", fileSize);
							} else if (fileSize < 1000) {
								message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
										"%d bytes", fileSize);
							} else if (fileSize < 1000000) {
								message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
										"%d.%d KB", fileSize / 1000, (fileSize / 100) % 10);
							} else if (fileSize < 1000000000) {
								message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
										"%d.%d MB", fileSize / 1000000, (fileSize / 1000000) % 10);
							} else {
								message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, 
										"%d.%d GB", fileSize / 1000000000, (fileSize / 1000000000) % 10);
							}
						} else if (data->information.type == OS_NODE_DIRECTORY) {
							uint64_t children = data->information.directoryChildren;

							if (children == 0) message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "(empty)");
							else if (children == 1) message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "1 item");
							else message->listViewItem.textBytes = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "%d items", children);
						}
					} break;
				}

				message->listViewItem.text = guiStringBuffer;
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

bool Global::RemoveBookmark(char *path, size_t pathBytes) {
	for (uintptr_t i = 0; i < bookmarkCount; i++) {
		if (CompareStrings(bookmarks[i].path, path, bookmarks[i].pathBytes, pathBytes) == 0) {
			OSMoveMemory(bookmarks + i + 1, bookmarks + bookmarkCount, -1 * sizeof(Bookmark), false);
			bookmarkCount--;

			{
				LinkedItem<Instance> *instance = instances.firstItem;

				while (instance) {
					// TODO Removing items from a list view.
					OSListViewReset(instance->thisItem->bookmarkList);
					OSListViewInsert(instance->thisItem->bookmarkList, 0, bookmarkCount);

					instance = instance->nextItem;
				}
			}

			return true;
		}
	}

	return false;
}

void Global::AddBookmark(char *path, size_t pathBytes) {
	if (bookmarkAllocated == bookmarkCount) {
		bookmarkAllocated = (bookmarkAllocated + 8) * 2;
		Bookmark *replacement = (Bookmark *) OSHeapAllocate(bookmarkAllocated * sizeof(Bookmark), false);
		OSCopyMemory(replacement, bookmarks, bookmarkCount * sizeof(Bookmark));
		OSHeapFree(bookmarks);
		bookmarks = replacement;
	}

	Bookmark *bookmark = bookmarks + bookmarkCount;

	bookmark->pathBytes = pathBytes;
	bookmark->path = (char *) OSHeapAllocate(pathBytes, false);
	OSCopyMemory(bookmark->path, path, pathBytes);

	bookmarkCount++;

	{
		LinkedItem<Instance> *instance = instances.firstItem;

		while (instance) {
			OSListViewInsert(instance->thisItem->bookmarkList, bookmarkCount - 1, 1);
			instance = instance->nextItem;
		}
	}
}

void Instance::ReportError(unsigned where, OSError error) {
	const char *message = "An unknown error occurred.";
	const char *description = "Please try again.";

	switch (where) {
		case ERROR_CANNOT_LOAD_FOLDER: {
			message = "Could not open the folder.";
			description = "The specified path was invalid.";
		} break;

		case ERROR_CANNOT_CREATE_FOLDER: {
			message = "Could not create a new folder.";
		} break;

		case ERROR_INTERNAL: {
			message = "An internal error occurred.";
		} break;
	}

	switch (error) {
		case OS_ERROR_PATH_NOT_TRAVERSABLE: {
			description = "One or more of the leading folders did not exist.";
		} break;

		case OS_ERROR_FILE_DOES_NOT_EXIST: {
			description = "The folder does not exist.";
		} break;

		case OS_ERROR_FILE_ALREADY_EXISTS: {
			description = "The folder already exists.";
		} break;

		case OS_ERROR_FILE_PERMISSION_NOT_GRANTED: {
			description = "You do not have permission to view the contents of this folder.";
		} break;

		case OS_ERROR_INCORRECT_NODE_TYPE: {
			description = "This is not a valid folder.";
		} break;

		case OS_ERROR_DRIVE_CONTROLLER_REPORTED: {
			description = "An error occurred while accessing your drive.";
		} break;
	}

	OSShowDialogAlert(OSLiteral("Error"), OSLiteral(message), OSLiteral(description), 
			OS_ICON_ERROR, window);
}

bool Instance::LoadFolder(char *path1, size_t pathBytes1, char *path2, size_t pathBytes2, unsigned historyMode) {
	if (!pathBytes1) return false;

	char *oldPath = path;
	size_t oldPathBytes = pathBytes;
	char *newPath;

	{
		goto normal;
		fail:;
		OSHeapFree(newPath);
		return false;
		normal:;
	}

	// Fix the paths.
	if (path2 || pathBytes1 != 1) {
		if (path1[pathBytes1 - 1] == '/') pathBytes1--;
		if (path2 && path2[pathBytes2 - 1] == '/') pathBytes2--;
	}

	// Create the path.
	newPath = (char *) OSHeapAllocate(pathBytes1 + (path2 ? (pathBytes2 + 1) : 0), false);
	OSCopyMemory(newPath, path1, pathBytes1);
	size_t newPathBytes = pathBytes1;

	if (path2) {
		newPath[newPathBytes] = '/';
		OSCopyMemory(newPath + newPathBytes + 1, path2, pathBytes2);
		newPathBytes += pathBytes2 + 1;
	}

#if 0
	if (oldPath && !CompareStrings(oldPath, newPath, oldPathBytes, newPathBytes)) {
		goto fail;
	}
#endif

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
	OSSetText(folderPath, path, pathBytes, OS_RESIZE_MODE_IGNORE);
	OSEnableCommand(window, commandNavigateParent, pathBytes1 != 1);
	OSListViewInvalidate(bookmarkList, 0, global.bookmarkCount);

	{
		bool found = false;

		for (uintptr_t i = 0; i < global.bookmarkCount; i++) {
			if (CompareStrings(global.bookmarks[i].path, path, global.bookmarks[i].pathBytes, pathBytes) == 0) {
				found = true;
				break;
			}
		}

		OSCheckCommand(window, commandBookmarkFolder, found);
	}

	// Add the previous folder to the history.
	if (oldPath && historyMode != LOAD_FOLDER_NO_HISTORY) {
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

	// Update the status label.
	{
		size_t length;
		if (folderChildCount == 0) length = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "(empty)");
		else if (folderChildCount == 1) length = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "1 item");
		else length = OSFormatString(guiStringBuffer, GUI_STRING_BUFFER_LENGTH, "%d items", folderChildCount);
		OSSetText(statusLabel, guiStringBuffer, length, OS_RESIZE_MODE_GROW_ONLY);
	}

	return true;
}

OSCallbackResponse DestroyInstance(OSObject object, OSMessage *message) {
	(void) object;
	Instance *instance = (Instance *) message->context;
	OSHeapFree(instance->folderChildren);
	OSHeapFree(instance->path);
	global.instances.Remove(&instance->thisItem);
	return OS_CALLBACK_HANDLED;
}

void Instance::Initialise() {
	thisItem.thisItem = this;
	global.instances.InsertEnd(&thisItem);

	window = OSCreateWindow(mainWindow);
	OSSetInstance(window, this);

	OSSetCommandNotificationCallback(window, osCommandDestroyWindow, OS_MAKE_CALLBACK(DestroyInstance, this));

	OSObject rootLayout = OSCreateGrid(1, 6, OS_GRID_STYLE_LAYOUT);
	OSObject contentSplit = OSCreateGrid(3, 1, OS_GRID_STYLE_LAYOUT);
	OSObject toolbar1 = OSCreateGrid(5, 1, OS_GRID_STYLE_TOOLBAR);
	OSObject toolbar2 = OSCreateGrid(1, 1, OS_GRID_STYLE_TOOLBAR_ALT);
	OSObject statusBar = OSCreateGrid(2, 1, OS_GRID_STYLE_STATUS_BAR);

	OSSetRootGrid(window, rootLayout);
	OSAddGrid(rootLayout, 0, 3, contentSplit, OS_CELL_FILL);
	OSAddGrid(rootLayout, 0, 0, toolbar1, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);
	OSAddGrid(rootLayout, 0, 1, toolbar2, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);
	OSAddGrid(rootLayout, 0, 5, statusBar, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);

	bookmarkList = OSCreateListView(OS_CREATE_LIST_VIEW_SINGLE_SELECT);
	OSAddControl(contentSplit, 0, 0, bookmarkList, OS_CELL_H_EXPAND | OS_CELL_V_EXPAND);
	OSSetProperty(bookmarkList, OS_GUI_OBJECT_PROPERTY_SUGGESTED_WIDTH, (void *) 160);
	OSListViewInsert(bookmarkList, 0, global.bookmarkCount);
	OSSetObjectNotificationCallback(bookmarkList, OS_MAKE_CALLBACK(ProcessBookmarkListingNotification, this));

	folderListing = OSCreateListView(OS_CREATE_LIST_VIEW_MULTI_SELECT);
	OSAddControl(contentSplit, 2, 0, folderListing, OS_CELL_FILL);
	OSListViewSetColumns(folderListing, folderListingColumns, sizeof(folderListingColumns) / sizeof(folderListingColumns[0]));
	OSSetObjectNotificationCallback(folderListing, OS_MAKE_CALLBACK(ProcessFolderListingNotification, this));

	OSAddControl(contentSplit, 1, 0, OSCreateLine(OS_ORIENTATION_VERTICAL), OS_CELL_V_EXPAND | OS_CELL_V_PUSH);

	OSObject backButton = OSCreateButton(commandNavigateBackwards, OS_BUTTON_STYLE_TOOLBAR);
	OSAddControl(toolbar1, 0, 0, backButton, OS_CELL_V_CENTER | OS_CELL_V_PUSH);
	OSObject forwardButton = OSCreateButton(commandNavigateForwards, OS_BUTTON_STYLE_TOOLBAR_ICON_ONLY);
	OSAddControl(toolbar1, 1, 0, forwardButton, OS_CELL_V_CENTER | OS_CELL_V_PUSH);
	OSObject parentButton = OSCreateButton(commandNavigateParent, OS_BUTTON_STYLE_TOOLBAR_ICON_ONLY);
	OSAddControl(toolbar1, 2, 0, parentButton, OS_CELL_V_CENTER | OS_CELL_V_PUSH);

	folderPath = OSCreateTextbox(OS_TEXTBOX_STYLE_COMMAND);
	OSSetControlCommand(folderPath, commandNavigatePath);
	OSAddControl(toolbar1, 3, 0, folderPath, OS_CELL_H_EXPAND | OS_CELL_H_PUSH | OS_CELL_V_CENTER | OS_CELL_V_PUSH);

	OSObject bookmarkFolderButton = OSCreateButton(commandBookmarkFolder, OS_BUTTON_STYLE_TOOLBAR_ICON_ONLY);
	OSAddControl(toolbar1, 4, 0, bookmarkFolderButton, OS_CELL_V_CENTER | OS_CELL_V_PUSH);

	OSObject newFolderButton = OSCreateButton(commandNewFolder, OS_BUTTON_STYLE_TOOLBAR);
	OSAddControl(toolbar2, 0, 0, newFolderButton, OS_CELL_V_CENTER | OS_CELL_V_PUSH);

	statusLabel = OSCreateLabel(OSLiteral(""));
	OSAddControl(statusBar, 1, 0, statusLabel, OS_FLAGS_DEFAULT);

	LoadFolder(OSLiteral("/"));
	OSSetFocusedControl(folderListing, true);
}

void ProgramEntry() {
	global.AddBookmark(OSLiteral("/OS"));
	((Instance *) OSHeapAllocate(sizeof(Instance), true))->Initialise();
	((Instance *) OSHeapAllocate(sizeof(Instance), true))->Initialise();
	OSProcessMessages();
}
