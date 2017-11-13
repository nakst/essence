// TODO Think more about locking.

#ifdef IMPLEMENTATION

#define ESFS_HEADER
#include "../util/esfs.cpp"

struct EsFSVolume {
	Node *Initialise(Device *_drive);

	uint64_t BlocksNeededToStore(uint64_t size);

	Node *LoadRootDirectory();
	Node *SearchDirectory(char *name, size_t nameLength, Node *directory, uint64_t &flags);

	bool AccessBlock(uint64_t block, uint64_t count, int operation, void *buffer, uint64_t offsetIntoBlock);
	bool AccessStream(EsFSAttributeFileData *data, uint64_t offset, uint64_t size, void *_buffer, bool write, uint64_t *lastAccessedActualBlock = nullptr);
	EsFSAttributeHeader *FindAttribute(uint16_t attribute, void *_attributeList);
	bool ResizeDataStream(EsFSAttributeFileData *data, uint64_t newSize, bool clearNewBlocks, uint64_t containerBlock);
	EsFSGlobalExtent AllocateExtent(uint64_t localGroup, uint64_t desiredBlocks);
	uint16_t GetBlocksInGroup(uint64_t group);

	Device *drive;
	Filesystem *filesystem;

	EsFSSuperblock superblock;
	EsFSGroupDescriptorP *groupDescriptorTable;
	size_t sectorsPerBlock;
};

struct EsFSFile {
	uint64_t containerBlock;
	uint64_t offsetIntoBlock;

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

bool EsFSVolume::AccessBlock(uint64_t block, uint64_t countBytes, int operation, void *buffer, uint64_t offsetIntoBlock) {
	bool result = drive->block.Access(block * sectorsPerBlock * drive->block.sectorSize + offsetIntoBlock, countBytes, operation, (uint8_t *) buffer);

	if (!result) {
		// TODO Bad block handling.
		// 	- Propagate "damaged file" error to program.
		// 	- Mark blocks as bad.
		// 	- Mark file as damaged?
		KernelPanic("EsFSVolume::AccessBlock - Could not access block %d (bytes = %d).\n", block, countBytes);
	}

	return true;
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

uint16_t EsFSVolume::GetBlocksInGroup(uint64_t group) {
	if (group == superblock.groupCount - 1) {
		return superblock.blockCount % superblock.blocksPerGroup;
	} else {
		return superblock.blocksPerGroup;
	}
}

Node *EsFSVolume::LoadRootDirectory() {
	void *root = OSHeapAllocate(superblock.blockSize * superblock.rootDirectoryFileEntry.count, false);
	Defer(OSHeapFree(root));

	if (!AccessBlock(superblock.rootDirectoryFileEntry.offset, superblock.blockSize, DRIVE_ACCESS_READ, root, 0)) {
		return nullptr;
	}

	uint8_t *rootEnd = (uint8_t *) FindAttribute(ESFS_ATTRIBUTE_LIST_END, (EsFSFileEntry *) root + 1);
	size_t fileEntryLength = rootEnd - (uint8_t *) root;

	uint64_t temp = 0;
	Node *node = vfs.RegisterNodeHandle(OSHeapAllocate(sizeof(Node) + sizeof(EsFSFile) + fileEntryLength, true), temp, ((EsFSFileEntry *) root)->identifier, nullptr, OS_NODE_DIRECTORY);
	EsFSFile *eFile = (EsFSFile *) (node + 1);
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);

	eFile->fileEntryLength = fileEntryLength;
	CopyMemory(fileEntry, root, fileEntryLength);

	EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) FindAttribute(ESFS_ATTRIBUTE_FILE_DIRECTORY, fileEntry + 1);
	node->data.type = OS_NODE_DIRECTORY;
	node->data.directory.entryCount = directory->itemsInDirectory;

	CopyMemory(&node->identifier, &fileEntry->identifier, sizeof(UniqueIdentifier));

	return node;
}

Node *EsFSVolume::Initialise(Device *_drive) {
	drive = _drive;
	
	// Load the superblock.
	EsFSSuperblockP *superblockP = (EsFSSuperblockP *) OSHeapAllocate(sizeof(EsFSSuperblockP), false);
	if (!drive->block.Access(8192, 8192, DRIVE_ACCESS_READ, (uint8_t *) superblockP)) return nullptr;
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

	// Read the group descriptor table.
	groupDescriptorTable = (EsFSGroupDescriptorP *) OSHeapAllocate(superblock.gdt.count * superblock.blockSize, false);
	AccessBlock(superblock.gdt.offset, superblock.gdt.count * superblock.blockSize, DRIVE_ACCESS_READ, groupDescriptorTable, 0);

	KernelLog(LOG_INFO, "Initialising EssenceFS volume %s\n", ESFS_MAXIMUM_VOLUME_NAME_LENGTH, superblock.volumeName);
	return LoadRootDirectory();
}

bool EsFSVolume::AccessStream(EsFSAttributeFileData *data, uint64_t offset, uint64_t size, void *_buffer, bool write, uint64_t *lastAccessedActualBlock) {
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
				if (!AccessBlock(data->indirect2[i], superblock.blockSize, DRIVE_ACCESS_READ, i2ExtentList + i * (superblock.blockSize / sizeof(EsFSGlobalExtent)), 0)) {
					return false;
				}
			}
		}
	}

	uint64_t blockInStream = offsetBlockAligned / superblock.blockSize;
#if 1
	uint64_t maxBlocksToFind = drive->block.maxAccessSectorCount * drive->block.sectorSize / superblock.blockSize;
#else
	uint64_t maxBlocksToFind = 1;
#endif
	uint64_t i = 0;

	while (sizeBlocks) {
		uint64_t globalBlock = 0;
		uint64_t blocksFound = 0;

		while (blocksFound < maxBlocksToFind && sizeBlocks) {
			uint64_t nextGlobalBlock = 0;

			switch (data->indirection) {
				case ESFS_DATA_INDIRECT: {
					uint64_t p = 0;

					for (int i = 0; i < data->extentCount; i++) {
						if (blockInStream < p + data->indirect[i].count) {
							nextGlobalBlock = data->indirect[i].offset + blockInStream - p;
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
							nextGlobalBlock = i2ExtentList[i].offset + blockInStream - p;
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
				globalBlock = nextGlobalBlock;
			} else if (nextGlobalBlock == globalBlock + blocksFound) {
				// Continue.
			} else {
				break;
			}

			blockInStream++;
			blocksFound++;
			sizeBlocks--;
		}

		if (!globalBlock) {
			KernelPanic("EsFSVolume::AccessStream - Could not find block.\n");
		}

		// Access the modified data.

		uint64_t offsetIntoBlock = 0;
		uint64_t dataToTransfer = superblock.blockSize * blocksFound;

		if (!i) {
			offsetIntoBlock = offset - offsetBlockAligned;
			dataToTransfer -= offsetIntoBlock;
		} 
		
		if (!sizeBlocks) {
			dataToTransfer = size; // Only transfer the remaining bytes.
		}

		if (lastAccessedActualBlock) *lastAccessedActualBlock = globalBlock;

		if (!AccessBlock(globalBlock, dataToTransfer, write ? DRIVE_ACCESS_WRITE : DRIVE_ACCESS_READ, buffer, offsetIntoBlock)) {
			return false; // Drive error.
		}

		buffer += dataToTransfer;
		size -= dataToTransfer;
		i++;
	}
	
	return true;
}

EsFSGlobalExtent EsFSVolume::AllocateExtent(uint64_t localGroup, uint64_t desiredBlocks) {
	// TODO Optimise this function.
	// 	- Cache extent tables.

	uint8_t *extentTableBuffer = (uint8_t *) OSHeapAllocate(superblock.blockSize, false);
	Defer(OSHeapFree(extentTableBuffer));

	uint64_t groupsSearched = 0;

	for (uint64_t blockGroup = localGroup; groupsSearched < superblock.groupCount; blockGroup = (blockGroup + 1) % superblock.groupCount, groupsSearched++) {
		EsFSGroupDescriptor *descriptor = &groupDescriptorTable[blockGroup].d;

		if (descriptor->blocksUsed == GetBlocksInGroup(blockGroup)) {
			continue;
		} 

		if (descriptor->extentCount * sizeof(EsFSLocalExtent) > ESFS_MAX_BLOCK_SIZE) {
			// This shouldn't happen as long as the number of blocks in a group does not exceed ESFS_MAX_BLOCK_SIZE.
			KernelPanic("EsFSVolume::AllocateExtent - Extent table larger than expected.\n");
		}

		if (!descriptor->extentTable) {
			// The group does not have an extent table allocated for it yet, so let's make it.
			descriptor->extentTable = blockGroup * superblock.blocksPerGroup;
			descriptor->extentCount = 1;
			descriptor->blocksUsed = superblock.blocksPerGroupExtentTable;

			EsFSLocalExtent *extent = (EsFSLocalExtent *) extentTableBuffer;
			extent->offset = superblock.blocksPerGroupExtentTable; 
			extent->count = GetBlocksInGroup(blockGroup) - superblock.blocksPerGroupExtentTable;

			// The table is saved at the end of the function.
		} else {
			AccessBlock(descriptor->extentTable, BlocksNeededToStore(descriptor->extentCount * sizeof(EsFSLocalExtent)) * superblock.blockSize, DRIVE_ACCESS_READ, extentTableBuffer, 0);
		}

		uint16_t largestSeenIndex = 0;
		EsFSLocalExtent *extentTable = (EsFSLocalExtent *) extentTableBuffer;
		EsFSGlobalExtent extent;

		// First, look for an extent with enough size for the whole allocation.
		for (uint16_t i = 0; i < descriptor->extentCount; i++) {
			if (extentTable[i].count > desiredBlocks) {
				extent.offset = extentTable[i].offset;
				extent.count = desiredBlocks;
				extentTable[i].offset += desiredBlocks;
				extentTable[i].count -= desiredBlocks;
				goto finish;
			} else if (extentTable[i].count == desiredBlocks) {
				extent.offset = extentTable[i].offset;
				extent.count = desiredBlocks;
				descriptor->extentCount--;
				extentTable[i] = extentTable[descriptor->extentCount];
				goto finish;
			} else {
				if (extent.count > extentTable[largestSeenIndex].count) {
					largestSeenIndex = i;
				}
			}
		}

		// If that didn't work, we'll have to do a partial allocation.
		extent.offset = extentTable[largestSeenIndex].offset;
		extent.count = extentTable[largestSeenIndex].count;
		descriptor->extentCount--;
		extentTable[largestSeenIndex] = extentTable[descriptor->extentCount];

		finish:

		extent.offset += blockGroup * superblock.blocksPerGroup;
		descriptor->blocksUsed += extent.count;
		superblock.blocksUsed += extent.count;

		AccessBlock(descriptor->extentTable, BlocksNeededToStore(descriptor->extentCount * sizeof(EsFSLocalExtent)) * superblock.blockSize, DRIVE_ACCESS_WRITE, extentTableBuffer, 0);

		return extent;
	}

	// If we get here then the disk is full!
	return {};
}

bool EsFSVolume::ResizeDataStream(EsFSAttributeFileData *data, uint64_t newSize, bool clearNewBlocks, uint64_t containerBlock) {
	if (!data) {
		return false;
	}

	uint64_t oldSize = data->size;
	data->size = newSize;

	uint64_t oldBlocks = BlocksNeededToStore(oldSize);
	uint64_t newBlocks = BlocksNeededToStore(newSize);

	if (oldSize > newSize) {
		KernelPanic("EsFSVolume::ResizeDataStream - File shrinking not implemented yet.\n");
	}

	uint8_t wasDirect = false;
	uint8_t directTemporary[ESFS_DIRECT_BYTES];

	if (newSize > ESFS_DIRECT_BYTES && data->indirection == ESFS_DATA_DIRECT) {
		// Change from direct to indirect.
		data->indirection = ESFS_DATA_INDIRECT;
		CopyMemory(directTemporary, data->direct, oldSize);
		wasDirect = true;
		oldBlocks = 0;
	} else if (data->indirection == ESFS_DATA_DIRECT) {
		return true; // We don't need to resize the file.
	}

	uint64_t increaseBlocks = newBlocks - oldBlocks;

	EsFSGlobalExtent *newExtentList = nullptr;
	Defer(OSHeapFree(newExtentList));

	uint64_t extentListMaxSize = ESFS_INDIRECT_2_EXTENTS * (superblock.blockSize / sizeof(EsFSGlobalExtent));
	uint64_t firstModifiedExtentListBlock = 0;

	while (increaseBlocks) {
		EsFSGlobalExtent newExtent = AllocateExtent(containerBlock / superblock.blocksPerGroup, increaseBlocks);
		if (!newExtent.count) return false;

		if (clearNewBlocks) {
			void *zeroData = OSHeapAllocate(superblock.blockSize * newExtent.count, true);
			Defer(OSHeapFree(zeroData));

			if (!AccessBlock(newExtent.offset, superblock.blockSize * newExtent.count, DRIVE_ACCESS_WRITE, zeroData, 0)) {
				return false;
			}
		}

		increaseBlocks -= newExtent.count;

		switch (data->indirection) {
			case ESFS_DATA_INDIRECT: {
				if (data->extentCount != ESFS_INDIRECT_EXTENTS) {
				        data->indirect[data->extentCount] = newExtent;
				        data->extentCount++;
				} else {
					// We need to convert this to ESFS_DATA_INDIRECT_2.
					data->indirection = ESFS_DATA_INDIRECT_2;
					data->extentCount = 4;
					newExtentList = (EsFSGlobalExtent *) OSHeapAllocate(extentListMaxSize * sizeof(EsFSGlobalExtent), false);
					CopyMemory(newExtentList, data->indirect, ESFS_INDIRECT_EXTENTS * sizeof(EsFSGlobalExtent));
					newExtentList[data->extentCount++] = newExtent;
					ZeroMemory(data->indirect, ESFS_INDIRECT_EXTENTS * sizeof(EsFSGlobalExtent));
				}
			} break;

			case ESFS_DATA_INDIRECT_2: {
				if (!newExtentList) {
					newExtentList = (EsFSGlobalExtent *) OSHeapAllocate(extentListMaxSize * sizeof(EsFSGlobalExtent), false);

					firstModifiedExtentListBlock = BlocksNeededToStore(data->extentCount * sizeof(EsFSGlobalExtent)) - 1;

					if (!AccessBlock(data->indirect2[firstModifiedExtentListBlock], superblock.blockSize, DRIVE_ACCESS_READ, 
							newExtentList + firstModifiedExtentListBlock * (superblock.blockSize / sizeof(EsFSGlobalExtent)), 0)) {
						return false;
					}
				}

				newExtentList[data->extentCount++] = newExtent;

				if (extentListMaxSize < data->extentCount) {
					// The extent list is too large.
					return false;
				}
			} break;
		}
	}

	if (newExtentList) {
		uint64_t blocksNeeded = BlocksNeededToStore(data->extentCount * sizeof(EsFSGlobalExtent));

		for (uintptr_t i = firstModifiedExtentListBlock; i < blocksNeeded; i++) {
			if (!data->indirect2[i]) {
				EsFSGlobalExtent extent = AllocateExtent(containerBlock / superblock.blocksPerGroup, 1);
				data->indirect2[i] = extent.offset;
				if (!extent.count) return false;
			}

			if (!AccessBlock(data->indirect2[i], superblock.blockSize, DRIVE_ACCESS_WRITE, newExtentList + i * (superblock.blockSize / sizeof(EsFSGlobalExtent)), 0)) {
				return false;
			}
		}
	}

	if (wasDirect && oldSize) {
		// Copy the direct data into the new blocks.
		if (!AccessStream(data, 0, oldSize, directTemporary, true)) {
			return false;
		}
	}

	return true;
}

Node *EsFSVolume::SearchDirectory(char *searchName, size_t nameLength, Node *_directory, uint64_t &flags) {
	_directory->mutex.Acquire();
	Defer(_directory->mutex.Release());

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
		EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) FindAttribute(ESFS_ATTRIBUTE_FILE_DIRECTORY, returnValue + 1);
		if (!data) return nullptr; 
		if (!directory && returnValue->fileType == ESFS_FILE_TYPE_DIRECTORY) return nullptr; 

		OSNodeType type;

		switch (returnValue->fileType) {
			case ESFS_FILE_TYPE_FILE: type = OS_NODE_FILE; break;
			case ESFS_FILE_TYPE_DIRECTORY: type = OS_NODE_DIRECTORY; break;
		}

		Node *node;

		// If the file is already open, return that file.
		if ((node = vfs.FindOpenNode(returnValue->identifier, _directory->filesystem))) {
			return vfs.RegisterNodeHandle(node, flags, returnValue->identifier, _directory, type);
		}

		void *nodeAlloc = OSHeapAllocate(sizeof(Node) + sizeof(EsFSFile) + fileEntryLength, true);
		node = vfs.RegisterNodeHandle(nodeAlloc, flags, returnValue->identifier, _directory, type);

		if (!node) {
			OSHeapFree(nodeAlloc);
			return nullptr;
		}

		EsFSFile *eFile = (EsFSFile *) (node + 1);
		EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);

		eFile->fileEntryLength = fileEntryLength;
		CopyMemory(fileEntry, returnValue, fileEntryLength);

		node->data.type = type;

		switch (fileEntry->fileType) {
			case ESFS_FILE_TYPE_FILE: {
				node->data.file.fileSize = data->size;
			} break;

			case ESFS_FILE_TYPE_DIRECTORY: {
				node->data.directory.entryCount = directory->itemsInDirectory;
			} break;
		}

		CopyMemory(&node->identifier, &fileEntry->identifier, sizeof(UniqueIdentifier));

		eFile->containerBlock = lastAccessedActualBlock;
		eFile->offsetIntoBlock = (uintptr_t) returnValue - (uintptr_t) directoryBuffer;

		return node;
	}
}

inline Node *EsFSScan(char *name, size_t nameLength, Node *directory, uint64_t &flags) {
	EsFSVolume *fs = (EsFSVolume *) directory->filesystem->data;

	// Mutex is acquired in SearchDirectory.
	// 	- TODO Move this out into the VFS code.

	Node *node = fs->SearchDirectory(name, nameLength, directory, flags);
	return node;
}

inline bool EsFSRead(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, Node *file) {
	EsFSVolume *fs = (EsFSVolume *) file->filesystem->data;
	EsFSFile *eFile = (EsFSFile *) (file + 1);
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) fs->FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);
	return fs->AccessStream(data, offsetBytes, sizeBytes, buffer, false);
}

inline bool EsFSWrite(uint64_t offsetBytes, size_t sizeBytes, uint8_t *buffer, Node *file) {
	EsFSVolume *fs = (EsFSVolume *) file->filesystem->data;
	EsFSFile *eFile = (EsFSFile *) (file + 1);
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) fs->FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);
	return fs->AccessStream(data, offsetBytes, sizeBytes, buffer, true);
}

inline void EsFSSync(Node *node) {
	EsFSVolume *fs = (EsFSVolume *) node->filesystem->data;
	EsFSFile *eFile = (EsFSFile *) (node + 1);
	fs->AccessBlock(eFile->containerBlock, eFile->fileEntryLength, DRIVE_ACCESS_WRITE, eFile + 1, eFile->offsetIntoBlock);
}

inline bool EsFSResize(Node *file, uint64_t newSize) {
	EsFSVolume *fs = (EsFSVolume *) file->filesystem->data;
	EsFSFile *eFile = (EsFSFile *) (file + 1);
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) (eFile + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) fs->FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);
	return fs->ResizeDataStream(data, newSize, false, eFile->containerBlock);
}

#endif
