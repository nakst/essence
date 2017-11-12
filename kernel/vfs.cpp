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

enum FilesystemType {
	FILESYSTEM_EXT2,
	FILESYSTEM_ESFS,
};

struct File {
	size_t Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer);
	size_t Write(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer);
	bool Resize(uint64_t newSize);
	void Sync();

	size_t countRead, countWrite, countResize;
	bool exclusiveRead, exclusiveWrite, exclusiveResize;

	Mutex mutex;
	uint64_t fileSize;
	UniqueIdentifier identifier;
	struct Filesystem *filesystem;
	volatile size_t handles;
	bool modifiedSinceLastSync;

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
	void CloseFile(File *file, uint64_t flags);

	File *OpenFileHandle(void *existingFile, uint64_t &flags, UniqueIdentifier identifier);
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
bool Ext2Scan(char *name, size_t nameLength, File **file, Filesystem *filesystem, uint64_t &flags);

bool EsFSRead(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, File *file);
bool EsFSWrite(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, File *file);
void EsFSSync(File *file);
bool EsFSScan(char *name, size_t nameLength, File **file, Filesystem *filesystem, uint64_t &flags);
bool EsFSResize(File *file, uint64_t newSize);

bool File::Resize(uint64_t newSize) {
	mutex.Acquire();
	Defer(mutex.Release());

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

void File::Sync() {
	mutex.Acquire();
	Defer(mutex.Release());

	if (!modifiedSinceLastSync) {
		return;
	}

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

size_t File::Write(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (offsetBytes > fileSize) {
		return 0;
	}

	if (offsetBytes + sizeBytes > fileSize) {
		sizeBytes = fileSize - offsetBytes;
	}

	if (sizeBytes > fileSize) {
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

size_t File::Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (offsetBytes > fileSize) {
		return 0;
	}

	if (offsetBytes + sizeBytes > fileSize) {
		sizeBytes = fileSize - offsetBytes;
	}

	if (sizeBytes > fileSize) {
		return 0;
	}

	switch (filesystem->type) {
		case FILESYSTEM_EXT2: {
			bool fail = Ext2Read(offsetBytes, sizeBytes, buffer, this);
			if (!fail) return 0;
		} break;

		case FILESYSTEM_ESFS: {
			bool fail = EsFSRead(offsetBytes, sizeBytes, buffer, this);
			if (!fail) return 0;
		} break;

		default: {
			KernelPanic("File::Read - Unsupported filesystem.\n");
		} break;
	}

	return sizeBytes;
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

void VFS::CloseFile(File *file, uint64_t flags) {
	file->mutex.Acquire();
	file->handles--;
	bool noHandles = file->handles == 0;

	if (flags & OS_OPEN_FILE_ACCESS_READ)      file->countRead--;
	if (flags & OS_OPEN_FILE_ACCESS_WRITE)     file->countWrite--;
	if (flags & OS_OPEN_FILE_ACCESS_RESIZE)    file->countResize--;

	if (flags & OS_OPEN_FILE_EXCLUSIVE_READ)   file->exclusiveRead = false;
	if (flags & OS_OPEN_FILE_EXCLUSIVE_WRITE)  file->exclusiveWrite = false;
	if (flags & OS_OPEN_FILE_EXCLUSIVE_RESIZE) file->exclusiveResize = false;

	// TODO Notify anyone waiting for the file to be accessible.

	file->mutex.Release();

	if (noHandles) {
		file->Sync();

		fileHashTableMutex.Acquire();
		*file->pointerToThisFileInHashTableSlot = file->nextFileInHashTableSlot;
		fileHashTableMutex.Release();

		OSHeapFree(file);
	}
}

File *VFS::OpenFile(char *name, size_t nameLength, uint64_t flags) {
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
	uint64_t temp = 0;
	File *directory = vfs.OpenFileHandle(mountpoint->root, temp, mountpoint->root->identifier);

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

		bool isActualFile = !nameLength;
		bool result;
		uint64_t temp = 0;

		switch (filesystem->type) {
			case FILESYSTEM_EXT2: {
				result = Ext2Scan(entry, entryLength, &directory, filesystem, isActualFile ? flags : temp);
			} break;

			case FILESYSTEM_ESFS: {
				result = EsFSScan(entry, entryLength, &directory, filesystem, isActualFile ? flags : temp);
			} break;

			default: {
				KernelPanic("VFS::OpenFile - Unimplemented filesystem type %d\n", filesystem->type);
			} break;
		}

		if (!result) {
			if (desiredFlags != flags) {
				// We couldn't only the file with the desired access flags.
				return nullptr;
			}

			// TODO If the flag OS_OPEN_FILE_FAIL_IF_NOT_FOUND is clear, 
			// 	then create the file.

			return nullptr;
		} 
	}

	if (directory && (flags & OS_OPEN_FILE_FAIL_IF_FOUND)) {
		CloseFile(directory, 0);
	}

	File *file = directory;
	file->filesystem = filesystem;
	return file;
}

File *VFS::OpenFileHandle(void *_existingFile, uint64_t &flags, UniqueIdentifier identifier) {
	File *existingFile = (File *) _existingFile;

	existingFile->mutex.Acquire();
	Defer(existingFile->mutex.Release());

	if ((flags & OS_OPEN_FILE_ACCESS_READ) && existingFile->exclusiveRead) {
		flags &= ~(OS_OPEN_FILE_ACCESS_READ);
		return nullptr;
	}

	if ((flags & OS_OPEN_FILE_ACCESS_WRITE) && existingFile->exclusiveWrite) {
		flags &= ~(OS_OPEN_FILE_ACCESS_WRITE);
		return nullptr;
	}

	if ((flags & OS_OPEN_FILE_ACCESS_RESIZE) && existingFile->exclusiveResize) {
		flags &= ~(OS_OPEN_FILE_ACCESS_RESIZE);
		return nullptr;
	}

	if ((flags & OS_OPEN_FILE_EXCLUSIVE_READ) && existingFile->countRead) {
		flags &= ~(OS_OPEN_FILE_EXCLUSIVE_READ);
		return nullptr;
	}

	if ((flags & OS_OPEN_FILE_EXCLUSIVE_WRITE) && existingFile->countWrite) {
		flags &= ~(OS_OPEN_FILE_EXCLUSIVE_WRITE);
		return nullptr;
	}

	if ((flags & OS_OPEN_FILE_EXCLUSIVE_RESIZE) && existingFile->countResize) {
		flags &= ~(OS_OPEN_FILE_EXCLUSIVE_RESIZE);
		return nullptr;
	}

	if (flags & OS_OPEN_FILE_ACCESS_READ)      existingFile->countRead++;
	if (flags & OS_OPEN_FILE_ACCESS_WRITE)     existingFile->countWrite++;
	if (flags & OS_OPEN_FILE_ACCESS_RESIZE)    existingFile->countResize++;

	if (flags & OS_OPEN_FILE_EXCLUSIVE_READ)   existingFile->exclusiveRead = true;
	if (flags & OS_OPEN_FILE_EXCLUSIVE_WRITE)  existingFile->exclusiveWrite = true;
	if (flags & OS_OPEN_FILE_EXCLUSIVE_RESIZE) existingFile->exclusiveResize = true;

	if (!existingFile->handles) {
		fileHashTableMutex.Acquire();
		uint16_t slot = ((uint16_t) identifier.d[0] + ((uint16_t) identifier.d[1] << 8)) & 0xFFF;
		existingFile->nextFileInHashTableSlot = fileHashTable[slot];
		if (fileHashTable[slot]) fileHashTable[slot]->pointerToThisFileInHashTableSlot = &existingFile->nextFileInHashTableSlot;
		fileHashTable[slot] = existingFile;
		existingFile->pointerToThisFileInHashTableSlot = fileHashTable + slot;
		fileHashTableMutex.Release();
	}

	existingFile->handles++;

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
