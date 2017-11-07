#ifndef IMPLEMENTATION

enum FilesystemType {
	FILESYSTEM_EXT2,
	FILESYSTEM_ESFS,
};

struct File {
	bool Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer);

	Mutex mutex;
	uint64_t fileSize;
	UniqueIdentifier identifier;
	struct Filesystem *filesystem;
	volatile size_t handles;

	struct File *nextFileInHashTableSlot;
	struct File **pointerToThisFileInHashTableSlot;
};

struct Filesystem {
	FilesystemType type;
	File *root;
	LinkedList mountpoints;
	LinkedItem allFilesystemsItem;
	void *data;
};

struct Mountpoint {
	File *root;
	char path[MAX_PATH];
	size_t pathLength;
	Filesystem *filesystem;

	LinkedItem filesystemMountpointsItem;
	LinkedItem allMountpointsItem;
};

struct VFS {
	void Initialise();
	Filesystem *RegisterFilesystem(File *root, FilesystemType type, void *data, UniqueIdentifier installationID);
	File *OpenFile(char *name, size_t nameLength, uint64_t flags);
	void CloseFile(File *file);

	File *OpenFileHandle(void *existingFile);
	File *FindFile(UniqueIdentifier identifier, Filesystem *filesystem);

	Pool filesystemPool, mountpointPool;
	LinkedList filesystems, mountpoints;
	Mutex filesystemsMutex, mountpointsMutex;

	bool foundBootFilesystem;

#define FILE_HASH_TABLE_BITS (12)
	File *fileHashTable[1 << FILE_HASH_TABLE_BITS];
	Mutex fileHashTableMutex;
};

VFS vfs;

#endif

#ifdef IMPLEMENTATION

bool Ext2Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, File *file);
bool EsFSRead(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, File *file);

bool File::Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer) {
	switch (filesystem->type) {
		case FILESYSTEM_EXT2: {
			return Ext2Read(offsetBytes, sizeBytes, buffer, this);
		} break;

		case FILESYSTEM_ESFS: {
			return EsFSRead(offsetBytes, sizeBytes, buffer, this);
		} break;

		default: {
			KernelPanic("File::Read - Unsupported filesystem.\n");
			return false;
		} break;
	}
}

void VFS::Initialise() {
	filesystemPool.Initialise(sizeof(Filesystem));
	mountpointPool.Initialise(sizeof(Mountpoint));

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

bool Ext2FSScan(char *name, size_t nameLength, File **file, Filesystem *filesystem);
bool EsFSScan(char *name, size_t nameLength, File **file, Filesystem *filesystem);

void VFS::CloseFile(File *file) {
	file->mutex.Acquire();
	file->handles--;
	bool noHandles = file->handles == 0;
	file->mutex.Release();

	if (noHandles) {
		fileHashTableMutex.Acquire();
		*file->pointerToThisFileInHashTableSlot = file->nextFileInHashTableSlot;
		fileHashTableMutex.Release();

		OSHeapFree(file);
	}
}

File *VFS::OpenFile(char *name, size_t nameLength, uint64_t flags) {
	(void) flags;
	KernelLog(LOG_VERBOSE, "Opening file: %s\n", nameLength, name);

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
	File *directory = vfs.OpenFileHandle(mountpoint->root);

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

		bool result;

		switch (filesystem->type) {
			case FILESYSTEM_EXT2: {
				result = Ext2FSScan(entry, entryLength, &directory, filesystem);
			} break;

			case FILESYSTEM_ESFS: {
				result = EsFSScan(entry, entryLength, &directory, filesystem);
			} break;

			default: {
				KernelPanic("VFS::OpenFile - Unimplemented filesystem type %d\n", filesystem->type);
			} break;
		}

		if (!result) {
			return nullptr;
		} 
	}

	File *file = directory;
	file->filesystem = filesystem;
	return file;
}

File *VFS::OpenFileHandle(void *_existingFile) {
	File *existingFile = (File *) _existingFile;

	existingFile->mutex.Acquire();

	if (!existingFile->handles) {
		fileHashTableMutex.Acquire();
		uint16_t slot = ((uint16_t) existingFile->identifier.d[0] + ((uint16_t) existingFile->identifier.d[1] << 8)) & 0xFFF;
		existingFile->nextFileInHashTableSlot = fileHashTable[slot];
		if (fileHashTable[slot]) fileHashTable[slot]->pointerToThisFileInHashTableSlot = &existingFile->nextFileInHashTableSlot;
		fileHashTable[slot] = existingFile;
		existingFile->pointerToThisFileInHashTableSlot = fileHashTable + slot;
		fileHashTableMutex.Release();
	} else {
		existingFile->handles++;
	}

	existingFile->mutex.Release();

	return existingFile;
}

File *VFS::FindFile(UniqueIdentifier identifier, Filesystem *filesystem) {
	fileHashTableMutex.Acquire();
	Defer(fileHashTableMutex.Release());

	uint16_t slot = ((uint16_t) identifier.d[0] + ((uint16_t) identifier.d[1] << 8)) & 0xFFF;

	File *file = fileHashTable[slot];

	while (file) {
		if (file->filesystem == filesystem 
				&& !CompareBytes(&file->identifier, &identifier, sizeof(UniqueIdentifier))
				&& file->handles) {
			return file;
		}

		file = file->nextFileInHashTableSlot;
	}

	return nullptr; // The file has not been opened.
}

Filesystem *VFS::RegisterFilesystem(File *root, FilesystemType type, void *data, UniqueIdentifier fsInstallationID) {
	filesystemsMutex.Acquire();
	mountpointsMutex.Acquire();

	uintptr_t filesystemID = filesystems.count;

	Filesystem *filesystem = (Filesystem *) filesystemPool.Add();
	filesystem->allFilesystemsItem.thisItem = filesystem;
	filesystem->type = type;
	filesystem->root = root;
	filesystem->data = data;
	filesystems.InsertEnd(&filesystem->allFilesystemsItem);

	Mountpoint *mountpoint = (Mountpoint *) mountpointPool.Add();
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
		Mountpoint *mountpoint = (Mountpoint *) mountpointPool.Add();
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
