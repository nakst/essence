#ifdef IMPLEMENTATION

#define ESFS_HEADER
#include "../util/esfs.cpp"

struct EsFSVolume {
	File *Initialise(Device *_drive);

	uint64_t BlocksNeededToStore(uint64_t size);

	File *LoadRootDirectory();
	File *SearchDirectory(char *name, size_t nameLength, File *directory);

	bool AccessBlock(uint64_t block, uint64_t count, int operation, void *buffer);
	bool AccessExtent(EsFSGlobalExtent extent, int operation, void *buffer);
	bool AccessExtent(EsFSLocalExtent extent, int operation, void *buffer);
	bool AccessStream(EsFSAttributeFileData *data, uint64_t offset, uint64_t size, void *_buffer, bool write, uint64_t *lastAccessedActualBlock = nullptr);
	EsFSAttributeHeader *FindAttribute(uint16_t attribute, void *_attributeList);

	Device *drive;

	EsFSSuperblock superblock;
	size_t sectorsPerBlock;

	Mutex mutex;

	uint8_t blockBuffer[ESFS_MAX_BLOCK_SIZE]; 
};

struct EsFSFile {
	size_t fileEntryLength;
	// Followed by the file entry itself.
};

uint64_t EsFSVolume::BlocksNeededToStore(uint64_t size) {
	uint64_t blocks = size / superblock.blockSize;

	if (size % superblock.blockSize) {
		blocks++;
	}

	return blocks;
}

bool EsFSVolume::AccessBlock(uint64_t block, uint64_t count, int operation, void *buffer) {
	mutex.AssertLocked();
	return drive->block.Access(block * sectorsPerBlock, count * sectorsPerBlock, operation, (uint8_t *) buffer);
}

bool EsFSVolume::AccessExtent(EsFSGlobalExtent extent, int operation, void *buffer) {
	mutex.AssertLocked();
	return drive->block.Access(extent.offset * sectorsPerBlock, extent.count * sectorsPerBlock, operation, (uint8_t *) buffer);
}

bool EsFSVolume::AccessExtent(EsFSLocalExtent extent, int operation, void *buffer) {
	mutex.AssertLocked();
	return drive->block.Access(extent.offset * sectorsPerBlock, extent.count * sectorsPerBlock, operation, (uint8_t *) buffer);
}

EsFSAttributeHeader *EsFSVolume::FindAttribute(uint16_t attribute, void *_attributeList) {
	uint8_t *attributeList = (uint8_t *) _attributeList;
	EsFSAttributeHeader *header = (EsFSAttributeHeader *) attributeList;

	while (header->type != ESFS_ATTRIBUTE_LIST_END) {
		if (header->type == attribute) {
			return header;
		} else {
			header = (EsFSAttributeHeader *) (attributeList += header->size);
		}
	}

	if (attribute == ESFS_ATTRIBUTE_LIST_END) {
		return header;
	}

	return nullptr; // The list did not have the desired attribute
}

File *EsFSVolume::LoadRootDirectory() {
	mutex.Acquire();
	Defer(mutex.Release());

	void *root = OSHeapAllocate(superblock.blockSize * superblock.rootDirectoryFileEntry.count, false);
	Defer(OSHeapFree(root));

	if (!AccessExtent(superblock.rootDirectoryFileEntry, DRIVE_ACCESS_READ, root)) {
		return nullptr;
	}

	uint8_t *rootEnd = (uint8_t *) FindAttribute(ESFS_ATTRIBUTE_LIST_END, (EsFSFileEntry *) root + 1);
	size_t fileEntryLength = rootEnd - (uint8_t *) root;

	File *file = vfs.OpenFile(OSHeapAllocate(sizeof(File) + sizeof(EsFSFile) + fileEntryLength, true));
	EsFSFile *eFile = (EsFSFile *) (file + 1);
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);

	eFile->fileEntryLength = fileEntryLength;
	CopyMemory(fileEntry, root, fileEntryLength);

	CopyMemory(&file->identifier, &fileEntry->identifier, sizeof(UniqueIdentifier));

	return file;
}

File *EsFSVolume::Initialise(Device *_drive) {
	drive = _drive;
	
	// Load the superblock.
	EsFSSuperblockP *superblockP = (EsFSSuperblockP *) OSHeapAllocate(sizeof(EsFSSuperblockP), false);
	if (!drive->block.Access(8192 / drive->block.sectorSize,
			8192 / drive->block.sectorSize,
			DRIVE_ACCESS_READ, (uint8_t *) superblockP)) return nullptr;
	CopyMemory(&superblock, &superblockP->d, sizeof(EsFSSuperblock));
	Defer(OSHeapFree(superblockP));

	if (CompareBytes(superblock.signature, (void *) ESFS_SIGNATURE_STRING, ESFS_SIGNATURE_STRING_LENGTH)) {
		// The signature in the superblock was invalid.
		return nullptr;
	}

	if (superblock.requiredReadVersion > ESFS_DRIVER_VERSION) {
		// This driver is out of date.
		return nullptr;
	}

	if (superblock.requiredWriteVersion > ESFS_DRIVER_VERSION) {
		// This driver is out of date.
		return nullptr;
	}

	if (superblock.mounted) {
		// The drive is already mounted.
		KernelLog(LOG_WARNING, "Trying to mount an EssenceFS volume that was not unmounted correctly.\n");
		return nullptr;
	}

	// TODO Enable this when we have a proper shutdown/unmount facility.
#if 0
	// Save the mounted superblock.
	superblockP->d.mounted = true;
	if (!drive->block.Access(8192 / drive->block.sectorSize,
			8192 / drive->block.sectorSize,
			DRIVE_ACCESS_WRITE, (uint8_t *) superblockP)) return nullptr;
#endif

	sectorsPerBlock = superblock.blockSize / drive->block.sectorSize;

	KernelLog(LOG_INFO, "Initialising EssenceFS volume %s\n", ESFS_MAXIMUM_VOLUME_NAME_LENGTH, superblock.volumeName);
	return LoadRootDirectory();
}

bool EsFSVolume::AccessStream(EsFSAttributeFileData *data, uint64_t offset, uint64_t size, void *_buffer, bool write, uint64_t *lastAccessedActualBlock) {
	mutex.AssertLocked();

	if (!size) return true;

	if (data->indirection == ESFS_DATA_DIRECT) {
		if (write) {
			CopyMemory(data->direct + offset, _buffer, size);
		} else {
			CopyMemory(_buffer, data->direct + offset, size);
		}

		return true;
	}

	uint64_t offsetBlockAligned = offset & ~(superblock.blockSize - 1);
	uint64_t sizeBlocks = BlocksNeededToStore((size + (offset - offsetBlockAligned)));

	uint8_t *buffer = (uint8_t *) _buffer;

	EsFSGlobalExtent *i2ExtentList = nullptr;
	Defer(OSHeapFree(i2ExtentList));

	if (data->indirection == ESFS_DATA_INDIRECT_2) {
		i2ExtentList = (EsFSGlobalExtent *) OSHeapAllocate(BlocksNeededToStore(data->extentCount * sizeof(EsFSGlobalExtent)) * superblock.blockSize, false);

		for (int i = 0; i < ESFS_INDIRECT_2_EXTENTS; i++) {
			if (data->indirect2[i]) {
				if (!AccessBlock(data->indirect2[i], 1, DRIVE_ACCESS_READ, i2ExtentList + i * (superblock.blockSize / sizeof(EsFSGlobalExtent)))) {
					return false;
				}
			}
		}
	}
	
	for (uint64_t i = 0; i < sizeBlocks; i++) {
		// Work out which block contains this data.

		uint64_t blockInStream = offsetBlockAligned / superblock.blockSize + i;
		uint64_t globalBlock = 0;

		switch (data->indirection) {
			case ESFS_DATA_INDIRECT: {
				uint64_t p = 0;

				for (int i = 0; i < data->extentCount; i++) {
					if (blockInStream < p + data->indirect[i].count) {
						globalBlock = data->indirect[i].offset + blockInStream - p;
						break;
					} else {
						p += data->indirect[i].count;
					}
				}
			} break;	

			case ESFS_DATA_INDIRECT_2: {
				uint64_t p = 0;

				for (int i = 0; i < data->extentCount; i++) {
					if (blockInStream < p + i2ExtentList[i].count) {
						globalBlock = i2ExtentList[i].offset + blockInStream - p;
						break;
					} else {
						p += i2ExtentList[i].count;
					}
				}
			} break;

			default: {
				KernelPanic("EsFSVolume::AccessStream - Unsupported indirection format %d.\n", data->indirection);
			} break;
		}

		if (!globalBlock) {
			KernelPanic("EsFSVolume::AccessStream - Could not find block.\n");
		}

		// Access the modified data.

		uint64_t offsetIntoBlock = 0;
		uint64_t dataToTransfer = superblock.blockSize;

		if (i == 0){
			offsetIntoBlock = offset - offsetBlockAligned;
		} 
		
		if (i == sizeBlocks - 1) {
			dataToTransfer = size;
		}

		if (lastAccessedActualBlock) *lastAccessedActualBlock = globalBlock;

		if (write) {
			if (offsetIntoBlock || dataToTransfer != superblock.blockSize) {
				if (!AccessBlock(globalBlock, 1, DRIVE_ACCESS_READ, blockBuffer)) {
					return false;
				}
			}

			CopyMemory(blockBuffer + offsetIntoBlock, buffer, dataToTransfer);

			if (!AccessBlock(globalBlock, 1, DRIVE_ACCESS_WRITE, blockBuffer)) {
				return false;
			}
		} else {
			if (!AccessBlock(globalBlock, 1, DRIVE_ACCESS_READ, blockBuffer)) {
				return false;
			}

			CopyMemory(buffer, blockBuffer + offsetIntoBlock, dataToTransfer);
		}

		buffer += dataToTransfer;
		size -= dataToTransfer;
	}

	return true;
}

File *EsFSVolume::SearchDirectory(char *searchName, size_t nameLength, File *_directory) {
	mutex.Acquire();
	Defer(mutex.Release());

	EsFSFileEntry *fileEntry = (EsFSFileEntry *) ((EsFSFile *) (_directory + 1) + 1);

	EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) FindAttribute(ESFS_ATTRIBUTE_FILE_DIRECTORY, fileEntry + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (!directory) {
		KernelPanic("EsFSVolume::SearchDirectory - Directory did not have a directory attribute.\n");
	}

	if (!data) {
		KernelPanic("EsFSVolume::SearchDirectory - Directory did not have a data attribute.\n");
	}

	if (data->size == 0 || !directory->itemsInDirectory) {
		if (directory->itemsInDirectory) {
			KernelPanic("EsFSVolume::SearchDirectory - Directory had items but was 0 bytes.\n");
		}

		return nullptr;
	}

	uint8_t *directoryBuffer = (uint8_t *) OSHeapAllocate(superblock.blockSize, false);
	Defer(OSHeapFree(directoryBuffer));
	uint64_t blockPosition = 0, blockIndex = 0;
	uint64_t lastAccessedActualBlock = 0;
	AccessStream(data, blockIndex, superblock.blockSize, directoryBuffer, false, &lastAccessedActualBlock);

	size_t fileEntryLength;
	EsFSFileEntry *returnValue = nullptr;

	for (uint64_t i = 0; i < directory->itemsInDirectory; i++) {
		if (blockPosition == superblock.blockSize || !directoryBuffer[blockPosition]) {
			// We're reached the end of the block.
			// The next directory entry will be at the start of the next block.
			blockPosition = 0;
			blockIndex++;
			AccessStream(data, blockIndex * superblock.blockSize, superblock.blockSize, directoryBuffer, false, &lastAccessedActualBlock);
		}

		EsFSDirectoryEntry *entry = (EsFSDirectoryEntry *) (directoryBuffer + blockPosition);

		if (CompareBytes(entry->signature, (void *) ESFS_DIRECTORY_ENTRY_SIGNATURE, CStringLength((char *) ESFS_DIRECTORY_ENTRY_SIGNATURE))) {
			KernelPanic("EsFSVolume::SearchDirectory - Directory entry had invalid signature.\n");
		}

		EsFSAttributeDirectoryName *name = (EsFSAttributeDirectoryName *) FindAttribute(ESFS_ATTRIBUTE_DIRECTORY_NAME, entry + 1);
		if (!name) goto nextFile;
		if (name->nameLength != nameLength) goto nextFile;
		if (CompareBytes(name + 1, searchName, nameLength)) goto nextFile;

		{
			EsFSAttributeDirectoryFile *file = (EsFSAttributeDirectoryFile *) FindAttribute(ESFS_ATTRIBUTE_DIRECTORY_FILE, entry + 1);

			if (file) {
				returnValue = (EsFSFileEntry *) (file + 1);
				// loadInformation->positionInBlock = (uint64_t) returnValue - (uint64_t) directoryBuffer;
				fileEntryLength = file->header.size - sizeof(EsFSAttributeDirectoryFile);
			} else {
				// There isn't a file associated with this directory entry.
			}

			break;
		}

		nextFile:
		EsFSAttributeHeader *end = FindAttribute(ESFS_ATTRIBUTE_LIST_END, entry + 1);
		blockPosition += end->size + (uintptr_t) end - (uintptr_t) entry;
	}

	if (!returnValue) {
		return nullptr;
	}

	{
		EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, returnValue + 1);
		if (!data) return nullptr;

		File *file = vfs.OpenFile(OSHeapAllocate(sizeof(File) + sizeof(EsFSFile) + fileEntryLength, true));
		EsFSFile *eFile = (EsFSFile *) (file + 1);
		EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);

		eFile->fileEntryLength = fileEntryLength;
		CopyMemory(fileEntry, returnValue, fileEntryLength);

		file->fileSize = data->size;
		CopyMemory(&file->identifier, &fileEntry->identifier, sizeof(UniqueIdentifier));

		return file;
	}
}

inline bool EsFSScan(char *name, size_t nameLength, File **file, Filesystem *filesystem) {
	EsFSVolume *fs = (EsFSVolume *) filesystem->data;
	File *_file = fs->SearchDirectory(name, nameLength, *file);
	if (!_file) return false;
	*file = _file;
	return true;
}

inline bool EsFSRead(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, File *file) {
	file->mutex.Acquire();
	Defer(file->mutex.Release());

	EsFSVolume *fs = (EsFSVolume *) file->filesystem->data;
	fs->mutex.Acquire();
	Defer(fs->mutex.Release());

	EsFSFile *eFile = (EsFSFile *) (file + 1);
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) fs->FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);
	return fs->AccessStream(data, offsetBytes, sizeBytes, buffer, false);
}

#endif
