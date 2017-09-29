#ifdef IMPLEMENTATION

struct Ext2Superblock {
	uint32_t totalInodes;
	uint32_t totalBlocks;
	uint32_t superuserReservedBlocks;
	uint32_t unallocatedBlocks;
	uint32_t unallocatedInodes;
	uint32_t superblockBlock;
	uint32_t blockSize; // As a power of 2 multiplied by 1024
	uint32_t fragmentSize; // As a power of 2 multiplied by 1024
	uint32_t blocksPerBlockGroup;
	uint32_t fragmentsPerBlockGroup;
	uint32_t inodesPerBlockGroup;
	uint32_t lastMountTime;
	uint32_t lastWriteTime;
	uint16_t modificationCount; // Reset by checking the filesystem for errors
	uint16_t mountsAllowedBeforeCheck;
	uint16_t signature; // 0xEF53
	uint16_t fsState; // 1 = clean, 2 = has errors
	uint16_t errorHandler; // 1 = ignore, 2 = read-only, 3 = panic
	uint16_t versionMinor;
	uint32_t lastCheckTime;
	uint32_t timeAllowedBeforeCheck;
	uint32_t creationOperatingSystem; // 0 = Linux, 3 = FreeBSD...
	uint32_t versionMajor;
	uint16_t userIDReserve;
	uint16_t groupIDReserve;

	// The following fields are available is the major version is >= 1
	uint32_t firstNonReservedInode;
	uint16_t inodeStructureSize;
	uint16_t superblockBlockGroup;
	uint32_t optionalFeatures; // 1 = preallocate directories, 2 = AFS server, 4 = has journal, 8 = extended inodes, 16 = resizable, 32 = hash index directories
	uint32_t requiredFeatures; // 1 = compression, 2 = type field in directories, 4 = replay journal, 8 = has journal
	uint32_t readOnlyFeatures; // 1 = sparse superblocks, 2 = 64-bit file size, 4 = binary tree directories
	char fileSystemID[16];
	char volumeName[16]; // Zero terminated
	char lastMountPath[64]; // Zero terminated
	uint32_t compressionAlgorithm;
	uint8_t preallocateBlocksFiles;
	uint8_t preallocateBlocksDirectory;
	uint16_t _unused0;
	char journalID[16];
	uint32_t journalInode;
	uint32_t journalDevice;
	uint32_t inodeOrphanHead;
	char _unused1[1024 - 236];
};

struct Ext2BlockGroupDescriptor {
	uint32_t blockUsageBitmap;
	uint32_t inodeUsageBitmap;
	uint32_t inodeTableBlock;
	uint16_t unallocatedBlocks;
	uint16_t unallocatedInodes;
	uint16_t directoriesInGroup;
	char _unused0[32 - 18];
};

struct Ext2InodeData {
	uint16_t typeAndPermissions; // lower 12 bits are normal UNIX permissions,
				     // upper 4 bits: 1 = FIFO, 2 = character device, 4 = directory, 6 = block device, 8 = file, 10 = symbolic link, 12 = socket
	uint16_t userIDLow;
	uint32_t sizeLow;
	uint32_t lastAccessTime;
	uint32_t creationTime;
	uint32_t lastModificationTime;
	uint32_t deletionTime;
	uint16_t groupIDLow;
	uint16_t hardLinksToSelf;
	uint32_t diskSectorsUsed;
	uint32_t flags; // 4 = compressed, 8 = synchronous updates, 16 = immutable, 32 = append only, 64 = do not backup, 128 = do not update last accessed time
			// 0x10000 = hash indexed directory, 0x20000 = AFS directory, 0x40000 = journal file data
	uint32_t _unused0;
	uint32_t directBlocks[12];
	uint32_t indirectBlocks[3];
	uint32_t generation;
	uint32_t fileACL; 
	union {
		uint32_t sizeHigh;
		uint32_t directoryACL;
	};
	uint32_t fragmentBlock;
	uint8_t fragmentNumber;
	uint8_t fragmentSize;
	uint16_t _unused1;
	uint16_t userIDHigh;
	uint16_t groupIDHigh;
	uint32_t _unused2;
};

struct Ext2DirectoryEntry {
	uint32_t inode;
	uint16_t entrySize; // i.e. how far until next entry in bytes
	uint8_t nameLengthLow;

	union {
		uint8_t nameLengthHigh;
		uint8_t typeIndicator; // Required feature value 2
				       // 0 = unknown, 1 = file, 2 = directory, 3 = character device, 4 = block device, 5 = FIFO, 6 = socket, 7 = soft link
	};

	// Name and padding follows.
};

// Stored the in the driverData field in a File.
struct Ext2File {
	Ext2InodeData data;
};

struct Ext2FS {
	bool AccessBlock(uint64_t block, uint8_t *buffer, int operation = DRIVE_ACCESS_READ, size_t count = 1);
	bool AccessFile(File *file, uint64_t offsetBytes, uint64_t sizeBytes, int operation, uint8_t *buffer, bool lockAlreadyAcquired = false);
	bool OpenFile(uint64_t inode, File *file);
	File *Initialise(Device *drive); // Returns root directory.

	// 'name' is a UTF-8 string; '..' is the parent directory and '.' is the current directory
	uint64_t ScanDirectory(char *name, size_t nameLength, File *directory); 

	Ext2Superblock superblock; // Read at mount, written at unmount
	Device *drive;
	uint64_t sectorsPerBlock;
	uint64_t maxDeviceAccessBlocks;
	size_t bytesPerBlock;
	uint8_t buffer[4096 * 5];

	Mutex lock;
};

#define EXT2_FIRST_INODE 1
#define EXT2_ROOT_DIRECTORY_INODE 2

bool Ext2FS::AccessBlock(uint64_t block, uint8_t *buffer, int operation, size_t count) {
	lock.AssertLocked();
	return drive->block.Access(block * sectorsPerBlock, sectorsPerBlock * count, operation, buffer);
}

bool Ext2FS::AccessFile(File *file, uint64_t offsetBytes, uint64_t sizeBytes, int operation, uint8_t *_buffer, bool lockAlreadyAcquired) {
	if (!file) return false;
	if (offsetBytes + sizeBytes > file->fileSize) return false;

	Ext2InodeData *data = (Ext2InodeData *) file->driverData;
	uint32_t blocksInBuffers[] = {0, 0, 0};
	uint32_t bytesRemaining = sizeBytes;
	uint32_t blocksPerBlock = bytesPerBlock / 4;
	uint64_t position = 0;

	if (!lockAlreadyAcquired) lock.Acquire();
	Defer(if (!lockAlreadyAcquired) lock.Release());

	uint64_t sequenceBlock = 0;
	uintptr_t sequencePosition = 0;
	size_t sequenceLength = 0;

	// For every block of data to load:
	for (uint32_t i = offsetBytes;
			i < offsetBytes + sizeBytes;
			i += bytesPerBlock, bytesRemaining -= bytesPerBlock) {
		uint64_t block = i / bytesPerBlock;

		// Work out which block to load the data from.
		if (block < 12) {
			block = data->directBlocks[block];
		} else if (block < 12 + blocksPerBlock) {
			uint32_t offsetBlock = block - 12;
			uint32_t b1 = data->indirectBlocks[0];
			if (blocksInBuffers[0] != b1) AccessBlock(blocksInBuffers[0] = b1, buffer + bytesPerBlock * 1);
			block = ((uint32_t *) (buffer + bytesPerBlock * 1))[offsetBlock];
		} else if (block < 12 + blocksPerBlock + blocksPerBlock * blocksPerBlock) {
			uint32_t offsetBlock = block - 12 - blocksPerBlock;
			uint32_t b1 = data->indirectBlocks[1];
			if (blocksInBuffers[0] != b1) AccessBlock(blocksInBuffers[0] = b1, buffer + bytesPerBlock * 1);
			uint32_t b2 = ((uint32_t *) (buffer + bytesPerBlock * 1))[offsetBlock / blocksPerBlock];
			if (blocksInBuffers[1] != b2) AccessBlock(blocksInBuffers[1] = b2, buffer + bytesPerBlock * 2);
			block = ((uint32_t *) (buffer + bytesPerBlock * 2))[offsetBlock % blocksPerBlock];
		} else {
			uint32_t offsetBlock = block - 12 - blocksPerBlock - blocksPerBlock * blocksPerBlock;
			uint32_t b1 = data->indirectBlocks[2];
			if (blocksInBuffers[0] != b1) AccessBlock(blocksInBuffers[0] = b1, buffer + bytesPerBlock * 1);
			uint32_t b2 = ((uint32_t *) (buffer + bytesPerBlock * 1))[(offsetBlock / blocksPerBlock) / blocksPerBlock];
			if (blocksInBuffers[1] != b2) AccessBlock(blocksInBuffers[1] = b2, buffer + bytesPerBlock * 2);
			uint32_t b3 = ((uint32_t *) (buffer + bytesPerBlock * 2))[(offsetBlock / blocksPerBlock) % blocksPerBlock];
			if (blocksInBuffers[2] != b3) AccessBlock(blocksInBuffers[2] = b3, buffer + bytesPerBlock * 3);
			block = ((uint32_t *) (buffer + bytesPerBlock * 3))[offsetBlock % blocksPerBlock];
		}

		// Work out how many bytes to copy.
		size_t bytesToCopy = bytesPerBlock;
		size_t destinationOffset = 0;

		if (i == offsetBytes) {
			destinationOffset = i % bytesPerBlock;
			bytesToCopy -= destinationOffset;
		}

		if (bytesToCopy > bytesRemaining) {
			bytesToCopy = bytesRemaining;
		}

		// Flush the sequence if it's full or we cannot add to it.
		if (destinationOffset != 0 || bytesToCopy != bytesPerBlock
				|| sequenceLength == maxDeviceAccessBlocks || sequenceBlock + sequenceLength != block) {
			if (sequenceLength) {
				if (!AccessBlock(sequenceBlock, _buffer + sequencePosition, operation, sequenceLength)) {
					return false;
				}
			}

			sequenceLength = 0;
		}

		// Load/store the block.
		if (destinationOffset == 0 && bytesToCopy == bytesPerBlock) {
			if (!sequenceLength) {
				sequenceBlock = block;
				sequencePosition = position;
			}

			sequenceLength++;
		} else {
			if (operation == DRIVE_ACCESS_WRITE) {
				CopyMemory(buffer + destinationOffset, _buffer + position, bytesToCopy);
			}

			if (!AccessBlock(block, buffer + bytesPerBlock * 0, operation)) {
				return false;
			}

			if (operation == DRIVE_ACCESS_READ) {
				CopyMemory(_buffer + position, buffer + destinationOffset, bytesToCopy);
			}
		}

		position += bytesToCopy;
	}

	// Flush the sequence at the end.
	if (sequenceLength) {
		if (!AccessBlock(sequenceBlock, _buffer + sequencePosition, operation, sequenceLength)) {
			return false;
		}
	}

	return true;
}

uint64_t Ext2FS::ScanDirectory(char *name, size_t nameLength, File *directory) {
	uint64_t position = 0;
	uint64_t directoryBufferPosition = 0;
	uint64_t remaining = directory->fileSize;
	uint8_t *directoryBuffer = buffer + 4096 * 4;

	lock.Acquire();
	Defer(lock.Release());

	while (position < directory->fileSize) {
		// This works because directory entries cannot span over a block, and the maximum block size is 4KiB.
		if (position >= directoryBufferPosition + 4096 || !position) {
			if (!AccessFile(directory, position, remaining > 4096 ? 4096 : remaining, 
						DRIVE_ACCESS_READ, directoryBuffer, true)) {
				return 0;
			}

			directoryBufferPosition = position;
		}

		Ext2DirectoryEntry *entry = (Ext2DirectoryEntry *) (directoryBuffer + (position - directoryBufferPosition));
		char *name2 = (char *) (entry + 1);

		if (!entry->inode) goto nextFile;

		if (superblock.requiredFeatures & 2) {
			if (nameLength != entry->nameLengthLow) {
				goto nextFile;
			}
		} else {
			if (nameLength != (size_t) entry->nameLengthLow + ((size_t) entry->nameLengthHigh << 8)) {
				goto nextFile;
			}
		}

		for (uintptr_t i = 0; i < nameLength; i++) {
			if (name[i] != name2[i]) {
				goto nextFile;
			}
		}

		return entry->inode;

		nextFile:
		position += entry->entrySize;
		remaining -= entry->entrySize;
	}
	
	return 0;
}

bool Ext2FS::OpenFile(uint64_t inode, File *file) {
	// Check that the inode is within a valid range.
	if (inode < EXT2_FIRST_INODE || inode >= superblock.totalInodes) return false;

	lock.Acquire();
	Defer(lock.Release());

	// Load the block group descriptor.
	uint64_t blockGroup = (inode - EXT2_FIRST_INODE) / superblock.inodesPerBlockGroup;
	uint64_t bgdBlock = blockGroup * sizeof(Ext2BlockGroupDescriptor) / bytesPerBlock;
	uint64_t bgdBlockIndex = (blockGroup * sizeof(Ext2BlockGroupDescriptor) % bytesPerBlock) / sizeof(Ext2BlockGroupDescriptor);
	if (!AccessBlock((superblock.blockSize ? 1 : 2) + bgdBlock, buffer)) return false;
	Ext2BlockGroupDescriptor *blockGroupDescriptor = (Ext2BlockGroupDescriptor *) buffer + bgdBlockIndex;

	// Load the inode data.
	uint64_t inodeIndex = (inode - EXT2_FIRST_INODE) % superblock.inodesPerBlockGroup;
	uint64_t inodeTableBlock = inodeIndex * superblock.inodeStructureSize / bytesPerBlock;
	uint64_t inodeTableBlockIndex = (inodeIndex * superblock.inodeStructureSize % bytesPerBlock) / superblock.inodeStructureSize;
	if (!AccessBlock(blockGroupDescriptor->inodeTableBlock + inodeTableBlock, buffer)) return false;
	Ext2InodeData *inodeData = (Ext2InodeData *) buffer + inodeTableBlockIndex;

	// Check that the data we need to store can fit in the File struct.
	if (sizeof(Ext2File) > FILE_DRIVER_DATA) {
		KernelPanic("Ext2FS::OpenFile - sizeof(Ext2File) is greater than FILE_DRIVER_DATA\n");
	}

	// Store the file information.
	ZeroMemory(file, sizeof(File));
	Ext2File *driverData = (Ext2File *) &file->driverData;
	CopyMemory(&driverData->data, inodeData, sizeof(Ext2InodeData));
	file->inode = inode;
	file->fileSize = (uint64_t) inodeData->sizeLow + ((inodeData->typeAndPermissions & 0x4000) ? 0 : ((uint64_t) inodeData->sizeHigh << 32));

	return true;
}

File *Ext2FS::Initialise(Device *_drive) {
	// Load the superblock and calculate basic filesystem information.
	drive = _drive;
	drive->block.Access(1024 / drive->block.sectorSize, 1024 / drive->block.sectorSize, DRIVE_ACCESS_READ, (uint8_t *) &superblock);
	bytesPerBlock = 1024 << superblock.blockSize;
	sectorsPerBlock = bytesPerBlock / drive->block.sectorSize;
	maxDeviceAccessBlocks = drive->block.maxAccessSectorCount / sectorsPerBlock;
	if (!sectorsPerBlock) return nullptr;

	// Check that the filesystem can be read/written.
	if (superblock.signature != 0xEF53) return nullptr;
	if (superblock.fsState != 1) return nullptr;
	if (superblock.versionMajor < 1) return nullptr;
	if (superblock.requiredFeatures & ~2) return nullptr;

	File *root = (File *) vfs.filePool.Add();
	return OpenFile(EXT2_ROOT_DIRECTORY_INODE, root) ? root : nullptr;
}

inline bool Ext2FSScan(char *name, size_t nameLength, File *file, Filesystem *filesystem) {
	Ext2FS *fs = (Ext2FS *) filesystem->data;
	uintptr_t inode = fs->ScanDirectory(name, nameLength, file);
	if (inode) return fs->OpenFile(inode, file);
	else return false; 
}

inline bool Ext2Read(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, File *file) {
	Ext2FS *fs = (Ext2FS *) file->filesystem->data;
	return fs->AccessFile(file, offsetBytes, sizeBytes, DRIVE_ACCESS_READ, buffer);
}

#endif
