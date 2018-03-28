#include "../api/os.h"

#define OS_MANIFEST_DEFINITIONS
#include "../bin/os/file_manager.manifest.h"

struct FolderChild {
	OSDirectoryChild data;
	bool selected;
};

struct Instance {
	OSObject folderListing,
		 folderPath;

	FolderChild *folderChildren;
	size_t folderChildCount;

	void LoadFolder(char *path, size_t pathBytes);
};

OSListViewColumn folderListingColumns[] = {
#define COLUMN_NAME (0)
	{ OSLiteral("Name"), 270, true, },
#define COLUMN_DATE_MODIFIED (1)
	{ OSLiteral("Date modified"), 120, false, },
#define COLUMN_TYPE (2)
	{ OSLiteral("Type"), 120, false, },
#define COLUMN_SIZE (3)
	{ OSLiteral("Size"), 100, false, },
};

int SortFolder(const void *_a, const void *_b, void *argument) {
	(void) argument;

	FolderChild *a = (FolderChild *) _a;
	FolderChild *b = (FolderChild *) _b;

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

		default: {
			return OS_CALLBACK_NOT_HANDLED;
		} break;
	}
}

void Instance::LoadFolder(char *path, size_t pathBytes) {
	OSNodeInformation node;
	OSError error;

	error = OSOpenNode(path, pathBytes, OS_OPEN_NODE_DIRECTORY | OS_OPEN_NODE_FAIL_IF_NOT_FOUND, &node);

	if (error != OS_SUCCESS) {
		// TODO Error reporting.
		OSCrashProcess(error);
	}

	size_t childCount = node.directoryChildren + 1024 /*Just in case extra files are created between OSOpenNode and here.*/;
	OSDirectoryChild *children = (OSDirectoryChild *) OSHeapAllocate(childCount * sizeof(OSDirectoryChild), true);
	error = OSEnumerateDirectoryChildren(node.handle, children, childCount);

	if (error != OS_SUCCESS) {
		// TODO Error reporting.
		OSCrashProcess(error);
	}

	for (uintptr_t i = 0; i < childCount; i++) {
		if (!children[i].information.present) {
			childCount = i;
			break;
		}
	}

	folderChildren = (FolderChild *) OSHeapAllocate(childCount * sizeof(FolderChild), true);
	folderChildCount = childCount;

	for (uintptr_t i = 0; i < childCount; i++) {
		OSCopyMemory(&folderChildren[i].data, children + i, sizeof(OSDirectoryChild));
	}

	OSSort(folderChildren, folderChildCount, sizeof(FolderChild), SortFolder, nullptr);

	OSListViewInsert(folderListing, 0, childCount);
	OSSetText(folderPath, path, pathBytes);

	OSHeapFree(children);
	OSCloseHandle(node.handle);
}

void ProgramEntry() {
	Instance *instance = (Instance *) OSHeapAllocate(sizeof(Instance), true);

	OSObject window = OSCreateWindow(mainWindow);
	OSSetInstance(window, instance);

	OSObject layout1 = OSCreateGrid(1, 3, OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_GAP);
	OSObject layout2 = OSCreateGrid(2, 1, OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_GAP);
	OSObject layout3 = OSCreateGrid(1, 1, OS_FLAGS_DEFAULT);
	OSSetRootGrid(window, layout1);
	OSAddGrid(layout1, 0, 2, layout2, OS_CELL_FILL);
	OSAddGrid(layout1, 0, 0, layout3, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);

	instance->folderListing = OSCreateListView(OS_CREATE_LIST_VIEW_MULTI_SELECT);
	OSAddControl(layout2, 1, 0, instance->folderListing, OS_CELL_FILL);
	OSListViewSetColumns(instance->folderListing, folderListingColumns, sizeof(folderListingColumns) / sizeof(folderListingColumns[0]));
	OSSetObjectNotificationCallback(instance->folderListing, OS_MAKE_CALLBACK(ProcessFolderListingNotification, instance));

	OSAddControl(layout1, 0, 1, OSCreateLine(OS_ORIENTATION_HORIZONTAL), OS_CELL_H_EXPAND | OS_CELL_H_PUSH);

	instance->folderPath = OSCreateTextbox(0);
	OSAddControl(layout3, 0, 0, instance->folderPath, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);

	instance->LoadFolder(OSLiteral("/os/"));

	OSProcessMessages();
}
