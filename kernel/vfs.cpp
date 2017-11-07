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
	void RegisterFilesystem(File *root, FilesystemType type, void *data, UniqueIdentifier installationID);
	File *OpenFile(char *name, size_t nameLength, uint64_t flags);
	File *OpenFile(void *existingFile);
	void CloseFile(File *file);

	Pool filesystemPool, mountpointPool;
	LinkedList filesystems, mountpoints;

	bool foundBootFilesystem;

	Mutex lock;
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
		OSHeapFree(file);
	}
}

File *VFS::OpenFile(char *name, size_t nameLength, uint64_t flags) {
	(void) flags;
	KernelLog(LOG_VERBOSE, "Opening file: %s\n", nameLength, name);

	lock.Acquire();
	Defer(lock.Release());

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

	if (!longestMatch) {
		// The requested file was not on any mounted filesystem.
		return nullptr;
	}

	Mountpoint *mountpoint = longestMatch;
	name += mountpoint->pathLength;
	nameLength -= mountpoint->pathLength;

	Filesystem *filesystem = mountpoint->filesystem;
	File *directory = vfs.OpenFile(mountpoint->root);

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

File *VFS::OpenFile(void *_existingFile) {
	File *existingFile = (File *) _existingFile;

	existingFile->mutex.Acquire();
	existingFile->handles++;
	existingFile->mutex.Release();

	return existingFile;
}

void VFS::RegisterFilesystem(File *root, FilesystemType type, void *data, UniqueIdentifier fsInstallationID) {
	lock.Acquire();

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

	lock.Release();

	if (!foundBootFilesystem) {
		// We currently only support booting on EssenceFS volumes.
		if (type != FILESYSTEM_ESFS) {
			return;
		}

		// This wasn't the boot volume.
		if (CompareBytes(&fsInstallationID, &installationID, sizeof(UniqueIdentifier))) {
			return;
		}

		// Mount the volume at root.
		foundBootFilesystem = true;
		lock.Acquire();
		Mountpoint *mountpoint = (Mountpoint *) mountpointPool.Add();
		mountpoint->root = root;
		mountpoint->filesystem = filesystem;
		mountpoint->pathLength = FormatString(mountpoint->path, MAX_PATH, "/");
		mountpoint->allMountpointsItem.thisItem = mountpoint;
		mountpoint->filesystemMountpointsItem.thisItem = mountpoint;
		mountpoints.InsertEnd(&mountpoint->allMountpointsItem);
		filesystem->mountpoints.InsertEnd(&mountpoint->filesystemMountpointsItem);
		lock.Release();
	}
}

#endif
