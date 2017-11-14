// TODO
//
// 	-> Creating files
// 	-> Get file information
// 	-> Directory enumeration
//
// 	-> Creating directories
// 	-> Moving files/directories
// 	-> Deleting files/directories
// 	-> Asynchronous file I/O (start, cancel, wait, get progress)
// 	-> Wait for file access
// 	-> File/block cache
// 	-> Keep files around even when all closed to cache
// 	-> Memory mapped files

#ifndef IMPLEMENTATION

#define DIRECTORY_ACCESS (OS_OPEN_NODE_DIRECTORY)

enum FilesystemType {
#if 0
	FILESYSTEM_EXT2,
#endif
	FILESYSTEM_ESFS,
};

struct Directory {
	uint64_t entryCount;
};

struct File {
	uint64_t fileSize;
};

struct NodeData {
	OSNodeType type;

	union {
		File file;
		Directory directory;
	};
};

struct Node {
	// Files:
	size_t Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer);
	size_t Write(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer);
	bool Resize(uint64_t newSize);

	// Directories:
	Node *OpenChild();
	size_t EnumerateChildren(size_t count, struct NodeData *output);

	void CopyInformation(OSNodeInformation *information);
	void Sync();
	bool modifiedSinceLastSync;

	// "Read" on a directory -> enumerate/search files
	// "Write" on a directory -> add/remove files 
	size_t countRead, countWrite, countResize;
	bool exclusiveRead, exclusiveWrite, exclusiveResize;

	UniqueIdentifier identifier;
	struct Filesystem *filesystem;
	volatile size_t handles;

	struct Node *nextNodeInHashTableSlot;
	struct Node **pointerToThisNodeInHashTableSlot;

	Mutex mutex; // Lock the node during an operation.

	NodeData data;
	Node *parent;
};

struct Filesystem {
	FilesystemType type;
	Node *root;
	LinkedList mountpoints;
	LinkedItem allFilesystemsItem;
	void *data;
};

struct Mountpoint {
	char path[MAX_PATH];
	size_t pathLength;

	Node *root;
	Filesystem *filesystem;

	LinkedItem filesystemMountpointsItem;
	LinkedItem allMountpointsItem;
};

struct VFS {
	void Initialise();
	Filesystem *RegisterFilesystem(Node *root, FilesystemType type, void *data, UniqueIdentifier installationID);

	Node *OpenNode(char *name, size_t nameLength, uint64_t flags);
	void CloseNode(Node *node, uint64_t flags);

	Node *RegisterNodeHandle(void *existingNode, uint64_t &flags /*Removes failing access flags*/, UniqueIdentifier identifier, Node *parent, OSNodeType type);
	Node *FindOpenNode(UniqueIdentifier identifier, Filesystem *filesystem);

	LinkedList filesystems, mountpoints;
	Mutex filesystemsMutex, mountpointsMutex;

	bool foundBootFilesystem;

#define NODE_HASH_TABLE_BITS (12)
	Node *nodeHashTable[1 << NODE_HASH_TABLE_BITS];
	Mutex nodeHashTableMutex;
};

VFS vfs;

#endif

#ifdef IMPLEMENTATION

#if 0
bool Ext2Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, Node *file);
Node *Ext2Scan(char *name, size_t nameLength, Node *directory, uint64_t &flags);
#endif

bool EsFSRead(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, Node *file);
bool EsFSWrite(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, Node *file);
void EsFSSync(Node *node);
Node *EsFSScan(char *name, size_t nameLength, Node *directory, uint64_t &flags);
bool EsFSResize(Node *file, uint64_t newSize);

bool Node::Resize(uint64_t newSize) {
	mutex.Acquire();
	Defer(mutex.Release());

	parent->mutex.Acquire();
	Defer(parent->mutex.Release());

	modifiedSinceLastSync = true;

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			return EsFSResize(this, newSize);
		} break;

		default: {
			return false;
		} break;
	}
}

void Node::Sync() {
	mutex.Acquire();
	Defer(mutex.Release());

	if (!modifiedSinceLastSync) {
		return;
	}

	parent->mutex.Acquire();
	Defer(parent->mutex.Release());

	modifiedSinceLastSync = false;

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			EsFSSync(this);
		} break;

		default: {
			// The filesystem does not need to the do anything.
		} break;
	}
}

void Node::CopyInformation(OSNodeInformation *information) {
	CopyMemory(&information->identifier, &identifier, sizeof(UniqueIdentifier));
	information->type = data.type;

	switch (data.type) {
		case OS_NODE_FILE: {
			information->fileSize = data.file.fileSize;
		} break;

		case OS_NODE_DIRECTORY: {
			information->directoryChildren = data.directory.entryCount;
		} break;
	}
}

size_t Node::Write(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (offsetBytes > data.file.fileSize) {
		return 0;
	}

	if (offsetBytes + sizeBytes > data.file.fileSize) {
		sizeBytes = data.file.fileSize - offsetBytes;
	}

	if (sizeBytes > data.file.fileSize) {
		return 0;
	}

	modifiedSinceLastSync = true;

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			bool fail = EsFSWrite(offsetBytes, sizeBytes, buffer, this);
			if (!fail) return 0;
		} break;

		default: {
			// The filesystem driver is read-only.
			return 0;
		} break;
	}

	return sizeBytes;
}

size_t Node::Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (offsetBytes > data.file.fileSize) {
		return 0;
	}

	if (offsetBytes + sizeBytes > data.file.fileSize) {
		sizeBytes = data.file.fileSize - offsetBytes;
	}

	if (sizeBytes > data.file.fileSize) {
		return 0;
	}

	switch (filesystem->type) {
#if 0
		case FILESYSTEM_EXT2: {
			bool fail = Ext2Read(offsetBytes, sizeBytes, buffer, this);
			if (!fail) return 0;
		} break;
#endif

		case FILESYSTEM_ESFS: {
			bool fail = EsFSRead(offsetBytes, sizeBytes, buffer, this);
			if (!fail) return 0;
		} break;

		default: {
			KernelPanic("Node::Read - Unsupported filesystem.\n");
		} break;
	}

	return sizeBytes;
}

void VFS::Initialise() {
	bool bootedFromEsFS = false;
	for (uintptr_t i = 0; i < 16; i++) {
		if (installationID.d[i]) {
			bootedFromEsFS = true;
			break;
		}
	}

	if (!bootedFromEsFS) {
		KernelPanic("VFS::Initialise - The operating system was not booted from an EssenceFS volume.\n");
	}
}

void VFS::CloseNode(Node *node, uint64_t flags) {
	nodeHashTableMutex.Acquire();

	node->handles--;

	bool noHandles = node->handles == 0;

	if (flags & OS_OPEN_NODE_ACCESS_READ)      node->countRead--;
	if (flags & OS_OPEN_NODE_ACCESS_WRITE)     node->countWrite--;
	if (flags & OS_OPEN_NODE_ACCESS_RESIZE)    node->countResize--;

	if (flags & OS_OPEN_NODE_EXCLUSIVE_READ)   node->exclusiveRead = false;
	if (flags & OS_OPEN_NODE_EXCLUSIVE_WRITE)  node->exclusiveWrite = false;
	if (flags & OS_OPEN_NODE_EXCLUSIVE_RESIZE) node->exclusiveResize = false;

	// TODO Notify anyone waiting for the file to be accessible.

	Node *parent = node->parent;

	if (noHandles) {
		node->Sync();

		if (node->parent) {
			*node->pointerToThisNodeInHashTableSlot = node->nextNodeInHashTableSlot;
		}

		OSHeapFree(node);
	}

	nodeHashTableMutex.Release();

	if (parent) {
		CloseNode(parent, DIRECTORY_ACCESS);
	}
}

Node *VFS::OpenNode(char *name, size_t nameLength, uint64_t flags) {
	mountpointsMutex.Acquire();

	LinkedItem *_mountpoint = mountpoints.firstItem;
	Mountpoint *longestMatch = nullptr;

	while (_mountpoint) {
		Mountpoint *mountpoint = (Mountpoint *) _mountpoint->thisItem;
		size_t pathLength = mountpoint->pathLength;

		if (nameLength < pathLength) goto next;
		if (longestMatch && pathLength < longestMatch->pathLength) goto next;
		if (CompareBytes(name, mountpoint->path, pathLength)) goto next;
		longestMatch = mountpoint;

		next:
		_mountpoint = _mountpoint->nextItem;
	}

	mountpointsMutex.Release();

	if (!longestMatch) {
		// The requested file was not on any mounted filesystem.
		return nullptr;
	}

	Mountpoint *mountpoint = longestMatch;
	name += mountpoint->pathLength;
	nameLength -= mountpoint->pathLength;

	Filesystem *filesystem = mountpoint->filesystem;
	uint64_t directoryAccess = DIRECTORY_ACCESS;
	Node *node = RegisterNodeHandle(mountpoint->root, directoryAccess, mountpoint->root->identifier, nullptr, OS_NODE_DIRECTORY);
	node->filesystem = filesystem;

	uint64_t desiredFlags = flags;

	while (nameLength) {
		char *entry = name;
		size_t entryLength = 0;

		while (nameLength) {
			nameLength--;
			name++;

			if (entry[entryLength] == '/') {
				break;
			}

			entryLength++;
		}

		bool isFinalNode = !nameLength;
		Node *parent = node;

		if (node->filesystem != filesystem) {
			KernelPanic("VFS::OpenFile - Incorrect node filesystem.\n");
		}

		switch (filesystem->type) {
#if 0
			case FILESYSTEM_EXT2: {
				node = Ext2Scan(entry, entryLength, node, isFinalNode ? flags : directoryAccess);
			} break;
#endif

			case FILESYSTEM_ESFS: {
				node = EsFSScan(entry, entryLength, node, isFinalNode ? flags : directoryAccess);
			} break;

			default: {
				KernelPanic("VFS::OpenNode - Unimplemented filesystem type %d\n", filesystem->type);
			} break;
		}

		if (!node) {
			// Close the handles we opened to the parent directories while traversing to this node.
			CloseHandleToObject(parent, KERNEL_OBJECT_NODE, directoryAccess);

			if (!directoryAccess) {
				// A "directory" in the path wasn't a directory.
				return nullptr;
			}

			if (desiredFlags != flags) {
				// We couldn't only the file with the desired access flags.
				return nullptr;
			}

			// TODO If the flag OS_OPEN_NODE_FAIL_IF_NOT_FOUND is clear, 
			// 	then create the file.

			return nullptr;
		} 
	}

	if (node && (flags & OS_OPEN_NODE_FAIL_IF_FOUND)) {
		CloseNode(node, flags);
	}

	return node;
}

Node *VFS::RegisterNodeHandle(void *_existingNode, uint64_t &flags, UniqueIdentifier identifier, Node *parent, OSNodeType type) {
	if (parent && !parent->filesystem) {
		KernelPanic("VFS::RegisterNodeHandle - Trying to register a node without a filesystem.\n");
	}

	Node *existingNode = (Node *) _existingNode;

	nodeHashTableMutex.Acquire();
	Defer(nodeHashTableMutex.Release());

	if ((flags & OS_OPEN_NODE_ACCESS_READ) && existingNode->exclusiveRead) {
		flags &= ~(OS_OPEN_NODE_ACCESS_READ);
		return nullptr;
	}

	if ((flags & OS_OPEN_NODE_ACCESS_WRITE) && existingNode->exclusiveWrite) {
		flags &= ~(OS_OPEN_NODE_ACCESS_WRITE);
		return nullptr;
	}

	if ((flags & OS_OPEN_NODE_ACCESS_RESIZE) && existingNode->exclusiveResize) {
		flags &= ~(OS_OPEN_NODE_ACCESS_RESIZE);
		return nullptr;
	}

	if ((flags & OS_OPEN_NODE_EXCLUSIVE_READ) && existingNode->countRead) {
		flags &= ~(OS_OPEN_NODE_EXCLUSIVE_READ);
		return nullptr;
	}

	if ((flags & OS_OPEN_NODE_EXCLUSIVE_WRITE) && existingNode->countWrite) {
		flags &= ~(OS_OPEN_NODE_EXCLUSIVE_WRITE);
		return nullptr;
	}

	if ((flags & OS_OPEN_NODE_EXCLUSIVE_RESIZE) && existingNode->countResize) {
		flags &= ~(OS_OPEN_NODE_EXCLUSIVE_RESIZE);
		return nullptr;
	}

	if ((flags & OS_OPEN_NODE_DIRECTORY) && type != OS_NODE_DIRECTORY) {
		flags &= ~(OS_OPEN_NODE_DIRECTORY);
		return nullptr;
	}

	if ((flags & !OS_OPEN_NODE_DIRECTORY) && type == OS_NODE_DIRECTORY) {
		flags |= (OS_OPEN_NODE_DIRECTORY);
		return nullptr;
	}

	if (flags & OS_OPEN_NODE_ACCESS_READ)      existingNode->countRead++;
	if (flags & OS_OPEN_NODE_ACCESS_WRITE)     existingNode->countWrite++;
	if (flags & OS_OPEN_NODE_ACCESS_RESIZE)    existingNode->countResize++;

	if (flags & OS_OPEN_NODE_EXCLUSIVE_READ)   existingNode->exclusiveRead = true;
	if (flags & OS_OPEN_NODE_EXCLUSIVE_WRITE)  existingNode->exclusiveWrite = true;
	if (flags & OS_OPEN_NODE_EXCLUSIVE_RESIZE) existingNode->exclusiveResize = true;

	existingNode->handles++;

	if (1 == existingNode->handles && parent) {
		existingNode->filesystem = parent->filesystem;
		existingNode->parent = parent; // The handle to the parent will be closed when the file is closed.

		uint16_t slot = ((uint16_t) identifier.d[0] + ((uint16_t) identifier.d[1] << 8)) & 0xFFF;
		existingNode->nextNodeInHashTableSlot = nodeHashTable[slot];
		if (nodeHashTable[slot]) nodeHashTable[slot]->pointerToThisNodeInHashTableSlot = &existingNode->nextNodeInHashTableSlot;
		nodeHashTable[slot] = existingNode;
		existingNode->pointerToThisNodeInHashTableSlot = nodeHashTable + slot;
	}

	return existingNode;
}

Node *VFS::FindOpenNode(UniqueIdentifier identifier, Filesystem *filesystem) {
	nodeHashTableMutex.Acquire();
	Defer(nodeHashTableMutex.Release());

	uint16_t slot = ((uint16_t) identifier.d[0] + ((uint16_t) identifier.d[1] << 8)) & 0xFFF;

	Node *node = nodeHashTable[slot];

	while (node) {
		if (node->filesystem == filesystem 
				&& !CompareBytes(&node->identifier, &identifier, sizeof(UniqueIdentifier))
				&& node->handles /* Make sure the node isn't about to be deallocated! */) {
			return node;
		}

		node = node->nextNodeInHashTableSlot;
	}

	return nullptr; // The node is not currently open.
}

Filesystem *VFS::RegisterFilesystem(Node *root, FilesystemType type, void *data, UniqueIdentifier fsInstallationID) {
	filesystemsMutex.Acquire();
	mountpointsMutex.Acquire();

	if (root->parent) {
		KernelPanic("VFS::RegisterFilesystem - \"Root\" node had a parent.\n");
	}

	uintptr_t filesystemID = filesystems.count;

	Filesystem *filesystem = (Filesystem *) OSHeapAllocate(sizeof(Filesystem), true);
	filesystem->allFilesystemsItem.thisItem = filesystem;
	filesystem->type = type;
	filesystem->root = root;
	filesystem->data = data;
	filesystems.InsertEnd(&filesystem->allFilesystemsItem);

	Mountpoint *mountpoint = (Mountpoint *) OSHeapAllocate(sizeof(Mountpoint), true);
	mountpoint->root = root;
	mountpoint->filesystem = filesystem;
	mountpoint->pathLength = FormatString(mountpoint->path, MAX_PATH, OS_FOLDER "/Volume%d/", filesystemID);
	mountpoint->allMountpointsItem.thisItem = mountpoint;
	mountpoint->filesystemMountpointsItem.thisItem = mountpoint;
	mountpoints.InsertEnd(&mountpoint->allMountpointsItem);
	filesystem->mountpoints.InsertEnd(&mountpoint->filesystemMountpointsItem);

	filesystemsMutex.Release();
	mountpointsMutex.Release();

	if (!foundBootFilesystem) {
		// We currently only support booting on EssenceFS volumes.
		if (type != FILESYSTEM_ESFS) {
			goto end;
		}

		// This wasn't the boot volume.
		if (CompareBytes(&fsInstallationID, &installationID, sizeof(UniqueIdentifier))) {
			goto end;
		}

		foundBootFilesystem = true;

		filesystemsMutex.Acquire();
		mountpointsMutex.Acquire();

		// Mount the volume at root.
		Mountpoint *mountpoint = (Mountpoint *) OSHeapAllocate(sizeof(Mountpoint), true);
		mountpoint->root = root;
		mountpoint->filesystem = filesystem;
		mountpoint->pathLength = FormatString(mountpoint->path, MAX_PATH, "/");
		mountpoint->allMountpointsItem.thisItem = mountpoint;
		mountpoint->filesystemMountpointsItem.thisItem = mountpoint;
		mountpoints.InsertEnd(&mountpoint->allMountpointsItem);
		filesystem->mountpoints.InsertEnd(&mountpoint->filesystemMountpointsItem);

		filesystemsMutex.Release();
		mountpointsMutex.Release();
	}

	end:;
	return filesystem;
}

#endif
