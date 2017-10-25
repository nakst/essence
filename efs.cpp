// TODO list:
// 	-> read/write file commands
// 	-> directory enumeration
// 	-> subdirectories
// 	-> write bootloader
// 	-> port to kernel
// 	-> better testing of things
// 	-> code cleanup!!
// 	(future)
// 	-> sparse files
// 	-> hard/symbolic links
// 	-> journal
// 	-> directory indexing
// 	-> anything else?

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// Volume format:
// 	- Boot block (always 8KiB)		) Core data
// 	- Superblock (always 8Kib)		)
// 	- Group descriptor table		)
// 	- Metadata files			)
// 	- Block groups (block usage extents, data blocks)
// 	- Superblock backup

struct UniqueIdentifier {
	uint8_t d[16];
};

#define MAX_BLOCK_SIZE (16384)

uint8_t entryBuffer[MAX_BLOCK_SIZE];			// Temporary buffers.
uint8_t entryBuffer2[MAX_BLOCK_SIZE];
uint8_t entryBuffer3[MAX_BLOCK_SIZE];
size_t entryBufferPosition;
size_t entryBuffer2Position;
size_t entryBuffer3Position;

// The boot block and superblock are EACH 8KiB.
#define BOOT_SUPER_BLOCK_SIZE (8192)

#define DRIVER_VERSION (1)
#define DRIVE_MINIMUM_SIZE (1048576)

#define MAXIMUM_VOLUME_NAME_LENGTH (32)

#define SIGNATURE_STRING        ("EssenceFS!     \0")
#define SIGNATURE_STRING_LENGTH (16)

struct LocalExtent {
	uint16_t offset;				// The offset in blocks from the first block in the group.
	uint16_t count;					// The number of blocks described by the extent.
};

struct GlobalExtent {
	uint64_t offset;				// The offset in blocks from the first block in the volume.
	uint64_t count;					// The number of blocks described by the extent.
};

#define MAX_TEMP_EXTENTS (16384)
GlobalExtent tempExtents[MAX_TEMP_EXTENTS];		// A temporary list of the extents in a file as they are allocated.
							// This is needed because we don't know how many extents are needed until we've allocated them.

struct Superblock {
	// Version 1

	char signature[SIGNATURE_STRING_LENGTH];	// The filesystem signature; should be SIGNATURE_STRING.
	char volumeName[MAXIMUM_VOLUME_NAME_LENGTH];	// The name of the volume.

	uint16_t requiredReadVersion;			// If this is greater than the driver's version, then the filesystem cannot be read.
	uint16_t requiredWriteVersion;			// If this is greater than the driver's version, then the filesystem cannot be written.

	uint8_t mounted;				// Non-zero to indicate that the volume is mounted, or was not properly unmounted.

	uint64_t blockSize;				// The size of a block on the volume.
	uint64_t blockCount;				// The number of blocks on the volume.
	uint64_t blocksUsed;				// The number of blocks that are in use.

	uint16_t blocksPerGroup;			// The number of blocks in a group.
	uint64_t groupCount;				// The number of groups on the volume.

	LocalExtent gdt;				// The group descriptor table's location.
	LocalExtent rootDirectoryFileEntry;		// The file entry for the root directory.

	UniqueIdentifier identifier;			// The unique identifier for the volume.
	UniqueIdentifier osInstallation;		// The unique identifier of the Essence installation this volume was made for.
							// If this is zero, then the volume is not an Essence installation volume.
};

struct GroupDescriptor {
	uint64_t extentTable;				// The first block containing the extent table.
							// This is usually at the start of the group.
							// If this is 0 then there is no extent table, and no blocks in the group are used.
	uint16_t extentCount;				// The number of extents in the extent table of the group.
							
	uint16_t blocksUsed;				// The number of used blocks in this group.

	// TODO Maybe store the size of the largest extent in the group here?
};

struct AttributeHeader {
#define ATTRIBUTE_LIST_END (0xFFFF)			// Used to mark the end of an attribute list.

#define ATTRIBUTE_FILE_SECURITY (1)			// Contains the security information relating to the item.
#define ATTRIBUTE_FILE_DATA (2)				// Contains a data stream of the file.
#define ATTRIBUTE_FILE_DIRECTORY (3)			// Contains information about a directory file.

#define ATTRIBUTE_DIRECTORY_NAME (1)			// Contains the name of the file in the directory.
#define ATTRIBUTE_DIRECTORY_FILE (2)			// Contains the file entry embedded in the directory.
							// TODO Hard links.

	uint16_t type;
	uint16_t size;
};

struct AttributeFileSecurity {
	// If this attribute does not exist, the file inherits its permissions from its parent.

	AttributeHeader header;

	UniqueIdentifier owner;				// The identifier of the account/program that owns this file/directory.
							// The identifier is only valid if this drive is running on the specified installation.
							// If this field is 0, then there is no assigned owner.
};

struct AttributeFileData {
	AttributeHeader header;

#define STREAM_DEFAULT  (0) // The normal data stream for a file.
	uint8_t stream;					// The stream this data describes.

#define DATA_DIRECT     (0) // size bytes of data follow.
#define DATA_INDIRECT   (1) // extentCount extents follow, which describe where the data is.
#define DATA_INDIRECT_2 (2) // extentCount extents follow, which describe lists of extents, which describe where the data is.
#define DATA_NONE	(3) // Every byte in the file is 0.
	uint8_t indirection;				// The level of indirection needed to read the data.

	uint16_t extentCount;				// The number of extents describing the file.

	uint64_t size;					// The size of the data in the stream in bytes.

	uint64_t sizeBlocks;				// The number of blocks allocated to store the file data.

	union {
		// TODO Change the size of this depending on the block size?
#define DIRECT_BYTES (64)
		char direct[DIRECT_BYTES];
#define INDIRECT_EXTENTS (4)
		GlobalExtent indirect[INDIRECT_EXTENTS];
	};
};

struct AttributeFileDirectory {
	AttributeHeader header;

	uint64_t itemsInDirectory;			// The number of items that the directory currently holds.
	uint16_t spaceAvailableInLastBlock;		// Since directory entries must not span over a block boundary,
							// we should keep track of whether we need to resize the directory
							// to insert a new directory entry.
};

struct AttributeDirectoryName {
	AttributeHeader header;

	uint8_t nameLength;
	// Name follows.
};

struct FileEntry {
	// The size of a file entry must be less than the size of a block.
	// File entries may not span over a block boundary.

#define FILE_ENTRY_SIGNATURE "FileEsFS"
	char signature[8];				// Must be FILE_ENTRY_SIGNATURE.

	UniqueIdentifier identifier;

#define FILE_TYPE_FILE (1)
#define FILE_TYPE_DIRECTORY (2)
#define FILE_TYPE_SYMBOLIC_LINK (3)
	uint8_t fileType;

	uint64_t creationTime;				// TODO Decide the format for times?
	uint64_t modificationTime;

	// Attributes follow.
};

struct AttributeDirectoryFile {
	AttributeHeader header;
	
	// FileEntry follows.
};

struct DirectoryEntry {
#define DIRECTORY_ENTRY_SIGNATURE "DirEntry"
	char signature[8];				// Must be DIRECTORY_ENTRY_SIGNATURE.

	// Attributes follow.
};

struct SuperblockP {
	Superblock d;
	char p[BOOT_SUPER_BLOCK_SIZE - sizeof(Superblock)];	// The P means padding.
};

struct GroupDescriptorP {
	GroupDescriptor d;
	char p[32 - sizeof(GroupDescriptor)];
};

// GLOBAL VARIABLES

FILE *randomFile;
FILE *drive;

uint64_t blockSize;
SuperblockP superblockP;
Superblock *superblock = &superblockP.d;
GroupDescriptorP *groupDescriptorTable;

// FUNCTIONS

void EntryBufferReset() {
	memset(entryBuffer, 0, blockSize);
	entryBufferPosition = 0;
}

void GenerateUniqueIdentifier(UniqueIdentifier &u) {
	fread(&u, 1, sizeof(UniqueIdentifier), randomFile);
}

uint64_t ParseSizeString(char *string) {
	uint64_t size = 0;
	size_t stringLength = strlen(string);

	for (uintptr_t i = 0; i < stringLength; i++) {
		if (string[i] >= '0' && string[i] <= '9') {
			size = (size * 10) + string[i] - '0';
		} else if (string[i] == 'k') {
			size *= 1024;
		} else if (string[i] == 'm') {
			size *= 1024 * 1024;
		} else if (string[i] == 'g') {
			size *= 1024 * 1024 * 1024;
		} else if (string[i] == 't') {
			size *= (uint64_t) 1024 * (uint64_t) 1024 * (uint64_t) 1024 * (uint64_t) 1024;
		}
	}

	return size;
}

void ReadBlock(uintptr_t block, uintptr_t count, void *buffer) {
	fseek(drive, block * blockSize, SEEK_SET);
	fread(buffer, 1, blockSize * count, drive);
}

void WriteBlock(uintptr_t block, uintptr_t count, void *buffer) {
	fseek(drive, block * blockSize, SEEK_SET);

	if (fwrite(buffer, 1, blockSize * count, drive) != blockSize * count) {
		printf("Error: Could not write to blocks %d->%d of drive.\n", block, block + count);
		exit(1);
	}

	printf("Wrote %d block%s to index %d.\n", count, count == 1 ? "" : "s", block);
#if 0
	for (uintptr_t i = 0; i < count * blockSize / 32; i++) {
		for (uintptr_t j = 0; j < 32; j++) {
			printf("%.2X ", ((uint8_t *) buffer)[i * 32 + j]);
		}

		printf("\n");
	}
#endif
}

void MountVolume() {
	printf("Mounting volume...\n");

	// Read the superblock.
	blockSize = BOOT_SUPER_BLOCK_SIZE;
	ReadBlock(1, 1, &superblockP);
	superblock->mounted = 1;
	WriteBlock(1, 1, &superblockP); // Save that we've mounted the volume.
	blockSize = superblock->blockSize;

	// TODO Check all the fields for validity.

	printf("Volume ID: ");
	for (int i = 0; i < 16; i++) printf("%.2X%c", superblock->identifier.d[i], i == 15 ? '\n' : '-');

	// Read the group descriptor table.
	groupDescriptorTable = (GroupDescriptorP *) malloc(superblock->gdt.count * blockSize);
	ReadBlock(superblock->gdt.offset, superblock->gdt.count, groupDescriptorTable);
}

void UnmountVolume() {
	printf("Unmounting volume...\n");
	WriteBlock(superblock->gdt.offset, superblock->gdt.count, groupDescriptorTable);
	blockSize = BOOT_SUPER_BLOCK_SIZE;
	superblock->mounted = 0;
	WriteBlock(1, 1, &superblockP); 
	free(groupDescriptorTable);
}

uint64_t BlocksNeededToStore(uint64_t size) {
	uint64_t blocks = size / blockSize;

	if (size % blockSize) {
		blocks++;
	}

	return blocks;
}

void PrepareCoreData(size_t driveSize, char *volumeName) {
	// Select the block size based on the size of the drive.
	if (driveSize < 512 * 1024 * 1024) {
		blockSize = 512;
	} else if (driveSize < 1024 * 1024 * 1024) {
		blockSize = 1024;
	} else if (driveSize < 2048l * 1024 * 1024) {
		blockSize = 2048;
	} else if (driveSize < 256l * 1024 * 1024 * 1024) {
		blockSize = 4096;
	} else if (driveSize < 256l * 1024 * 1024 * 1024 * 1024) {
		blockSize = 8192;
	} else {
		blockSize = MAX_BLOCK_SIZE;
	}

	SuperblockP superblockP = {};
	Superblock *superblock = &superblockP.d;

	memcpy(superblock->signature, SIGNATURE_STRING, SIGNATURE_STRING_LENGTH);
	memcpy(superblock->volumeName, volumeName, strlen(volumeName));

	// This is the first version!
	superblock->requiredWriteVersion = DRIVER_VERSION;
	superblock->requiredReadVersion = DRIVER_VERSION;

	superblock->blockSize = blockSize;
	superblock->blockCount = driveSize / superblock->blockSize;

	// Have at most 4096 blocks per group.
	superblock->blocksPerGroup = 4096;
	while (true) {
		superblock->groupCount = superblock->blockCount / superblock->blocksPerGroup;
		if (superblock->groupCount) break;
		superblock->blocksPerGroup /= 2;
	}

	printf("Block size: %d\n", superblock->blockSize);
	printf("Block groups: %d\n", superblock->groupCount);
	printf("Blocks per group: %d\n", superblock->blocksPerGroup);

	uint64_t blocksInGDT = BlocksNeededToStore(superblock->groupCount * sizeof(GroupDescriptorP));
	superblock->gdt.count = blocksInGDT;

	printf("Blocks in BGDT: %d\n", blocksInGDT);

	uint64_t bootSuperBlocks = (2 * BOOT_SUPER_BLOCK_SIZE) / blockSize;
	superblock->gdt.offset = bootSuperBlocks;

	uint64_t initialBlockUsage = bootSuperBlocks	// Boot block and superblock.
				   + blocksInGDT 	// Block group descriptor table.
				   + 1			// Metadata files.
				   + 1;			// First group extent table.

	printf("Blocks used: %d\n", initialBlockUsage);

	// Subtract the number of blocks in the superblock again so that those blocks at the end of the last group are not used.
	// This is because they are needed to store a backup of the super block.
	superblock->blockCount -= bootSuperBlocks / 2;
	printf("Block count: %d\n", superblock->blockCount);

	superblock->blocksUsed = initialBlockUsage; 

	// Make sure that the core data is contained in the first group.
	if (initialBlockUsage >= superblock->blocksPerGroup) {
		printf("Error: Could not fit core data (%d blocks) in first group.\n", initialBlockUsage);
		exit(1);
	}

	GroupDescriptorP *descriptorTable = (GroupDescriptorP *) malloc(blocksInGDT * blockSize);
	memset(descriptorTable, 0, blocksInGDT * blockSize);

	GroupDescriptor *firstGroup = &descriptorTable[0].d;
	firstGroup->extentTable = initialBlockUsage - 1; // Use the first block after the core data for the extent table of the first group.
	firstGroup->extentCount = 1;			// 1 extent containing all the unused blocks in the group.
	firstGroup->blocksUsed = initialBlockUsage;

	printf("First group extent table: %d\n", firstGroup->extentTable);

	// The extent table for the first group.
	uint8_t _extent[blockSize] = {};
	LocalExtent *extent = (LocalExtent *) &_extent;
	extent->offset = initialBlockUsage;
	extent->count = superblock->blocksPerGroup - initialBlockUsage;

	printf("First unused extent offset: %d\n", extent->offset);
	printf("First unused extent count: %d\n", extent->count);

	GenerateUniqueIdentifier(superblock->identifier);

	printf("Volume ID: ");
	for (int i = 0; i < 16; i++) printf("%.2X%c", superblock->identifier.d[i], i == 15 ? '\n' : '-');

	// Metadata files.
	superblock->rootDirectoryFileEntry.offset = bootSuperBlocks + blocksInGDT;
	superblock->rootDirectoryFileEntry.count = 1;

	// Don't overwrite the boot block.
	WriteBlock(bootSuperBlocks / 2, bootSuperBlocks / 2, superblock);
	WriteBlock(superblock->blockCount, bootSuperBlocks / 2, superblock);
	WriteBlock(bootSuperBlocks, blocksInGDT, descriptorTable);
	WriteBlock(firstGroup->extentTable, 1, &_extent);

	free(descriptorTable);
}

void FormatVolume(size_t driveSize, char *volumeName) {
	// Setup the superblock, group table and the first group.
	PrepareCoreData(driveSize, volumeName);

	// Mount the volume.
	MountVolume();

	// Create the file entry for the root directory.
	{
		EntryBufferReset();

		FileEntry *entry = (FileEntry *) (entryBuffer + entryBufferPosition);
		GenerateUniqueIdentifier(entry->identifier);
		entry->fileType = FILE_TYPE_DIRECTORY;
		memcpy(&entry->signature, FILE_ENTRY_SIGNATURE, strlen(FILE_ENTRY_SIGNATURE));
		entryBufferPosition += sizeof(FileEntry);

		AttributeFileSecurity *security = (AttributeFileSecurity *) (entryBuffer + entryBufferPosition);
		security->header.type = ATTRIBUTE_FILE_SECURITY;
		security->header.size = sizeof(AttributeFileSecurity);
		entryBufferPosition += security->header.size;

		AttributeFileData *data = (AttributeFileData *) (entryBuffer + entryBufferPosition);
		data->header.type = ATTRIBUTE_FILE_DATA;
		data->header.size = sizeof(AttributeFileData) + 0 /*Directory is empty*/;
		data->stream = STREAM_DEFAULT;
		data->indirection = DATA_DIRECT;
		entryBufferPosition += data->header.size;

		AttributeFileDirectory *directory = (AttributeFileDirectory *) (entryBuffer + entryBufferPosition);
		directory->header.type = ATTRIBUTE_FILE_DIRECTORY;
		directory->header.size = sizeof(AttributeFileDirectory);
		directory->itemsInDirectory = 0;
		directory->spaceAvailableInLastBlock = 0;
		entryBufferPosition += directory->header.size;

		AttributeHeader *end = (AttributeHeader *) (entryBuffer + entryBufferPosition);
		end->type = ATTRIBUTE_LIST_END;
		end->size = sizeof(AttributeHeader);
		entryBufferPosition += end->size;

		if (entryBufferPosition > blockSize) {
			printf("Error: File entry for root directory exceeds block size.\n");
			exit(1);
		}

		WriteBlock(superblock->rootDirectoryFileEntry.offset, 1, entryBuffer);
	}

	// Unmount the volume.
	UnmountVolume();
}

AttributeHeader *FindAttribute(uint16_t attribute, void *_attributeList) {
	uint8_t *attributeList = (uint8_t *) _attributeList;
	AttributeHeader *header = (AttributeHeader *) attributeList;

	while (header->type != ATTRIBUTE_LIST_END) {
		if (header->type == attribute) {
			return header;
		} else {
			header = (AttributeHeader *) (attributeList += header->size);
		}
	}

	return nullptr; // The list did not have the desired attribute
}

GlobalExtent AllocateExtent(uint64_t localGroup, uint64_t desiredBlocks) {
	// TODO Optimise this function.
	// 	- Reduce number of extents.
	// 	- Maximise data locality.
	// 	- Cache extent tables.

	uint64_t groupsSearched = 0;

	for (uint64_t blockGroup = localGroup; groupsSearched < superblock->groupCount; blockGroup = (blockGroup + 1) % superblock->groupCount, groupsSearched++) {
		GroupDescriptor *descriptor = &groupDescriptorTable[blockGroup].d;

		if (descriptor->blocksUsed == superblock->blocksPerGroup) {
			continue;
		}

		if (descriptor->extentCount * sizeof(LocalExtent) > MAX_BLOCK_SIZE) {
			printf("Error: Extent table larger than expected.\n");
			exit(1);
		}

		ReadBlock(descriptor->extentTable, BlocksNeededToStore(descriptor->extentCount * sizeof(LocalExtent)), entryBuffer3);

		uint16_t largestSeenIndex = 0;
		LocalExtent *extentTable = (LocalExtent *) entryBuffer3;
		GlobalExtent extent;

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

		extent.offset += blockGroup * superblock->blocksPerGroup;
		descriptor->blocksUsed += extent.count;
		superblock->blocksUsed += extent.count;

		WriteBlock(descriptor->extentTable, BlocksNeededToStore(descriptor->extentCount * sizeof(LocalExtent)), entryBuffer3);

		return extent;
	}

	// If we get here then the disk is full!
	printf("Error: Disk full!\n");
	exit(1);
}

void ResizeDataStream(AttributeFileData *data, uint64_t newSize) {
	uint64_t oldSize = data->size;
	data->size = newSize;

	if (newSize == oldSize) {
		printf("Size has not changed.\n");
		return;
	} else if (newSize < oldSize) {
		printf("Error: Shrinking files not yet supported.\n");
		exit(1);
	} else if (oldSize == 0) {
		if (newSize <= DIRECT_BYTES) {
			data->indirection = DATA_DIRECT;
			data->extentCount = 0;
			data->sizeBlocks = 0;
		} else {
			uint64_t blocksNeeded = BlocksNeededToStore(newSize);
			data->sizeBlocks = blocksNeeded;
			uint64_t extentCount = 0;

			while (blocksNeeded) {
				if (extentCount == MAX_TEMP_EXTENTS) {
					printf("Error: Too many extents in a file.\n");
					exit(1);
				}

				tempExtents[extentCount] = AllocateExtent(0 /*TODO*/, blocksNeeded);
				blocksNeeded -= tempExtents[extentCount].count;

				extentCount++;
			}

			data->extentCount = extentCount;

			if (extentCount <= INDIRECT_EXTENTS) {
				data->indirection = DATA_INDIRECT;
				memcpy(data->indirect, tempExtents, sizeof(GlobalExtent) * extentCount);
			} else {
				data->indirection = DATA_INDIRECT_2;

				uint64_t blocksNeededForExtentList = BlocksNeededToStore(sizeof(GlobalExtent) * extentCount);
				uint64_t indirectExtentCount = 0;
				uint64_t extentsWritten = 0;

				while (blocksNeededForExtentList) {
					if (indirectExtentCount == INDIRECT_EXTENTS) {
						printf("Error: Too many extents in a file (indirect 2).\n");
						exit(1);
					}

					data->indirect[indirectExtentCount] = AllocateExtent(0 /*TODO*/, blocksNeededForExtentList);
					blocksNeededForExtentList -= data->indirect[indirectExtentCount].count;

					uint64_t extentsStorableInThisExtent = data->indirect[indirectExtentCount].count / sizeof(GlobalExtent);
					WriteBlock(data->indirect[indirectExtentCount].offset,
							data->indirect[indirectExtentCount].count,
							(GlobalExtent *) tempExtents + extentsWritten);
					extentsWritten += extentsStorableInThisExtent;

					indirectExtentCount++;
				}

				if (extentsWritten != extentCount) {
					printf("Error: Something went wrong :(\n");
					exit(1);
				}
			}
		}
	} else {
		printf("Error: Growing files not yet supported.\n");
		exit(1);
	}
}

void AccessStream(AttributeFileData *data, uint64_t offset, uint64_t size, void *_buffer, bool write) {
	if (data->indirection == DATA_DIRECT) {
		memcpy(data->direct + offset, _buffer, size);
		return;
	}

	// TODO Access multiple blocks at a time.
	// TODO Optimise how the extents are calculated.

	uint64_t offsetBlockAligned = offset & ~(blockSize - 1);
	uint64_t sizeBlocks = BlocksNeededToStore((size + (offset - offsetBlockAligned)));

	uint8_t *buffer = (uint8_t *) _buffer;
	
	GlobalExtent *i2ExtentLists[INDIRECT_EXTENTS] = {};

	if (data->indirection == DATA_INDIRECT_2) {
		for (int i = 0; i < INDIRECT_EXTENTS; i++) {
			if (data->indirect[i].offset) {
				i2ExtentLists[i] = (GlobalExtent *) malloc(data->indirect[i].count * blockSize);
				ReadBlock(data->indirect[i].offset, data->indirect[i].count, i2ExtentLists[i]);
			}
		}
	}

	for (uint64_t i = 0; i < sizeBlocks; i++) {
		// Work out which block contains this data.

		uint64_t blockInStream = offsetBlockAligned / blockSize + i;
		uint64_t globalBlock = 0;

		if (data->indirection == DATA_INDIRECT) {
			uint64_t p = 0;

			for (int i = 0; i < INDIRECT_EXTENTS; i++) {
				if (blockInStream < p + data->indirect[i].count) {
					globalBlock = data->indirect[i].offset + blockInStream - p;
					break;
				} else {
					p += data->indirect[i].count;
				}
			}
		} else if (data->indirection == DATA_INDIRECT_2) {
			uint64_t p = 0;

			for (int i = 0; i < INDIRECT_EXTENTS; i++) {
				for (int j = 0; j < data->indirect[i].count * blockSize / sizeof(GlobalExtent); j++) {
					if (blockInStream < p + i2ExtentLists[i][j].count) {
						globalBlock = i2ExtentLists[i][j].offset + blockInStream - p;
						break;
					} else {
						p += i2ExtentLists[i][j].count;
					}
				}
			}
		}

		// Access the modified data.

		uint64_t offsetIntoBlock = 0;
		uint64_t dataToTransfer = blockSize;

		if (i == 0){
			offsetIntoBlock = offset - offsetBlockAligned;
		} 
		
		if (i == sizeBlocks - 1) {
			dataToTransfer = size;
		}

		uint8_t blockBuffer[blockSize];

		if (write) {
			if (offsetIntoBlock || dataToTransfer != blockSize) ReadBlock(globalBlock, 1, blockBuffer);
			memcpy(blockBuffer + offsetIntoBlock, buffer, dataToTransfer);
			WriteBlock(globalBlock, 1, blockBuffer);
		} else {
			ReadBlock(globalBlock, 1, blockBuffer);
			memcpy(buffer, blockBuffer + offsetIntoBlock, dataToTransfer);
		}

		buffer += dataToTransfer;
		size -= dataToTransfer;
	}

	for (int i = 0; i < INDIRECT_EXTENTS; i++) {
		free(i2ExtentLists[i]);
	}
}

void Tree() {
	printf("--> /\n");

	// Load the root directory.
	FileEntry *fileEntry = (FileEntry *) malloc(blockSize);
	ReadBlock(superblock->rootDirectoryFileEntry.offset,
			superblock->rootDirectoryFileEntry.count,
			fileEntry);

	AttributeFileDirectory *directory = (AttributeFileDirectory *) FindAttribute(ATTRIBUTE_FILE_DIRECTORY, fileEntry + 1);
	AttributeFileData *data = (AttributeFileData *) FindAttribute(ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (!directory) {
		printf("Error: Directory did not have a directory attribute.\n");
		exit(1);
	}

	if (!data) {
		printf("Error: Directory did not have a data attribute.\n");
		exit(1);
	}

	uint8_t *directoryBuffer = (uint8_t *) malloc(data->size);
	uint8_t *directoryBufferPosition = directoryBuffer;
	AccessStream(data, 0, data->size, directoryBuffer, false);

	for (uint64_t i = 0; i < directory->itemsInDirectory; i++) {
		DirectoryEntry *entry = (DirectoryEntry *) directoryBufferPosition;

		if (memcmp(entry->signature, DIRECTORY_ENTRY_SIGNATURE, strlen(DIRECTORY_ENTRY_SIGNATURE))) {
			printf("Error: Directory entry had invalid signature.\n");
			exit(1);
		}

		directoryBufferPosition += sizeof(DirectoryEntry);

		AttributeHeader *attribute = (AttributeHeader *) directoryBufferPosition;
		while (attribute->type != ATTRIBUTE_LIST_END) {
			attribute = (AttributeHeader *) directoryBufferPosition;

			switch (attribute->type) {
				case ATTRIBUTE_DIRECTORY_NAME: {
					AttributeDirectoryName *name = (AttributeDirectoryName *) attribute;
					printf("    %.*s ", name->nameLength, name + 1);
					for (int i = 0; i < 20 - name->nameLength; i++) printf(" ");
				} break;

				case ATTRIBUTE_DIRECTORY_FILE: {
					AttributeDirectoryFile *file = (AttributeDirectoryFile *) attribute;
					FileEntry *entry = (FileEntry *) (file + 1);

					if (memcmp(entry->signature, FILE_ENTRY_SIGNATURE, strlen(FILE_ENTRY_SIGNATURE))) {
						printf("Error: File entry had invalid signature.\n");
						exit(1);
					}

					for (int i = 0; i < 16; i++) printf("%.2X%c", entry->identifier.d[i], i == 15 ? ' ' : '-');
					printf("  ");

					printf("%s ", entry->fileType == FILE_TYPE_FILE ? "file" : 
							(entry->fileType == FILE_TYPE_DIRECTORY ? "directory" : 
								(entry->fileType == FILE_TYPE_SYMBOLIC_LINK ? "symbolic_link" : "unrecognised")));
					printf("  ");

					{
						uint8_t *attributePosition = (uint8_t *) (entry + 1);
						AttributeHeader *attribute = (AttributeHeader *) attributePosition;

						while (attribute->type != ATTRIBUTE_LIST_END) {
							attribute = (AttributeHeader *) attributePosition;

							switch (attribute->type) {
								case ATTRIBUTE_FILE_DATA: {
									printf("%d bytes", ((AttributeFileData *) attribute)->size);
								} break;
							}

							attributePosition += attribute->size;
						}
					}
				} break;

				case ATTRIBUTE_LIST_END: break;

				default: {
					// Unrecognised attribute.
				} break;
			}

			directoryBufferPosition += attribute->size;
		}

		printf("\n");
	}

	if (directory->itemsInDirectory) {
		printf("    (%d item%s)\n", directory->itemsInDirectory, directory->itemsInDirectory > 1 ? "s" : "");
	} else {
		printf("    (empty directory)\n");
	}

	free(directoryBuffer);
	free(fileEntry);
}

void AddFile(char *path, char *name, uint64_t size) {
	if (strcmp(path, "/")) {
		printf("Error: The root directory is currently only supported.\n");
		exit(1);
	}

	if (strlen(name) >= 256) {
		printf("Error: The filename is too long. It can be at maximum 255 bytes.\n");
		exit(1);
	}

	{
		EntryBufferReset();

		// Step 1: Make the new file entry.

		// TODO Cleanup copy/paste stuffs.
		FileEntry *entry = (FileEntry *) (entryBuffer + entryBufferPosition);
		GenerateUniqueIdentifier(entry->identifier);
		entry->fileType = FILE_TYPE_FILE;
		memcpy(&entry->signature, FILE_ENTRY_SIGNATURE, strlen(FILE_ENTRY_SIGNATURE));
		entryBufferPosition += sizeof(FileEntry);

		AttributeFileData *data = (AttributeFileData *) (entryBuffer + entryBufferPosition);
		data->header.type = ATTRIBUTE_FILE_DATA;
		data->header.size = sizeof(AttributeFileData) + 0 /*Empty file*/;
		data->stream = STREAM_DEFAULT;
		data->indirection = DATA_NONE;
		data->size = size;
		entryBufferPosition += data->header.size;

		AttributeHeader *end = (AttributeHeader *) (entryBuffer + entryBufferPosition);
		end->type = ATTRIBUTE_LIST_END;
		end->size = sizeof(AttributeHeader);
		entryBufferPosition += end->size;

		// Move this to the second entry buffer.
		memcpy(entryBuffer2, entryBuffer, MAX_BLOCK_SIZE);
		entryBuffer2Position = entryBufferPosition;
	}

	{
		EntryBufferReset();

		// Step 2: Make the new directory entry.
		DirectoryEntry *entry = (DirectoryEntry *) (entryBuffer + entryBufferPosition);
		memcpy(entry->signature, DIRECTORY_ENTRY_SIGNATURE, strlen(DIRECTORY_ENTRY_SIGNATURE));
		entryBufferPosition += sizeof(DirectoryEntry);

		char *_n = name;
		AttributeDirectoryName *name = (AttributeDirectoryName *) (entryBuffer + entryBufferPosition);
		name->header.type = ATTRIBUTE_DIRECTORY_NAME;
		name->header.size = strlen(_n) + sizeof(AttributeDirectoryName);
		name->nameLength = strlen(_n);
		memcpy(name + 1, _n, strlen(_n));
		entryBufferPosition += name->header.size;

		AttributeDirectoryFile *file = (AttributeDirectoryFile *) (entryBuffer + entryBufferPosition);
		file->header.type = ATTRIBUTE_DIRECTORY_FILE;
		file->header.size = sizeof(AttributeDirectoryFile) + entryBuffer2Position;
		memcpy(file + 1, entryBuffer2, entryBuffer2Position);
		entryBufferPosition += file->header.size;

		AttributeHeader *end = (AttributeHeader *) (entryBuffer + entryBufferPosition);
		end->type = ATTRIBUTE_LIST_END;
		end->size = sizeof(AttributeHeader);
		entryBufferPosition += end->size;

		if (entryBufferPosition > blockSize) {
			printf("Error: Directory entry for new file exceeds block size.\n");
			exit(1);
		}
	}

	FileEntry *fileEntry = (FileEntry *) malloc(blockSize);
	ReadBlock(superblock->rootDirectoryFileEntry.offset,
			superblock->rootDirectoryFileEntry.count,
			fileEntry);
	
	AttributeFileDirectory *directory = (AttributeFileDirectory *) FindAttribute(ATTRIBUTE_FILE_DIRECTORY, fileEntry + 1);
	AttributeFileData *data = (AttributeFileData *) FindAttribute(ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (!directory) {
		printf("Error: Directory did not have a directory attribute.\n");
		exit(1);
	}

	if (!data) {
		printf("Error: Directory did not have a data attribute.\n");
		exit(1);
	}

	printf("spaceAvailableInLastBlock = %d, entryBufferPosition = %d\n", directory->spaceAvailableInLastBlock, entryBufferPosition);

	{
		// Step 3: Store the directory entry.
		if (directory->spaceAvailableInLastBlock >= entryBufferPosition) {
			// There is enough space in the last block.
		} else {
			// We need to add a new block to the file.
			ResizeDataStream(data, data->size + blockSize);
			directory->spaceAvailableInLastBlock = blockSize;
		}

		AccessStream(data, data->size - directory->spaceAvailableInLastBlock, entryBufferPosition, entryBuffer, true);
		directory->spaceAvailableInLastBlock -= entryBufferPosition;

		directory->itemsInDirectory++;
	}

	// Save the file entry.
	WriteBlock(superblock->rootDirectoryFileEntry.offset,
			superblock->rootDirectoryFileEntry.count,
			fileEntry);

	free(fileEntry);
}

int main(int argc, char **argv) {
	randomFile = fopen("/dev/random", "rb");

	if (argc < 3) {
		printf("Usage: <drive> <command> <options>\n");
		exit(1);
	}

	char *driveFilename = argv[1];
	char *command = argv[2];

	argc -= 3;
	argv += 3;

	drive = fopen(driveFilename, "r+b");

	if (!drive) {
		printf("Error: Could not open drive file.\n");
		exit(1);
	}

	fseek(drive, 0, SEEK_SET);

#define IS_COMMAND(_a) 0 == strcmp(command, _a)
#define CHECK_ARGS(_n, _u) if (argc != _n) { printf("Usage: <drive> %s\n", _u); exit(1); }

	if (IS_COMMAND("format")) {
		CHECK_ARGS(2, "format <size> <name>");
		uint64_t driveSize = ParseSizeString(argv[0]);

		if (driveSize < DRIVE_MINIMUM_SIZE) {
			printf("Error: Cannot create a drive of %d bytes (too small).\n", driveSize);
			exit(1);
		}

		if (truncate(driveFilename, driveSize)) {
			printf("Error: Could not change the file's size to %d bytes.\n", driveSize);
			exit(1);
		}

		if (strlen(argv[1]) > MAXIMUM_VOLUME_NAME_LENGTH) {
			printf("Error: Volume name '%s' is too long; must be <= %d bytes.\n", argv[1], MAXIMUM_VOLUME_NAME_LENGTH);
			exit(1);
		}

		FormatVolume(driveSize, argv[1]);
	} else if (IS_COMMAND("tree")) {
		CHECK_ARGS(0, "tree");

		MountVolume();
		Tree();
		UnmountVolume();
	} else if (IS_COMMAND("add-file")) {
		CHECK_ARGS(3, "add-file <path> <name> <size>");

		MountVolume();
		AddFile(argv[0], argv[1], ParseSizeString(argv[2]));
		UnmountVolume();
	} else {
		printf("Unrecognised command '%s'.\n", command);
		exit(1);
	}

	return 0;
}
