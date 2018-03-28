#include "../api/os.h"

#define OS_MANIFEST_DEFINITIONS
#include "../bin/os/file_manager.manifest.h"

struct FolderChild {
	OSDirectoryChild data;
};

struct Instance {
	OSObject folderListing;

	FolderChild *folder;

	void LoadFolder(char *path, size_t pathBytes);
};

OSListViewColumn folderListingColumns[] = {
	{ OSLiteral("Name"), 270, true, },
	{ OSLiteral("Date modified"), 120, false, },
	{ OSLiteral("Type"), 120, false, },
	{ OSLiteral("Size"), 100, false, },
};

OSCallbackResponse ProcessFolderListingNotification(OSObject object, OSMessage *message) {
	Instance *instance = (Instance *) message->context;

	(void) object;
	
	switch (message->type) {
		case OS_NOTIFICATION_GET_ITEM: {
			message->listViewItem.state = 0;
			message->listViewItem.text = (char *) instance->folder[message->listViewItem.index].data.name;
			message->listViewItem.textBytes = instance->folder[message->listViewItem.index].data.nameLengthBytes;

			return OS_CALLBACK_HANDLED;
		} break;

		case OS_NOTIFICATION_DESELECT_ALL: {
			// TODO 
			return OS_CALLBACK_HANDLED;
		} break;

		case OS_NOTIFICATION_SET_ITEM: {
			// TODO 
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

	folder = (FolderChild *) OSHeapAllocate(childCount * sizeof(FolderChild), true);

	for (uintptr_t i = 0; i < childCount; i++) {
		OSCopyMemory(&folder[i].data, children + i, sizeof(OSDirectoryChild));
	}

	OSListViewInsert(folderListing, 0, childCount);

	OSHeapFree(children);
	OSCloseHandle(node.handle);
}

void ProgramEntry() {
	Instance *instance = (Instance *) OSHeapAllocate(sizeof(Instance), true);

	OSObject window = OSCreateWindow(mainWindow);
	OSSetInstance(window, instance);

	OSObject layout1 = OSCreateGrid(1, 2, OS_FLAGS_DEFAULT);
	OSObject layout2 = OSCreateGrid(2, 1, OS_CREATE_GRID_NO_BORDER);
	OSSetRootGrid(window, layout1);
	OSAddGrid(layout1, 0, 1, layout2, OS_CELL_FILL);

	instance->folderListing = OSCreateListView(OS_CREATE_LIST_VIEW_BORDER | OS_CREATE_LIST_VIEW_MULTI_SELECT);
	OSAddControl(layout2, 1, 0, instance->folderListing, OS_CELL_FILL);
	OSListViewSetColumns(instance->folderListing, folderListingColumns, sizeof(folderListingColumns) / sizeof(folderListingColumns[0]));
	OSSetObjectNotificationCallback(instance->folderListing, OS_MAKE_CALLBACK(ProcessFolderListingNotification, instance));

	instance->LoadFolder(OSLiteral("/"));

	OSProcessMessages();
}
