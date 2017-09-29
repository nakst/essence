#ifndef IMPLEMENTATION

// TODO How are locks going to work?
// 	What about exclusive file access during write/append?

enum FilesystemType {
	FILESYSTEM_EXT2,
};

struct File {
	bool Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer);

	// Filesystem-specific data stored by the driver retained between calls.
#define FILE_DRIVER_DATA 200
	uint8_t driverData[FILE_DRIVER_DATA];
	uint64_t fileSize;
	uint64_t inode; // A unique identifier of the file.

	struct Filesystem *filesystem;
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
	void RegisterFilesystem(File *root, FilesystemType type, void *data);
	File *OpenFile(char *name, size_t nameLength);
	void CloseFile(File *file);

	Pool filePool, filesystemPool, mountpointPool;
	LinkedList filesystems, mountpoints;

	bool foundBootFilesystem;

	Mutex lock;
};

VFS vfs;

bool Ext2Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, File *file);

#endif

#ifdef IMPLEMENTATION

bool File::Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer) {
	switch (filesystem->type) {
		case FILESYSTEM_EXT2: {
			return Ext2Read(offsetBytes, sizeBytes, buffer, this);
		} break;

		default: {
			KernelPanic("File::Read - Unsupported filesystem.\n");
			return false;
		} break;
	}
}

void VFS::Initialise() {
	filePool.Initialise(sizeof(File));
	filesystemPool.Initialise(sizeof(Filesystem));
	mountpointPool.Initialise(sizeof(Mountpoint));
}

bool Ext2FSScan(char *name, size_t nameLength, File *file, Filesystem *filesystem);

void VFS::CloseFile(File *file) {
	vfs.filePool.Remove(file);
}

File *VFS::OpenFile(char *name, size_t nameLength) {
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

	File directory;
	Filesystem *filesystem = mountpoint->filesystem;
	CopyMemory(&directory, mountpoint->root, sizeof(File));

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

			default: {
				KernelPanic("VFS::OpenFile - Unimplemented filesystem type %d\n", filesystem->type);
			} break;
		}

		if (!result) {
			return nullptr;
		} 
	}

	File *file = (File *) vfs.filePool.Add();
	CopyMemory(file, &directory, sizeof(File));
	file->filesystem = filesystem;
	return file;
}

void VFS::RegisterFilesystem(File *root, FilesystemType type, void *data) {
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
	mountpoint->pathLength = FormatString(mountpoint->path, MAX_PATH, OS_FOLDER "/fs%d/", filesystemID);
	mountpoint->allMountpointsItem.thisItem = mountpoint;
	mountpoint->filesystemMountpointsItem.thisItem = mountpoint;
	mountpoints.InsertEnd(&mountpoint->allMountpointsItem);
	filesystem->mountpoints.InsertEnd(&mountpoint->filesystemMountpointsItem);

	lock.Release();

	if (!foundBootFilesystem) {
		const size_t bufferSize = 32;
		char buffer[bufferSize];
		File *file = OpenFile(buffer, FormatString(buffer, bufferSize, OS_FOLDER "/fs%d" OS_FOLDER "/bootfsid", filesystemID));

		if (file && file->fileSize <= bufferSize) {
			file->Read(0, file->fileSize, (uint8_t *) buffer);

			char *bootFSID = (char *) "1234";

			if (!CompareBytes(buffer, bootFSID, CStringLength(bootFSID))) {
				foundBootFilesystem = true;

				lock.Acquire();

				// TODO Security: What if the filesystem is removed before we get here?
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

			CloseFile(file);
		}
	}
}

File *OpenOSFile(const char *name, bool required = false) {
	char path[256]; // Probably enough.
	size_t length = FormatString(path, 256, OS_FOLDER "/%z", name);
	File *file = vfs.OpenFile(path, length);

	if (!file && required) {
		KernelPanic("OpenOSFile - Could not open required OS file %z.\n", name);
	}
	
	return file;
}

#endif
