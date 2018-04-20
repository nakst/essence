// TODO
// 	-> Moving files/directories
// 	-> Wait for file access
// 	-> Prevent opening/accessing deleted nodes

#ifndef IMPLEMENTATION

#define DIRECTORY_ACCESS (OS_OPEN_NODE_DIRECTORY)

enum FilesystemType {
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
	void Read(struct IOPacket *packet);
	void Write(struct IOPacket *packet, bool canResize);
	bool Resize(uint64_t newSize);
	void Complete(struct IOPacket *packet);

	// Directories:
	bool EnumerateChildren(OSDirectoryChild *buffer, size_t bufferCount);

	// General:
	OSError Delete();
	OSError Move(Node *newDirectory, char *newName, size_t newNameLength);
	void CopyInformation(OSNodeInformation *information);
	void Sync();

	size_t countRead, countWrite, countResize;
	size_t blockRead, blockWrite, blockResize;

	bool deleted;

	UniqueIdentifier identifier;
	struct Filesystem *filesystem;
	volatile size_t handles; // Every node handle also implies a handle to its parent.

	struct Node *nextNodeInHashTableSlot;
	struct Node **pointerToThisNodeInHashTableSlot;

	Semaphore semaphore; 

	NodeData data;
	Node *parent;

	// We keep a couple nodes around that have no handles, so they can be quickly accessed later.
	LinkedItem<Node> noHandleCacheItem; 

	SharedMemoryRegion region;
};

struct Filesystem {
	FilesystemType type;
	Node *root;
	LinkedList<struct Mountpoint> mountpoints;
	LinkedItem<Filesystem> allFilesystemsItem;
	void *data;
};

struct Mountpoint {
	char path[MAX_PATH];
	size_t pathLength;

	Node *root;
	Filesystem *filesystem;

	LinkedItem<Mountpoint> filesystemMountpointsItem;
	LinkedItem<Mountpoint> allMountpointsItem;
};

struct VFS {
	void Initialise();
	Filesystem *RegisterFilesystem(Node *root, FilesystemType type, void *data, UniqueIdentifier installationID);

	Node *OpenNode(char *name, size_t nameLength, uint64_t flags, OSError *error);
	void CloseNode(Node *node, uint64_t flags);

	Node *RegisterNodeHandle(void *existingNode, uint64_t &flags /*Removes failing access flags*/, UniqueIdentifier identifier, Node *parent, OSNodeType type, bool isNodeNew);
	Node *FindOpenNode(UniqueIdentifier identifier, Filesystem *filesystem);

	void NodeMapped(Node *node);
	void NodeUnmapped(Node *node);

	LinkedList<Filesystem> filesystems;
	LinkedList<Mountpoint> mountpoints;
	Mutex filesystemsMutex, mountpointsMutex;

#define MAX_CACHED_NODES (256)
	LinkedList<Node> cachedNodes;

	bool foundBootFilesystem;

#define NODE_HASH_TABLE_BITS (12)
	Node *nodeHashTable[1 << NODE_HASH_TABLE_BITS];
	Mutex nodeHashTableMutex; // Required to changed node handle count.
};

VFS vfs;

#endif

#ifdef IMPLEMENTATION

bool Node::Resize(uint64_t newSize) {
	parent->semaphore.Take();
	Defer(parent->semaphore.Return());

	semaphore.Take();
	Defer(semaphore.Return());

	if (deleted) return false;
	bool success = false;

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			success = EsFSResize(this, newSize);
		} break;
	}

	if (success) {
		data.file.fileSize = newSize;

		sharedMemoryManager.mutex.Acquire();
		sharedMemoryManager.ResizeSharedMemory(&region, newSize);
		sharedMemoryManager.mutex.Release();
	}

	return success;
}

OSError Node::Delete() {
	parent->semaphore.Take();
	Defer(parent->semaphore.Return());

	semaphore.Take();
	Defer(semaphore.Return());

	if (deleted) return OS_ERROR_NODE_ALREADY_DELETED;

	if (data.type == OS_NODE_DIRECTORY && data.directory.entryCount) {
		return OS_ERROR_DIRECTORY_NOT_EMPTY;
	}

	OSError result = OS_ERROR_UNKNOWN_OPERATION_FAILURE;

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			if (EsFSRemove(this)) result = OS_SUCCESS;
		} break;

		default: {
			// The filesystem does not support node removal.
			result = OS_ERROR_UNSUPPORTED_FILESYSTEM;
		} break;
	}

	if (result == OS_SUCCESS) {
		deleted = true;
	}

	return result;
}

OSError Node::Move(Node *newDirectory, char *newName, size_t newNameLength) {
	if (!newDirectory) {
		if (!parent) {
			return OS_ERROR_NODE_IS_ROOT;
		}

		newDirectory = parent;
	}

	if (newDirectory->data.type != OS_NODE_DIRECTORY) {
		return OS_ERROR_TARGET_INVALID_TYPE;
	}

	bool takeSemaphoreOnParentFirst = false;

	{
		Node *n = newDirectory;

		while (n) {
			n = n->parent;

			if (n == parent) {
				// We are trying to move this node into a child folder of the current parent.
				takeSemaphoreOnParentFirst = true;
			} else if (n == this) {
				// We are trying to move this node into a folder within itself.
				return OS_ERROR_TARGET_WITHIN_SOURCE;
			}
		}
	}

	// Eww....

	if (!takeSemaphoreOnParentFirst && parent != newDirectory) newDirectory->semaphore.Take();
	Defer(if (!takeSemaphoreOnParentFirst && parent != newDirectory) newDirectory->semaphore.Return());

	Node *oldDirectory = parent;

	oldDirectory->semaphore.Take();
	Defer(oldDirectory->semaphore.Return());

	if (takeSemaphoreOnParentFirst && parent != newDirectory) newDirectory->semaphore.Take();
	Defer(if (takeSemaphoreOnParentFirst && parent != newDirectory) newDirectory->semaphore.Return());

	semaphore.Take();
	Defer(semaphore.Return());

	if (deleted) return OS_ERROR_NODE_ALREADY_DELETED;
	if (newDirectory->filesystem != parent->filesystem) return OS_ERROR_VOLUME_MISMATCH;

	OSError result = OS_ERROR_UNKNOWN_OPERATION_FAILURE;

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			if (EsFSMove(this, newDirectory, newName, newNameLength)) result = OS_SUCCESS;
		} break;

		default: {
			// The filesystem does not support node removal.
			result = OS_ERROR_UNSUPPORTED_FILESYSTEM;
		} break;
	}

	if (result == OS_SUCCESS) {
		parent = newDirectory;
	}

	return result;
}

void Node::Sync() {
	parent->semaphore.Take();
	Defer(parent->semaphore.Return());

	semaphore.Take();
	Defer(semaphore.Return());

	if (deleted) return;
	
	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			EsFSSync(this);
		} break;

		default: {
			// The filesystem does not need to the do anything.
		} break;
	}
}

bool Node::EnumerateChildren(OSDirectoryChild *buffer, size_t bufferCount) {
	semaphore.Take();
	Defer(semaphore.Return());

	if (deleted) return false;

	if (bufferCount < data.directory.entryCount) {
		return false;
	}

	if (bufferCount > data.directory.entryCount) {
		buffer[data.directory.entryCount].information.present = false;
	}

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			EsFSEnumerate(this, buffer);
			return true;
		} break;
	}

	return false;
}

void Node::CopyInformation(OSNodeInformation *information) {
	information->type = data.type;

	switch (data.type) {
		case OS_NODE_FILE: {
			information->fileSize = data.file.fileSize;
		} break;

		case OS_NODE_DIRECTORY: {
			information->directoryChildren = data.directory.entryCount;
		} break;

		case OS_NODE_INVALID: {
		} break;
	}
}

void Node::Complete(IOPacket *packet) {
	packet->request->node->semaphore.Return();
}

void Node::Write(IOPacket *packet, bool canResize) {
	// TODO Access cache.

	IORequest *request = packet->request;

	if (request->offset + request->count > data.file.fileSize && canResize) {
		if (!Resize(request->offset + request->count)) {
			request->Cancel(OS_ERROR_COULD_NOT_RESIZE_FILE);
			return;
		}
	}

	semaphore.Take();

	if (deleted) {
		request->Cancel(OS_ERROR_NODE_ALREADY_DELETED);
		return;
	}

	if (request->offset > data.file.fileSize) {
		request->Cancel(OS_ERROR_ACCESS_NOT_WITHIN_FILE_BOUNDS);
		return;
	}

	if (request->offset + request->count > data.file.fileSize) {
		request->count = data.file.fileSize - request->offset;
	}

	if (request->count > data.file.fileSize) {
		request->Cancel(OS_ERROR_ACCESS_NOT_WITHIN_FILE_BOUNDS);
		return;
	}

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			IOPacket *fsPacket = packet->request->AddPacket(packet);
			fsPacket->type = IO_PACKET_ESFS;
			fsPacket->object = request->node;
			fsPacket->count = request->count;
			fsPacket->offset = request->offset;
			fsPacket->buffer = request->buffer;
			EsFSWrite(fsPacket);
			fsPacket->QueuedChildren();
		} break;

		default: {
			// The filesystem driver is read-only.
			request->Cancel(OS_ERROR_FILE_ON_READ_ONLY_VOLUME);
			return;
		} break;
	}
}

void Node::Read(IOPacket *packet) {
	// TODO Access cache.

	semaphore.Take();

	IORequest *request = packet->request;

	if (deleted) {
		request->Cancel(OS_ERROR_NODE_ALREADY_DELETED);
		return;
	}

	if (request->offset > data.file.fileSize) {
		request->Cancel(OS_ERROR_ACCESS_NOT_WITHIN_FILE_BOUNDS);
		return;
	}

	if (request->offset + request->count > data.file.fileSize) {
		request->count = data.file.fileSize - request->offset;
	}

	if (request->count > data.file.fileSize) {
		request->Cancel(OS_ERROR_ACCESS_NOT_WITHIN_FILE_BOUNDS);
		return;
	}

	switch (filesystem->type) {
		case FILESYSTEM_ESFS: {
			IOPacket *fsPacket = packet->request->AddPacket(packet);
			fsPacket->type = IO_PACKET_ESFS;
			fsPacket->object = request->node;
			fsPacket->count = request->count;
			fsPacket->offset = request->offset;
			fsPacket->buffer = request->buffer;
			EsFSRead(fsPacket);
			fsPacket->QueuedChildren();
		} break;

		default: {
			KernelPanic("Node::Read - Unsupported filesystem.\n");
		} break;
	}
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

void VFS::NodeUnmapped(Node *node) {
	CloseNode(node, 0);
}

void VFS::CloseNode(Node *node, uint64_t flags) {
	nodeHashTableMutex.Acquire();

	node->handles--;

	if ((flags & OS_OPEN_NODE_READ_BLOCK)   ) { node->blockRead--; }
	if ((flags & OS_OPEN_NODE_READ_ACCESS)  ) { node->countRead--; }
	if ((flags & OS_OPEN_NODE_WRITE_BLOCK)  ) { node->blockWrite--; }
	if ((flags & OS_OPEN_NODE_WRITE_ACCESS) ) { node->countWrite--; }
	if ((flags & OS_OPEN_NODE_RESIZE_BLOCK) ) { node->blockResize--; }
	if ((flags & OS_OPEN_NODE_RESIZE_ACCESS)) { node->countResize--; }

	// TODO Notify anyone waiting for the file to be accessible.

	Node *volatile parent = node->parent;
	Node *node3 = nullptr;

	if (node->handles == 0) {
		Node *node2 = node;
		bool cacheNode = !node2->deleted;

		if (cacheNode) {
			vfs.cachedNodes.InsertEnd(&node2->noHandleCacheItem);
		}

		if (!cacheNode || vfs.cachedNodes.count > MAX_CACHED_NODES) {
			node3 = node2;

			if (cacheNode) {
				LinkedItem<Node> *item = vfs.cachedNodes.firstItem;
				vfs.cachedNodes.Remove(item);
				node3 = item->thisItem;
			}

			// Remove the node from the hash table.
			{
				if (node3->nextNodeInHashTableSlot) {
					node3->nextNodeInHashTableSlot->pointerToThisNodeInHashTableSlot = node3->pointerToThisNodeInHashTableSlot;
				}

				*node3->pointerToThisNodeInHashTableSlot = node3->nextNodeInHashTableSlot;
			}
		}
	}

	nodeHashTableMutex.Release();

	if (node3) {
		node3->Sync();
		sharedMemoryManager.DestroySharedMemory(&node3->region);
		OSHeapFree(node3);
	}

	if (parent) {
		CloseNode(parent, DIRECTORY_ACCESS);
	}
}

Node *VFS::OpenNode(char *name, size_t nameLength, uint64_t flags, OSError *error) {
	mountpointsMutex.Acquire();

	LinkedItem<Mountpoint> *_mountpoint = mountpoints.firstItem;
	Mountpoint *longestMatch = nullptr;

	while (_mountpoint) {
		Mountpoint *mountpoint = _mountpoint->thisItem;
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
		*error = OS_ERROR_PATH_NOT_WITHIN_MOUNTED_VOLUME;
		return nullptr;
	}

	Mountpoint *mountpoint = longestMatch;
	name += mountpoint->pathLength;
	nameLength -= mountpoint->pathLength;

	Filesystem *filesystem = mountpoint->filesystem;
	uint64_t directoryAccess = DIRECTORY_ACCESS;
	Node *node = RegisterNodeHandle(mountpoint->root, directoryAccess, mountpoint->root->identifier, nullptr, OS_NODE_DIRECTORY, false);

	if (!node) {
		*error = OS_ERROR_PATH_NOT_TRAVERSABLE;
		return nullptr;
	}

	node->filesystem = filesystem;

	uint64_t desiredFlags = flags;
	bool secondAttempt = false;

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

		if (entryLength > OS_MAX_DIRECTORY_CHILD_NAME_LENGTH) {
			*error = OS_ERROR_PATH_NOT_TRAVERSABLE;
			return nullptr;
		}

		bool isFinalNode = !nameLength;
		Node *parent = node;
		parent->semaphore.Take();
		Defer(parent->semaphore.Return());

		tryAgain:;

		if (node->filesystem != filesystem) {
			KernelPanic("VFS::OpenFile - Incorrect node filesystem.\n");
		}

		switch (filesystem->type) {
			case FILESYSTEM_ESFS: {
				node = EsFSScan(entry, entryLength, parent, isFinalNode ? flags : directoryAccess);
			} break;

			default: {
				KernelPanic("VFS::OpenNode - Unimplemented filesystem type %d\n", filesystem->type);
			} break;
		}

		if (!node) {
			if (!isFinalNode) {	
				// We couldn't traverse the directory structure.

				if (secondAttempt || !(flags & OS_OPEN_NODE_CREATE_DIRECTORIES)) {
					*error = OS_ERROR_PATH_NOT_TRAVERSABLE;
					CloseHandleToObject(parent, KERNEL_OBJECT_NODE, directoryAccess);
					return nullptr;
				} else {
					switch (filesystem->type) {
						case FILESYSTEM_ESFS: {
							if (!EsFSCreate(entry, entryLength, OS_NODE_DIRECTORY, parent)) {
							        *error = OS_ERROR_FILE_DOES_NOT_EXIST;
							        CloseHandleToObject(parent, KERNEL_OBJECT_NODE, directoryAccess);
							        return nullptr;
							}
						} break;
					}

					node = parent;
					secondAttempt = true;
					goto tryAgain;
				}
			}

			if (desiredFlags != flags) {
				// We couldn't only the file with the desired access flags.
				uint64_t difference = desiredFlags ^ flags;

				if (difference & (OS_OPEN_NODE_READ_ACCESS | OS_OPEN_NODE_WRITE_ACCESS | OS_OPEN_NODE_RESIZE_ACCESS)) {
					*error = OS_ERROR_FILE_IN_EXCLUSIVE_USE;
				} else if (difference & (OS_OPEN_NODE_READ_BLOCK | OS_OPEN_NODE_WRITE_BLOCK | OS_OPEN_NODE_RESIZE_BLOCK)) {
					*error = OS_ERROR_FILE_CANNOT_GET_EXCLUSIVE_USE;
				} else if (difference & (OS_OPEN_NODE_DIRECTORY)) {
					*error = OS_ERROR_INCORRECT_NODE_TYPE;
				} else {
					*error = OS_ERROR_UNKNOWN_OPERATION_FAILURE;
				}

				CloseHandleToObject(parent, KERNEL_OBJECT_NODE, directoryAccess);
				return nullptr;
			}

			// The file does not exist.

			if (secondAttempt || (flags & OS_OPEN_NODE_FAIL_IF_NOT_FOUND)) {
				*error = OS_ERROR_FILE_DOES_NOT_EXIST;
				CloseHandleToObject(parent, KERNEL_OBJECT_NODE, directoryAccess);
				return nullptr;
			} else {
				switch (filesystem->type) {
					case FILESYSTEM_ESFS: {
						if (!EsFSCreate(entry, entryLength, (flags & OS_OPEN_NODE_DIRECTORY) ? OS_NODE_DIRECTORY : OS_NODE_FILE, parent)) {
							*error = OS_ERROR_FILE_DOES_NOT_EXIST;
							CloseHandleToObject(parent, KERNEL_OBJECT_NODE, directoryAccess);
							return nullptr;
						}
					} break;
				}

				node = parent;
				secondAttempt = true;
				flags &= ~OS_OPEN_NODE_FAIL_IF_FOUND;
				goto tryAgain;
			}
		} 

		secondAttempt = false;
	}

	if (node && (flags & OS_OPEN_NODE_FAIL_IF_FOUND)) {
		CloseNode(node, flags);
		*error = OS_ERROR_FILE_ALREADY_EXISTS;
		return nullptr;
	}

	*error = OS_SUCCESS;
	return node;
}

void VFS::NodeMapped(Node *node) {
	if (node->parent) {
		NodeMapped(node->parent);
	}

	nodeHashTableMutex.Acquire();
	Defer(nodeHashTableMutex.Release());

	if (node->handles == 0) {
		KernelPanic("VFS::NodeMapped - Mapped a node with 0 handles.\n");
	}

	node->handles++;
}

Node *VFS::RegisterNodeHandle(void *_existingNode, uint64_t &flags, UniqueIdentifier identifier, Node *parent, OSNodeType type, bool isNodeNew) {
	if (parent && !parent->filesystem) {
		KernelPanic("VFS::RegisterNodeHandle - Trying to register a node without a filesystem.\n");
	}

	Node *existingNode = (Node *) _existingNode;
	existingNode->data.type = type;

	nodeHashTableMutex.Acquire();
	Defer(nodeHashTableMutex.Release());

	if ((flags & OS_OPEN_NODE_READ_BLOCK)    && (existingNode->countRead))   { flags ^= OS_OPEN_NODE_READ_BLOCK;    return nullptr; }
	if ((flags & OS_OPEN_NODE_READ_ACCESS)   && (existingNode->blockRead))   { flags ^= OS_OPEN_NODE_READ_ACCESS;   return nullptr; }
	if ((flags & OS_OPEN_NODE_WRITE_BLOCK)   && (existingNode->countWrite))  { flags ^= OS_OPEN_NODE_WRITE_BLOCK;   return nullptr; }
	if ((flags & OS_OPEN_NODE_WRITE_ACCESS)  && (existingNode->blockWrite))  { flags ^= OS_OPEN_NODE_WRITE_ACCESS;  return nullptr; }
	if ((flags & OS_OPEN_NODE_RESIZE_BLOCK)  && (existingNode->countResize)) { flags ^= OS_OPEN_NODE_RESIZE_BLOCK;  return nullptr; }
	if ((flags & OS_OPEN_NODE_RESIZE_ACCESS) && (existingNode->blockResize)) { flags ^= OS_OPEN_NODE_RESIZE_ACCESS; return nullptr; }

	if ((flags & OS_OPEN_NODE_DIRECTORY) && type != OS_NODE_DIRECTORY) {
		flags &= ~(OS_OPEN_NODE_DIRECTORY);
		return nullptr;
	}

	if ((flags & !OS_OPEN_NODE_DIRECTORY) && type == OS_NODE_DIRECTORY) {
		flags |= (OS_OPEN_NODE_DIRECTORY);
		return nullptr;
	}

	if ((flags & OS_OPEN_NODE_READ_BLOCK)   ) { existingNode->blockRead++; }
	if ((flags & OS_OPEN_NODE_READ_ACCESS)  ) { existingNode->countRead++; }
	if ((flags & OS_OPEN_NODE_WRITE_BLOCK)  ) { existingNode->blockWrite++; }
	if ((flags & OS_OPEN_NODE_WRITE_ACCESS) ) { existingNode->countWrite++; }
	if ((flags & OS_OPEN_NODE_RESIZE_BLOCK) ) { existingNode->blockResize++; }
	if ((flags & OS_OPEN_NODE_RESIZE_ACCESS)) { existingNode->countResize++; }

	if (existingNode->noHandleCacheItem.list) {
		vfs.cachedNodes.Remove(&existingNode->noHandleCacheItem);

		if (isNodeNew) {
			KernelPanic("VFS::RegisterNodeHandle - Cached node apparently new.\n");
		}
	}

	if (isNodeNew) {
		Node *newNode = existingNode;

		newNode->semaphore.Set(1);
		newNode->noHandleCacheItem.thisItem = newNode;

		newNode->region.node = newNode;

		sharedMemoryManager.mutex.Acquire();
		newNode->region.access = SharedMemoryRegion::COPY_ON_WRITE;
		sharedMemoryManager.ResizeSharedMemory(&newNode->region, newNode->data.file.fileSize);
		sharedMemoryManager.mutex.Release();
	}

	existingNode->handles++;

	if (isNodeNew && parent) {
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

	if (node && node->pointerToThisNodeInHashTableSlot != nodeHashTable + slot) {
		KernelPanic("VFS::FindOpenNode - Broken hash table.\n");
	}

	while (node) {
		if (node->filesystem == filesystem && !CompareBytes(&node->identifier, &identifier, sizeof(UniqueIdentifier)) && !node->deleted) {
			return node;
		}

		if (node == node->nextNodeInHashTableSlot) {
			KernelPanic("VFS::FindOpenNode - Broken hash table.\n");
		}

		Node **t = &node->nextNodeInHashTableSlot;
		node = node->nextNodeInHashTableSlot;

		if (node && node->pointerToThisNodeInHashTableSlot != t) {
			KernelPanic("VFS::FindOpenNode - Broken hash table.\n");
		}
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

void DetectFilesystem(Device *device) {
	uint8_t *information = (uint8_t *) OSHeapAllocate(device->block.sectorSize * 32, false);
	Defer(OSHeapFree(information));

	// Load the first 16KB of the drive to identify its filesystem.
	bool success = device->block.Access(nullptr, 0, 16384, DRIVE_ACCESS_READ, information);

	if (!success) {
		// We could not access the block device.
		return;
	}

	if (0) {
	} else if (((uint32_t *) information)[2048] == 0x65737345) {
		EsFSRegister(device);
	} else if (information[510] == 0x55 && information[511] == 0xAA && !device->block.sectorOffset /*Must be at start of drive*/) {
		// Check each partition in the table.
		for (uintptr_t i = 0; i < 4; i++) {
			for (uintptr_t j = 0; j < 0x10; j++) {
				if (information[j + 0x1BE + i * 0x10]) {
					goto partitionExists;
				}
			}

			continue;
			partitionExists:

			uint32_t offset = ((uint32_t) information[0x1BE + i * 0x10 + 8 ] << 0 )
				+ ((uint32_t) information[0x1BE + i * 0x10 + 9 ] << 8 )
				+ ((uint32_t) information[0x1BE + i * 0x10 + 10] << 16)
				+ ((uint32_t) information[0x1BE + i * 0x10 + 11] << 24);
			uint32_t count  = ((uint32_t) information[0x1BE + i * 0x10 + 12] << 0 )
				+ ((uint32_t) information[0x1BE + i * 0x10 + 13] << 8 )
				+ ((uint32_t) information[0x1BE + i * 0x10 + 14] << 16)
				+ ((uint32_t) information[0x1BE + i * 0x10 + 15] << 24);

			if (offset + count > device->block.sectorCount) {
				// The MBR is invalid.
				goto unknownFilesystem;
			}

			Device child;
			CopyMemory(&child, device, sizeof(Device));
			child.item = {};
			child.parent = device;
			child.block.sectorOffset += offset;
			child.block.sectorCount = count;
			deviceManager.Register(&child);
		}
	} else {
		unknownFilesystem:
		KernelLog(LOG_WARNING, "DeviceManager::Register - Could not detect filesystem or partition table on block device %d.\n", device->id);
	}
}

#endif
