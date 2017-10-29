// TODO list:
// 	-> delete files/directories
// 	-> data stream shrinking
// 	-> sparse files
// 	-> hard/symbolic links
// 	-> journal
//	-> checksums
//	-> directory indexing
//	-> check program
//	-> indirect 3
//	-> block error handling

#ifndef KERNEL
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ESFS_HEADER
#define ESFS_IMPLEMENTATION
#define ESFS_UTITILES

struct UniqueIdentifier {
	uint8_t d[16];
};
#endif

#ifdef ESFS_HEADER

// Volume format:
// 	- Boot block (always 8KiB)		) Core data
// 	- Superblock (always 8Kib)		)
// 	- Group descriptor table		)
// 	- Metadata files			)
// 	- Block groups (block usage extents, data blocks)
// 	- Superblock backup

#define ESFS_MAX_BLOCK_SIZE (16384)

// The boot block and superblock are EACH 8KiB.
#define ESFS_BOOT_SUPER_BLOCK_SIZE (8192)

#define ESFS_DRIVER_VERSION (2)
#define ESFS_DRIVE_MINIMUM_SIZE (1048576)

#define ESFS_MAXIMUM_VOLUME_NAME_LENGTH (32)

#define ESFS_SIGNATURE_STRING        ("EssenceFS!     \0")
#define ESFS_SIGNATURE_STRING_LENGTH (16)

struct EsFSLocalExtent {
	uint16_t offset;				// The offset in blocks from the first block in the group.
	uint16_t count;					// The number of blocks described by the extent.
};

struct EsFSGlobalExtent {
	uint64_t offset;				// The offset in blocks from the first block in the volume.
	uint64_t count;					// The number of blocks described by the extent.
};

struct EsFSSuperblock {
	// Version 1

/*0*/	char signature[ESFS_SIGNATURE_STRING_LENGTH];	// The filesystem signature; should be ESFS_SIGNATURE_STRING.
/*16*/	char volumeName[ESFS_MAXIMUM_VOLUME_NAME_LENGTH]; // The name of the volume.
/**/	
/*48*/	uint16_t requiredReadVersion;			// If this is greater than the driver's version, then the filesystem cannot be read.
/*50*/	uint16_t requiredWriteVersion;			// If this is greater than the driver's version, then the filesystem cannot be written.
/**/	
/*52*/	uint8_t mounted;				// Non-zero to indicate that the volume is mounted, or was not properly unmounted.
/**/	
/*56*/	uint64_t blockSize;				// The size of a block on the volume.
/*64*/	uint64_t blockCount;				// The number of blocks on the volume.
/*72*/	uint64_t blocksUsed;				// The number of blocks that are in use.
/**/	
/*80*/	uint16_t blocksPerGroup;			// The number of blocks in a group.
/*88*/	uint64_t groupCount;				// The number of groups on the volume.
/*96*/	uint64_t blocksPerGroupExtentTable;		// The number of blocks used to a store a group's extent table.
/**/	
/*104*/	EsFSLocalExtent gdt;				// The group descriptor table's location.
/*108*/	EsFSLocalExtent rootDirectoryFileEntry;		// The file entry for the root directory.
/*112*/	EsFSLocalExtent kernelFileEntry;		// The file entry for the kernel.
/**/	
/*116*/	UniqueIdentifier identifier;			// The unique identifier for the volume.
/*132*/	UniqueIdentifier osInstallation;		// The unique identifier of the Essence installation this volume was made for.
							// If this is zero, then the volume is not an Essence installation volume.
};

struct EsFSGroupDescriptor {
	uint64_t extentTable;				// The first block containing the extent table.
							// This is usually at the start of the group.
							// If this is 0 then there is no extent table, and no blocks in the group are used.
							// As the worst case for a fully merged table in a group will be blocks per group / 2 and each LocalExtent is 2 bytes,
							// the maximum size of an extent table will be (blocks per group) bytes.
							// This will therefore be the size of the table.

	uint16_t extentCount;				// The number of extents in the extent table of the group.
							
	uint16_t blocksUsed;				// The number of used blocks in this group.

	// TODO Maybe store the size of the largest extent in the group here?
};

struct EsFSAttributeHeader {
#define ESFS_ATTRIBUTE_LIST_END (0xFFFF)		// Used to mark the end of an attribute list.

#define ESFS_ATTRIBUTE_FILE_SECURITY (1)		// Contains the security information relating to the item.
#define ESFS_ATTRIBUTE_FILE_DATA (2)			// Contains a data stream of the file.
#define ESFS_ATTRIBUTE_FILE_DIRECTORY (3)		// Contains information about a directory file.

#define ESFS_ATTRIBUTE_DIRECTORY_NAME (1)		// Contains the name of the file in the directory.
#define ESFS_ATTRIBUTE_DIRECTORY_FILE (2)		// Contains the file entry embedded in the directory.
							// TODO Hard links.

	uint16_t type;
	uint16_t size;
};

struct EsFSAttributeFileSecurity {
	// If this attribute does not exist, the file inherits its permissions from its parent.

	EsFSAttributeHeader header;

	UniqueIdentifier owner;				// The identifier of the account/program that owns this file/directory.
							// The identifier is only valid if this drive is running on the specified installation.
							// If this field is 0, then there is no assigned owner.
};

struct EsFSAttributeFileData {
/*0*/	EsFSAttributeHeader header;

#define ESFS_STREAM_DEFAULT  (0) // The normal data stream for a file.
/*4*/	uint8_t stream;					// The stream this data describes.

#define ESFS_DATA_INDIRECT   (1)
#define ESFS_DATA_INDIRECT_2 (2) 
#define ESFS_DATA_INDIRECT_3 (3)
#define ESFS_DATA_DIRECT     (4)
/*5*/	uint8_t indirection;				// The level of indirection needed to read the data.

/*6*/	uint16_t extentCount;				// The number of extents describing the file.

/*8*/	uint64_t size;					// The size of the data in the stream in bytes.

/*16*/	union {
		// TODO Change the size of this depending on the block size?

#define ESFS_INDIRECT_EXTENTS (4)
		EsFSGlobalExtent indirect[ESFS_INDIRECT_EXTENTS]; // Extents that contain the stream's data.
	
#define ESFS_INDIRECT_2_EXTENTS (8)
		uint64_t indirect2[ESFS_INDIRECT_2_EXTENTS];	// Blocks that contain lists of extents that contain the stream's data.

#define ESFS_DIRECT_BYTES (64)
		uint8_t direct[ESFS_DIRECT_BYTES];		// The actual file data, inline.
	};
};

struct EsFSAttributeFileDirectory {
	EsFSAttributeHeader header;

	uint64_t itemsInDirectory;			// The number of items that the directory currently holds.
	uint16_t spaceAvailableInLastBlock;		// Since directory entries must not span over a block boundary,
							// we should keep track of whether we need to resize the directory
							// to insert a new directory entry.
};

struct EsFSAttributeDirectoryName {
	EsFSAttributeHeader header;

	uint8_t nameLength;
	// Name follows.
};

struct EsFSFileEntry {
	// The size of a file entry must be less than the size of a block.
	// File entries may not span over a block boundary.

#define ESFS_FILE_ENTRY_SIGNATURE "FileEsFS"
/*0*/	char signature[8];				// Must be ESFS_FILE_ENTRY_SIGNATURE.

/*8*/	UniqueIdentifier identifier;

#define ESFS_FILE_TYPE_FILE (1)
#define ESFS_FILE_TYPE_DIRECTORY (2)
#define ESFS_FILE_TYPE_SYMBOLIC_LINK (3)
/*24*/	uint8_t fileType;

/*32*/	uint64_t creationTime;				// TODO Decide the format for times?
/*40*/	uint64_t modificationTime;

/*48*/	// EsFSAttributes follow.
};

struct EsFSAttributeDirectoryFile {
	EsFSAttributeHeader header;
	
	// FileEntry follows.
};

struct EsFSDirectoryEntry {
#define ESFS_DIRECTORY_ENTRY_SIGNATURE "DirEntry"
	char signature[8];				// Must be ESFS_DIRECTORY_ENTRY_SIGNATURE.

	// EsFSAttributes follow.
};

struct EsFSSuperblockP {
	EsFSSuperblock d;
	char p[ESFS_BOOT_SUPER_BLOCK_SIZE - sizeof(EsFSSuperblock)];	// The P means padding.
};

struct EsFSGroupDescriptorP {
	EsFSGroupDescriptor d;
	char p[32 - sizeof(EsFSGroupDescriptor)];
};

struct EsFSLoadInformation {
	uint64_t containerBlock;
	uint64_t positionInBlock;
};

#endif 

#ifdef ESFS_IMPLEMENTATION

// GLOBAL VARIABLES

FILE *randomFile;
FILE *drive;

uint64_t blockSize;
uint64_t partitionOffset;
EsFSSuperblockP superblockP;
EsFSSuperblock *superblock = &superblockP.d;
EsFSGroupDescriptorP *groupDescriptorTable;

uint8_t entryBuffer[ESFS_MAX_BLOCK_SIZE];			// Temporary buffers.
size_t entryBufferPosition;
uint8_t extentTableBuffer[ESFS_MAX_BLOCK_SIZE];

#define ESFS_MAX_TEMP_EXTENTS (16384)
EsFSGlobalExtent tempExtents[ESFS_MAX_TEMP_EXTENTS];		// A temporary list of the extents in a file as they are allocated.
							// This is needed because we don't know how many extents are needed until we've allocated them.

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

void ReadBlock(uint64_t block, uint64_t count, void *buffer) {
	fseek(drive, block * blockSize + partitionOffset * 512, SEEK_SET);
	fread(buffer, 1, blockSize * count, drive);

#if 0
	printf("Read %d block%s from index %d.\n", count, count == 1 ? "" : "s", block);
#endif
}

void WriteBlock(uint64_t block, uint64_t count, void *buffer) {
	fseek(drive, block * blockSize + partitionOffset * 512, SEEK_SET);

	if (fwrite(buffer, 1, blockSize * count, drive) != blockSize * count) {
		printf("Error: Could not write to blocks %d->%d of drive.\n", block, block + count);
		exit(1);
	}

#if 0
	printf("Wrote %d block%s to index %d.\n", count, count == 1 ? "" : "s", block);
	for (uintptr_t i = 0; i < count * blockSize / 32; i++) {
		for (uintptr_t j = 0; j < 32; j++) {
			printf("%.2X ", ((uint8_t *) buffer)[i * 32 + j]);
		}

		printf("\n");
	}
#endif
}

void MountVolume() {
	// printf("Mounting volume...\n");

	// Read the superblock.
	blockSize = ESFS_BOOT_SUPER_BLOCK_SIZE;
	ReadBlock(1, 1, &superblockP);
	superblock->mounted = 1;
	WriteBlock(1, 1, &superblockP); // Save that we've mounted the volume.
	blockSize = superblock->blockSize;

	if (memcmp(superblock->signature, ESFS_SIGNATURE_STRING, ESFS_SIGNATURE_STRING_LENGTH)) {
		printf("Error: Superblock contained invalid signature.\n");
		exit(1);
	}

	if (superblock->requiredReadVersion > ESFS_DRIVER_VERSION) {
		printf("Error: Volume requires later driver version (%d) to read.\n", superblock->requiredReadVersion);
		exit(1);
	}

	if (superblock->requiredWriteVersion > ESFS_DRIVER_VERSION) {
		printf("Error: Volume requires later driver version (%d) to write.\n", superblock->requiredWriteVersion);
		exit(1);
	}

	// printf("Volume ID: ");
	// for (int i = 0; i < 16; i++) printf("%.2X%c", superblock->identifier.d[i], i == 15 ? '\n' : '-');

	// Read the group descriptor table.
	groupDescriptorTable = (EsFSGroupDescriptorP *) malloc(superblock->gdt.count * blockSize);
	ReadBlock(superblock->gdt.offset, superblock->gdt.count, groupDescriptorTable);
}

void UnmountVolume() {
	// printf("Unmounting volume...\n");
	WriteBlock(superblock->gdt.offset, superblock->gdt.count, groupDescriptorTable);
	blockSize = ESFS_BOOT_SUPER_BLOCK_SIZE;
	superblock->mounted = 0;
	WriteBlock(1, 1, &superblockP); 
	free(groupDescriptorTable);
}

void LoadRootDirectory(EsFSFileEntry *fileEntry, EsFSLoadInformation *loadInformation) {
	loadInformation->containerBlock = superblock->rootDirectoryFileEntry.offset;
	loadInformation->positionInBlock = 0;

	ReadBlock(superblock->rootDirectoryFileEntry.offset,
			superblock->rootDirectoryFileEntry.count,
			fileEntry);
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
		blockSize = ESFS_MAX_BLOCK_SIZE;
	}

	EsFSSuperblockP superblockP = {};
	EsFSSuperblock *superblock = &superblockP.d;

	memcpy(superblock->signature, ESFS_SIGNATURE_STRING, ESFS_SIGNATURE_STRING_LENGTH);
	memcpy(superblock->volumeName, volumeName, strlen(volumeName));

	// This is the first version!
	superblock->requiredWriteVersion = ESFS_DRIVER_VERSION;
	superblock->requiredReadVersion = ESFS_DRIVER_VERSION;

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

	superblock->blocksPerGroupExtentTable = BlocksNeededToStore(superblock->blocksPerGroup);

	uint64_t blocksInGDT = BlocksNeededToStore(superblock->groupCount * sizeof(EsFSGroupDescriptorP));
	superblock->gdt.count = blocksInGDT;

	// printf("Blocks in BGDT: %d\n", blocksInGDT);

	uint64_t bootSuperBlocks = (2 * ESFS_BOOT_SUPER_BLOCK_SIZE) / blockSize;
	superblock->gdt.offset = bootSuperBlocks;

	uint64_t initialBlockUsage = bootSuperBlocks	// Boot block and superblock.
				   + blocksInGDT 	// Block group descriptor table.
				   + 2			// Metadata files - currently: the root directory and the kernel.
				   + superblock->blocksPerGroupExtentTable; // First group extent table.

	// printf("Blocks used: %d\n", initialBlockUsage);

	// Subtract the number of blocks in the superblock again so that those blocks at the end of the last group are not used.
	// This is because they are needed to store a backup of the super block.
	superblock->blockCount -= bootSuperBlocks / 2;
	// printf("Block count: %d\n", superblock->blockCount);

	superblock->blocksUsed = initialBlockUsage; 

	// Make sure that the core data is contained in the first group.
	if (initialBlockUsage >= superblock->blocksPerGroup) {
		printf("Error: Could not fit core data (%d blocks) in first group.\n", initialBlockUsage);
		exit(1);
	}

	EsFSGroupDescriptorP *descriptorTable = (EsFSGroupDescriptorP *) malloc(blocksInGDT * blockSize);
	memset(descriptorTable, 0, blocksInGDT * blockSize);

	EsFSGroupDescriptor *firstGroup = &descriptorTable[0].d;
	firstGroup->extentTable = initialBlockUsage - superblock->blocksPerGroupExtentTable; // Use the few block after the core data for the extent table of the first group.
	firstGroup->extentCount = 1;			// 1 extent containing all the unused blocks in the group.
	firstGroup->blocksUsed = initialBlockUsage;

	// printf("First group extent table: %d\n", firstGroup->extentTable);

	// The extent table for the first group.
	uint8_t _extent[blockSize] = {}; // TODO Stack overflow?
	EsFSLocalExtent *extent = (EsFSLocalExtent *) &_extent;
	extent->offset = initialBlockUsage;
	extent->count = superblock->blocksPerGroup - initialBlockUsage;

	// printf("First unused extent offset: %d\n", extent->offset);
	// printf("First unused extent count: %d\n", extent->count);

	GenerateUniqueIdentifier(superblock->identifier);

	printf("Volume ID: ");
	for (int i = 0; i < 16; i++) printf("%.2X%c", superblock->identifier.d[i], i == 15 ? '\n' : '-');

	// Metadata files.
	superblock->rootDirectoryFileEntry.offset = bootSuperBlocks + blocksInGDT;
	superblock->rootDirectoryFileEntry.count = 1;
	superblock->kernelFileEntry.offset = bootSuperBlocks + blocksInGDT + 1;
	superblock->kernelFileEntry.count = 1;

	// Don't overwrite the boot block.
	WriteBlock(bootSuperBlocks / 2, bootSuperBlocks / 2, superblock);
	WriteBlock(superblock->blockCount, bootSuperBlocks / 2, superblock);
	WriteBlock(bootSuperBlocks, blocksInGDT, descriptorTable);
	WriteBlock(firstGroup->extentTable, 1, &_extent);

	free(descriptorTable);
}

void ResizeDataStream(EsFSAttributeFileData *data, uint64_t newSize, bool clearNewBlocks, EsFSLoadInformation *dataLoadInformation);
void AccessStream(EsFSAttributeFileData *data, uint64_t offset, uint64_t size, void *_buffer, bool write, uint64_t *lastAccessedActualBlock = nullptr);

void FormatVolume(size_t driveSize, char *volumeName, void *kernelData, uint64_t kernelDataLength) {
	// Setup the superblock, group table and the first group.
	PrepareCoreData(driveSize, volumeName);

	// Mount the volume.
	MountVolume();

	// Create the file entry for the root directory.
	{
		EntryBufferReset();

		EsFSFileEntry *entry = (EsFSFileEntry *) (entryBuffer + entryBufferPosition);
		GenerateUniqueIdentifier(entry->identifier);
		entry->fileType = ESFS_FILE_TYPE_DIRECTORY;
		memcpy(&entry->signature, ESFS_FILE_ENTRY_SIGNATURE, strlen(ESFS_FILE_ENTRY_SIGNATURE));
		entryBufferPosition += sizeof(EsFSFileEntry);

		EsFSAttributeFileSecurity *security = (EsFSAttributeFileSecurity *) (entryBuffer + entryBufferPosition);
		security->header.type = ESFS_ATTRIBUTE_FILE_SECURITY;
		security->header.size = sizeof(EsFSAttributeFileSecurity);
		entryBufferPosition += security->header.size;

		EsFSAttributeFileData *data = (EsFSAttributeFileData *) (entryBuffer + entryBufferPosition);
		data->header.type = ESFS_ATTRIBUTE_FILE_DATA;
		data->header.size = sizeof(EsFSAttributeFileData);
		data->stream = ESFS_STREAM_DEFAULT;
		data->indirection = ESFS_DATA_DIRECT;
		entryBufferPosition += data->header.size;

		EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) (entryBuffer + entryBufferPosition);
		directory->header.type = ESFS_ATTRIBUTE_FILE_DIRECTORY;
		directory->header.size = sizeof(EsFSAttributeFileDirectory);
		directory->itemsInDirectory = 0;
		directory->spaceAvailableInLastBlock = 0;
		entryBufferPosition += directory->header.size;

		EsFSAttributeHeader *end = (EsFSAttributeHeader *) (entryBuffer + entryBufferPosition);
		end->type = ESFS_ATTRIBUTE_LIST_END;
		end->size = sizeof(EsFSAttributeHeader);
		entryBufferPosition += end->size;

		if (entryBufferPosition > blockSize) {
			printf("Error: File entry for root directory exceeds block size.\n");
			exit(1);
		}

		WriteBlock(superblock->rootDirectoryFileEntry.offset, 1, entryBuffer);
	}

	// Create the file entry for the kernel.
	{
		EntryBufferReset();

		EsFSFileEntry *entry = (EsFSFileEntry *) (entryBuffer + entryBufferPosition);
		GenerateUniqueIdentifier(entry->identifier);
		entry->fileType = ESFS_FILE_TYPE_DIRECTORY;
		memcpy(&entry->signature, ESFS_FILE_ENTRY_SIGNATURE, strlen(ESFS_FILE_ENTRY_SIGNATURE));
		entryBufferPosition += sizeof(EsFSFileEntry);

		EsFSAttributeFileData *data = (EsFSAttributeFileData *) (entryBuffer + entryBufferPosition);
		data->header.type = ESFS_ATTRIBUTE_FILE_DATA;
		data->header.size = sizeof(EsFSAttributeFileData);
		data->stream = ESFS_STREAM_DEFAULT;
		data->indirection = ESFS_DATA_DIRECT;
		entryBufferPosition += data->header.size;
		EsFSLoadInformation dataLoadInformation;
		dataLoadInformation.containerBlock = superblock->kernelFileEntry.offset;
		dataLoadInformation.positionInBlock = 0;

		EsFSAttributeHeader *end = (EsFSAttributeHeader *) (entryBuffer + entryBufferPosition);
		end->type = ESFS_ATTRIBUTE_LIST_END;
		end->size = sizeof(EsFSAttributeHeader);
		entryBufferPosition += end->size;

		if (entryBufferPosition > 512) {
			printf("Error: File entry for kernel exceeds 512 bytes.\n");
			exit(1);
		}

		ResizeDataStream(data, kernelDataLength, true, &dataLoadInformation);
		AccessStream(data, 0, kernelDataLength, kernelData, true);
		WriteBlock(superblock->kernelFileEntry.offset, 1, entryBuffer);
	}

	// Unmount the volume.
	UnmountVolume();
}

EsFSAttributeHeader *FindAttribute(uint16_t attribute, void *_attributeList) {
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

uint16_t GetBlocksInGroup(uint64_t group) {
	if (group == superblock->groupCount - 1) {
		return superblock->blockCount % superblock->blocksPerGroup;
	} else {
		return superblock->blocksPerGroup;
	}
}

EsFSGlobalExtent AllocateExtent(uint64_t localGroup, uint64_t desiredBlocks) {
	// TODO Optimise this function.
	// 	- Cache extent tables.

	uint64_t groupsSearched = 0;

	// printf("Allocating extent for %d blocks....\n", desiredBlocks);

	for (uint64_t blockGroup = localGroup; groupsSearched < superblock->groupCount; blockGroup = (blockGroup + 1) % superblock->groupCount, groupsSearched++) {
		EsFSGroupDescriptor *descriptor = &groupDescriptorTable[blockGroup].d;

		if (descriptor->blocksUsed == GetBlocksInGroup(blockGroup)) {
			continue;
		} 

		if (descriptor->extentCount * sizeof(EsFSLocalExtent) > ESFS_MAX_BLOCK_SIZE) {
			// This shouldn't happen as long as the number of blocks in a group does not exceed ESFS_MAX_BLOCK_SIZE.
			printf("Error: Extent table larger than expected.\n");
			exit(1);
		}

		if (!descriptor->extentTable) {
			// The group does not have an extent table allocated for it yet, so let's make it.
			descriptor->extentTable = blockGroup * superblock->blocksPerGroup;
			descriptor->extentCount = 1;
			descriptor->blocksUsed = superblock->blocksPerGroupExtentTable;

			EsFSLocalExtent *extent = (EsFSLocalExtent *) extentTableBuffer;
			extent->offset = superblock->blocksPerGroupExtentTable; 
			extent->count = GetBlocksInGroup(blockGroup) - superblock->blocksPerGroupExtentTable;

			// The table is saved at the end of the function.
		} else {
			ReadBlock(descriptor->extentTable, BlocksNeededToStore(descriptor->extentCount * sizeof(EsFSLocalExtent)), extentTableBuffer);
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

		extent.offset += blockGroup * superblock->blocksPerGroup;
		descriptor->blocksUsed += extent.count;
		superblock->blocksUsed += extent.count;

		WriteBlock(descriptor->extentTable, BlocksNeededToStore(descriptor->extentCount * sizeof(EsFSLocalExtent)), extentTableBuffer);

		// printf("Allocated extent: %d -> %d\n", extent.offset, extent.offset + extent.count - 1);

		return extent;
	}

	// If we get here then the disk is full!
	printf("Error: Disk full!\n");
	exit(1);
}

void AccessStream(EsFSAttributeFileData *data, uint64_t offset, uint64_t size, void *_buffer, bool write, uint64_t *lastAccessedActualBlock) {
	// TODO Access multiple blocks at a time.
	// TODO Optimise how the extents are calculated.
	// TODO Check that we're in valid stream bounds.
	// TODO Merge last extent if possible.
	
	if (!size) return;

	if (data->indirection == ESFS_DATA_DIRECT) {
		if (write) {
			memcpy(data->direct + offset, _buffer, size);
		} else {
			memcpy(_buffer, data->direct + offset, size);
		}

		return;
	}

	uint64_t offsetBlockAligned = offset & ~(blockSize - 1);
	uint64_t sizeBlocks = BlocksNeededToStore((size + (offset - offsetBlockAligned)));

	uint8_t *buffer = (uint8_t *) _buffer;

	EsFSGlobalExtent *i2ExtentList = nullptr;

	if (data->indirection == ESFS_DATA_INDIRECT_2) {
		i2ExtentList = (EsFSGlobalExtent *) malloc(BlocksNeededToStore(data->extentCount * sizeof(EsFSGlobalExtent)) * blockSize);

		for (int i = 0; i < ESFS_INDIRECT_2_EXTENTS; i++) {
			if (data->indirect2[i]) {
				ReadBlock(data->indirect2[i], 1, i2ExtentList + i * (blockSize / sizeof(EsFSGlobalExtent)));
			}
		}
	}
	
	for (uint64_t i = 0; i < sizeBlocks; i++) {
		// Work out which block contains this data.

		uint64_t blockInStream = offsetBlockAligned / blockSize + i;
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
				printf("Error: Unsupported indirection format %d.\n", data->indirection);
				exit(1);
			} break;
		}

		if (!globalBlock) {
			printf("Error: Could not find block.\n");
			exit(1);
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

		uint8_t blockBuffer[blockSize]; // TODO Stack overflow?
		if (lastAccessedActualBlock) *lastAccessedActualBlock = globalBlock;

		// printf("stream access: block %d, %d in file, with write = %d, offset %d, transfer %d\n", globalBlock, blockInStream, write, offsetIntoBlock, dataToTransfer);

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

	free(i2ExtentList);
}

void ResizeDataStream(EsFSAttributeFileData *data, uint64_t newSize, bool clearNewBlocks, EsFSLoadInformation *dataLoadInformation) {
	uint64_t oldSize = data->size;
	data->size = newSize;

	uint64_t oldBlocks = BlocksNeededToStore(oldSize);
	uint64_t newBlocks = BlocksNeededToStore(newSize);

#if 0
	// Doesn't work with direct data.
	if (oldBlocks == newBlocks) {
		// If the number of blocks needed to store the file hasn't changed,
		// just return.
		return;
	}
#endif

	if (oldSize > newSize) {
		printf("Error: File shrinking not implemented yet.\n");
	}

	uint8_t wasDirect = false;
	uint8_t directTemporary[ESFS_DIRECT_BYTES];

	if (newSize > ESFS_DIRECT_BYTES && data->indirection == ESFS_DATA_DIRECT) {
		// Change from direct to indirect.
		data->indirection = ESFS_DATA_INDIRECT;
		memcpy(directTemporary, data->direct, oldSize);
		wasDirect = true;
		oldBlocks = 0;
	} else if (data->indirection == ESFS_DATA_DIRECT) {
		return;
	}

	uint64_t increaseSize = newSize - oldSize;
	uint64_t increaseBlocks = newBlocks - oldBlocks;

	EsFSGlobalExtent *newExtentList = nullptr;
	uint64_t extentListMaxSize = ESFS_INDIRECT_2_EXTENTS * (blockSize / sizeof(EsFSGlobalExtent));
	uint64_t firstModifiedExtentListBlock = 0;

	while (increaseBlocks) {
		EsFSGlobalExtent newExtent = AllocateExtent(dataLoadInformation->containerBlock / superblock->blocksPerGroup, increaseBlocks);

		if (clearNewBlocks) {
			void *zeroData = malloc(blockSize * newExtent.count);
			memset(zeroData, 0, blockSize * newExtent.count);
			WriteBlock(newExtent.offset, newExtent.count, zeroData);
			free(zeroData);
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
					newExtentList = (EsFSGlobalExtent *) malloc(extentListMaxSize * sizeof(EsFSGlobalExtent));
					memcpy(newExtentList, data->indirect, ESFS_INDIRECT_EXTENTS * sizeof(EsFSGlobalExtent));
					newExtentList[data->extentCount++] = newExtent;
					memset(data->indirect, 0, ESFS_INDIRECT_EXTENTS * sizeof(EsFSGlobalExtent));
				}
			} break;

			case ESFS_DATA_INDIRECT_2: {
				if (!newExtentList) {
					newExtentList = (EsFSGlobalExtent *) malloc(extentListMaxSize * sizeof(EsFSGlobalExtent));

					firstModifiedExtentListBlock = BlocksNeededToStore(data->extentCount * sizeof(EsFSGlobalExtent)) - 1;
					ReadBlock(data->indirect2[firstModifiedExtentListBlock], 1, newExtentList + firstModifiedExtentListBlock * (blockSize / sizeof(EsFSGlobalExtent)));
				}

				newExtentList[data->extentCount++] = newExtent;

				if (extentListMaxSize < data->extentCount) {
					printf("Error: The extent list is too large.\n");
					exit(1);
				}
			} break;
		}
	}

	if (newExtentList) {
		uint64_t blocksNeeded = BlocksNeededToStore(data->extentCount * sizeof(EsFSGlobalExtent));

		for (int i = firstModifiedExtentListBlock; i < blocksNeeded; i++) {
			if (!data->indirect2[i]) {
				data->indirect2[i] = AllocateExtent(dataLoadInformation->containerBlock / superblock->blocksPerGroup, 1).offset;
			}

			WriteBlock(data->indirect2[i], 1, newExtentList + i * (blockSize / sizeof(EsFSGlobalExtent)));
		}

		free(newExtentList);
	}

	if (wasDirect && oldSize) {
		// Copy the direct data into the new blocks.
		AccessStream(data, 0, oldSize, directTemporary, true);
	}
}

bool SearchDirectory(EsFSFileEntry *fileEntry, EsFSFileEntry *output, char *searchName, size_t nameLength, EsFSLoadInformation *loadInformation) {
	EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) FindAttribute(ESFS_ATTRIBUTE_FILE_DIRECTORY, fileEntry + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (!directory) {
		printf("Error: Directory did not have a directory attribute.\n");
		exit(1);
	}

	if (!data) {
		printf("Error: Directory did not have a data attribute.\n");
		exit(1);
	}

	if (data->size == 0 || !directory->itemsInDirectory) {
		if (directory->itemsInDirectory) {
			printf("Error: Directory had items but was 0 bytes.\n");
		}

		return false;
	}

	uint8_t *directoryBuffer = (uint8_t *) malloc(blockSize);
	uint64_t blockPosition = 0, blockIndex = 0;
	uint64_t lastAccessedActualBlock = 0;
	AccessStream(data, blockIndex, blockSize, directoryBuffer, false, &lastAccessedActualBlock);

	size_t fileEntryLength;
	EsFSFileEntry *returnValue = nullptr;

	for (uint64_t i = 0; i < directory->itemsInDirectory; i++) {
		if (blockPosition == blockSize || !directoryBuffer[blockPosition]) {
			// We're reached the end of the block.
			// The next directory entry will be at the start of the next block.
			blockPosition = 0;
			blockIndex++;
			AccessStream(data, blockIndex * blockSize, blockSize, directoryBuffer, false, &lastAccessedActualBlock);
		}

		EsFSDirectoryEntry *entry = (EsFSDirectoryEntry *) (directoryBuffer + blockPosition);

		if (memcmp(entry->signature, ESFS_DIRECTORY_ENTRY_SIGNATURE, strlen(ESFS_DIRECTORY_ENTRY_SIGNATURE))) {
			printf("Error: Directory entry had invalid signature.\n");
			exit(1);
		}

		EsFSAttributeDirectoryName *name = (EsFSAttributeDirectoryName *) FindAttribute(ESFS_ATTRIBUTE_DIRECTORY_NAME, entry + 1);
		if (!name) goto nextFile;
		if (name->nameLength != nameLength) goto nextFile;
		if (memcmp(name + 1, searchName, nameLength)) goto nextFile;

		{
			EsFSAttributeDirectoryFile *file = (EsFSAttributeDirectoryFile *) FindAttribute(ESFS_ATTRIBUTE_DIRECTORY_FILE, entry + 1);

			if (file) {
				returnValue = (EsFSFileEntry *) (file + 1);
				loadInformation->positionInBlock = (uint64_t) returnValue - (uint64_t) directoryBuffer;
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

	if (returnValue) {
		if (output) {
			memcpy(output, directoryBuffer, blockSize);
		}

		loadInformation->containerBlock = lastAccessedActualBlock;

		free(directoryBuffer);
		return true;
	} else {
		free(directoryBuffer);
		return false;
	}
}

void GetEntryForPath(char *path, EsFSFileEntry *directory, EsFSLoadInformation *loadInformation) {
	LoadRootDirectory(directory, loadInformation);

	if (path[0] != '/') {
		printf("Error: Path must start with '/'.\n");
		exit(1);
	}

	if (!path[1]) {
		return;
	}

	bool repositionDirectory = false;

	while (path[0]) {
		path++;
		size_t nameLength = 0;
		while (path[nameLength] && path[nameLength] != '/') nameLength++;
		if (!nameLength) break;

		if (!SearchDirectory(repositionDirectory ? (EsFSFileEntry *) ((uint8_t *) directory + loadInformation->positionInBlock) : directory, directory, path, nameLength, loadInformation)) {
			printf("Error: Could not resolve path.\n");
			exit(1);
		}

		repositionDirectory = true;

		path += nameLength;
	}
}

#endif 

#ifdef ESFS_UTITILES

void ResizeFile(char *path, uint64_t size) {
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) malloc(blockSize);
	EsFSFileEntry *originalFileEntry = fileEntry;
	EsFSLoadInformation information;
	GetEntryForPath(path, fileEntry, &information);
	fileEntry = (EsFSFileEntry *) ((uint8_t *) fileEntry + information.positionInBlock);

	EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (data) {
		ResizeDataStream(data, size, true, &information);
		WriteBlock(information.containerBlock, 1, originalFileEntry);
	} else {
		printf("Error: File did not have a data stream.\n");
		exit(1);
	}

	free(originalFileEntry);
}

void AddFile(char *path, char *name, uint16_t type) {
	if (strlen(name) >= 256) {
		printf("Error: The filename is too long. It can be at maximum 255 bytes.\n");
		exit(1);
	}

	if (!type) {
		printf("Error: Invalid file type.\n");
		exit(1);
	}

	EsFSFileEntry *fileEntry = (EsFSFileEntry *) malloc(blockSize);
	EsFSFileEntry *originalFileEntry = fileEntry;
	EsFSLoadInformation loadInformation;
	GetEntryForPath(path, fileEntry, &loadInformation);
	fileEntry = (EsFSFileEntry *) ((uint8_t *) fileEntry + loadInformation.positionInBlock);

	{
		EsFSLoadInformation temp;
		if (SearchDirectory(fileEntry, nullptr, name, strlen(name), &temp)) {
			printf("Error: File already exists in the directory.\n");
			exit(1);
		}
	}
	
	{
		EntryBufferReset();

		// Step 1: Make the new directory entry.
		EsFSDirectoryEntry *entry = (EsFSDirectoryEntry *) (entryBuffer + entryBufferPosition);
		memcpy(entry->signature, ESFS_DIRECTORY_ENTRY_SIGNATURE, strlen(ESFS_DIRECTORY_ENTRY_SIGNATURE));
		entryBufferPosition += sizeof(EsFSDirectoryEntry);

		char *_n = name;
		EsFSAttributeDirectoryName *name = (EsFSAttributeDirectoryName *) (entryBuffer + entryBufferPosition);
		name->header.type = ESFS_ATTRIBUTE_DIRECTORY_NAME;
		name->header.size = strlen(_n) + sizeof(EsFSAttributeDirectoryName);
		name->nameLength = strlen(_n);
		memcpy(name + 1, _n, strlen(_n));
		entryBufferPosition += name->header.size;

		EsFSAttributeDirectoryFile *file = (EsFSAttributeDirectoryFile *) (entryBuffer + entryBufferPosition);
		file->header.type = ESFS_ATTRIBUTE_DIRECTORY_FILE;
		entryBufferPosition += sizeof(EsFSAttributeDirectoryFile);
		uint64_t temp = entryBufferPosition;

		{
			// Step 2: Make the new file entry.

			EsFSFileEntry *entry = (EsFSFileEntry *) (entryBuffer + entryBufferPosition);
			GenerateUniqueIdentifier(entry->identifier);
			entry->fileType = type;
			memcpy(&entry->signature, ESFS_FILE_ENTRY_SIGNATURE, strlen(ESFS_FILE_ENTRY_SIGNATURE));
			entryBufferPosition += sizeof(EsFSFileEntry);

			EsFSAttributeFileData *data = (EsFSAttributeFileData *) (entryBuffer + entryBufferPosition);
			data->header.type = ESFS_ATTRIBUTE_FILE_DATA;
			data->header.size = sizeof(EsFSAttributeFileData);
			data->stream = ESFS_STREAM_DEFAULT;
			data->indirection = ESFS_DATA_DIRECT;
			entryBufferPosition += data->header.size;

			if (type == ESFS_FILE_TYPE_DIRECTORY) {
				EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) (entryBuffer + entryBufferPosition);
				directory->header.type = ESFS_ATTRIBUTE_FILE_DIRECTORY;
				directory->header.size = sizeof(EsFSAttributeFileDirectory);
				directory->itemsInDirectory = 0;
				directory->spaceAvailableInLastBlock = 0;
				entryBufferPosition += directory->header.size;
			}

			EsFSAttributeHeader *end = (EsFSAttributeHeader *) (entryBuffer + entryBufferPosition);
			end->type = ESFS_ATTRIBUTE_LIST_END;
			end->size = sizeof(EsFSAttributeHeader);
			entryBufferPosition += end->size;
		}

		file->header.size = sizeof(EsFSAttributeDirectoryFile) + entryBufferPosition - temp;

		EsFSAttributeHeader *end = (EsFSAttributeHeader *) (entryBuffer + entryBufferPosition);
		end->type = ESFS_ATTRIBUTE_LIST_END;
		end->size = sizeof(EsFSAttributeHeader);
		entryBufferPosition += end->size;

		if (entryBufferPosition > blockSize) {
			printf("Error: Directory entry for new file exceeds block size.\n");
			exit(1);
		}
	}

	EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) FindAttribute(ESFS_ATTRIBUTE_FILE_DIRECTORY, fileEntry + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (!directory) {
		printf("Error: Directory did not have a directory attribute.\n");
		exit(1);
	}

	if (!data) {
		printf("Error: Directory did not have a data attribute.\n");
		exit(1);
	}

	// printf("spaceAvailableInLastBlock = %d, entryBufferPosition = %d\n", directory->spaceAvailableInLastBlock, entryBufferPosition);

	{
		// Step 3: Store the directory entry.
		if (directory->spaceAvailableInLastBlock >= entryBufferPosition) {
			// There is enough space in the last block.
		} else {
			// We need to add a new block to the file.
			ResizeDataStream(data, data->size + blockSize, true, &loadInformation);
			directory->spaceAvailableInLastBlock = blockSize;
		}

		AccessStream(data, data->size - directory->spaceAvailableInLastBlock, entryBufferPosition, entryBuffer, true);
		directory->spaceAvailableInLastBlock -= entryBufferPosition;

		directory->itemsInDirectory++;
	}

	// Update the directory's file entry.
	WriteBlock(loadInformation.containerBlock, 1, originalFileEntry);
	free(originalFileEntry);
}

void ReadFile(char *path, FILE *output) {
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) malloc(blockSize);
	EsFSFileEntry *originalFileEntry = fileEntry;
	EsFSLoadInformation information;
	GetEntryForPath(path, fileEntry, &information);
	fileEntry = (EsFSFileEntry *) ((uint8_t *) fileEntry + information.positionInBlock);

	EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (data) {
		uint8_t *buffer = (uint8_t *) malloc(data->size);
		AccessStream(data, 0, data->size, buffer, false);

		printf("data->size = %d\n", data->size);

#if 0
		for (int i = 0; i < (data->size / 16) + 1; i++) {
			for (int j = 0; j < 16; j++) {
				if (data->size > j + i * 16) {
					printf("%.2X ", buffer[j + i * 16]);
				} else {
					printf("   ");
				}
			}

#if 1
			printf("    ");

			for (int j = 0; j < 16; j++) {
				if (data->size > j + i * 16) {
					char c = buffer[j + i * 16];
					if (c < 32 || c > 127) {
						printf(".");
					} else {
						printf("%c", c);
					}
				}
			}
#endif

			printf("\n");
		}
#endif

		fwrite(buffer, 1, data->size, output);

		free(buffer);
	} else {
		printf("Error: File did not have a data stream.\n");
		exit(1);
	}

	free(originalFileEntry);
}

void WriteFile(char *path, void *bufferData, uint64_t dataLength) {
	EsFSFileEntry *fileEntry = (EsFSFileEntry *) malloc(blockSize);
	EsFSFileEntry *originalFileEntry = fileEntry;
	EsFSLoadInformation information;
	GetEntryForPath(path, fileEntry, &information);
	fileEntry = (EsFSFileEntry *) ((uint8_t *) fileEntry + information.positionInBlock);

	EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);

	if (data) {
		if (data->size != dataLength) {
			printf("Error: File was not the correct length (%d vs %d).\n", data->size, dataLength);
			exit(1);
		}

		AccessStream(data, 0, data->size, bufferData, true);
		fileEntry->modificationTime++;
		WriteBlock(information.containerBlock, 1, originalFileEntry);
	} else {
		printf("Error: File did not have a data stream.\n");
		exit(1);
	}

	free(originalFileEntry);
}

void AvailableExtents(uint64_t group) {
	if (group >= superblock->groupCount) {
		printf("Error: Drive only has %d groups.\n", superblock->groupCount);
		exit(1);
	}

	EsFSGroupDescriptor *descriptor = &groupDescriptorTable[group].d;
	
	if (!descriptor->extentTable) {
		printf("(group not yet initialised)\n");
		printf("local extent: offset 0 (global %d), count %d\n", group * superblock->blocksPerGroup, GetBlocksInGroup(group));
		return;
	}

	ReadBlock(descriptor->extentTable, BlocksNeededToStore(descriptor->extentCount * sizeof(EsFSLocalExtent)), extentTableBuffer);
	EsFSLocalExtent *extentTable = (EsFSLocalExtent *) extentTableBuffer;

	for (uint16_t i = 0; i < descriptor->extentCount; i++) {
		printf("local extent: offset %d (global %d), count = %d\n", extentTable[i].offset, extentTable[i].offset + group * superblock->blocksPerGroup, extentTable[i].count);
	}
}

void Tree(char *path, int indent) {
	for (int i = 0; i < indent; i++) printf(" ");
	printf("--> %s\n", path);

	EsFSFileEntry *fileEntry = (EsFSFileEntry *) malloc(blockSize);
	EsFSFileEntry *originalFileEntry = fileEntry;
	EsFSLoadInformation information;
	GetEntryForPath(path, fileEntry, &information);
	fileEntry = (EsFSFileEntry *) ((uint8_t *) fileEntry + information.positionInBlock);

	EsFSAttributeFileDirectory *directory = (EsFSAttributeFileDirectory *) FindAttribute(ESFS_ATTRIBUTE_FILE_DIRECTORY, fileEntry + 1);
	EsFSAttributeFileData *data = (EsFSAttributeFileData *) FindAttribute(ESFS_ATTRIBUTE_FILE_DATA, fileEntry + 1);

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
		while (*directoryBufferPosition == 0) {
			directoryBufferPosition++;
		}

		EsFSDirectoryEntry *entry = (EsFSDirectoryEntry *) directoryBufferPosition;

		if (memcmp(entry->signature, ESFS_DIRECTORY_ENTRY_SIGNATURE, strlen(ESFS_DIRECTORY_ENTRY_SIGNATURE))) {
			printf("Error: Directory entry had invalid signature.\n");
			exit(1);
		}

		directoryBufferPosition += sizeof(EsFSDirectoryEntry);
		for (int i = 0; i < indent; i++) printf(" ");

		char *fullPath;

		EsFSAttributeHeader *attribute = (EsFSAttributeHeader *) directoryBufferPosition;
		while (attribute->type != ESFS_ATTRIBUTE_LIST_END) {
			attribute = (EsFSAttributeHeader *) directoryBufferPosition;

			switch (attribute->type) {
				case ESFS_ATTRIBUTE_DIRECTORY_NAME: {
					EsFSAttributeDirectoryName *name = (EsFSAttributeDirectoryName *) attribute;
					printf("    %.*s ", name->nameLength, name + 1);
					for (int i = 0; i < 28 - name->nameLength - indent; i++) printf(" ");
					fullPath = (char *) malloc(name->nameLength + strlen(path) + 2);
					fullPath[name->nameLength + strlen(path)] = 0;
					int a = sprintf(fullPath, "%s%s", path, path[1] ? "/" : "");
					memcpy(fullPath + strlen(fullPath), name + 1, name->nameLength);
					fullPath[name->nameLength + a] = 0;
				} break;

				case ESFS_ATTRIBUTE_DIRECTORY_FILE: {
					EsFSAttributeDirectoryFile *file = (EsFSAttributeDirectoryFile *) attribute;
					EsFSFileEntry *entry = (EsFSFileEntry *) (file + 1);

					if (memcmp(entry->signature, ESFS_FILE_ENTRY_SIGNATURE, strlen(ESFS_FILE_ENTRY_SIGNATURE))) {
						printf("Error: File entry had invalid signature.\n");
						exit(1);
					}

					for (int i = 0; i < 16; i++) printf("%.2X%c", entry->identifier.d[i], i == 15 ? ' ' : '-');
					printf("  ");

					printf("%s ", entry->fileType == ESFS_FILE_TYPE_FILE ? "file  " : 
							(entry->fileType == ESFS_FILE_TYPE_DIRECTORY ? "dir   " : 
								(entry->fileType == ESFS_FILE_TYPE_SYMBOLIC_LINK ? "s-link" : "unrecognised")));
					printf("  ");

					// printf("%d  ", entry->modificationTime);

					if (entry->fileType != ESFS_FILE_TYPE_DIRECTORY) {
						uint8_t *attributePosition = (uint8_t *) (entry + 1);
						EsFSAttributeHeader *attribute = (EsFSAttributeHeader *) attributePosition;

						while (attribute->type != ESFS_ATTRIBUTE_LIST_END) {
							attribute = (EsFSAttributeHeader *) attributePosition;

							switch (attribute->type) {
								case ESFS_ATTRIBUTE_FILE_DATA: {
									printf("%d bytes", ((EsFSAttributeFileData *) attribute)->size);
								} break;
							}

							attributePosition += attribute->size;
						}
					}

					printf("\n");

					if (entry->fileType == ESFS_FILE_TYPE_DIRECTORY) {
						Tree(fullPath, indent + 4);
					}
				} break;

				case ESFS_ATTRIBUTE_LIST_END: break;

				default: {
					// Unrecognised attribute.
				} break;
			}

			directoryBufferPosition += attribute->size;
		}

		free(fullPath);
	}

	for (int i = 0; i < indent; i++) printf(" ");
	if (directory->itemsInDirectory) {
		printf("    (%d item%s)\n", directory->itemsInDirectory, directory->itemsInDirectory > 1 ? "s" : "");
	} else {
		printf("    (empty directory)\n");
	}

	free(directoryBuffer);
	free(originalFileEntry);
}

void Import(char *target, char *source) {
	DIR *d = opendir(source);
	struct dirent *dir;
	char nameBuffer1[256];
	char nameBuffer2[256];

	if (d) {
		while ((dir = readdir(d))) {
			if (dir->d_name[0] != '.') {
				sprintf(nameBuffer1, "%s%s", target, dir->d_name);
				sprintf(nameBuffer2, "%s%s", source, dir->d_name);

				struct stat s = {};
				stat(nameBuffer2, &s);
				if (S_ISDIR(s.st_mode)) {
					AddFile(target, dir->d_name, ESFS_FILE_TYPE_DIRECTORY);
					strcat(nameBuffer1, "/");
					strcat(nameBuffer2, "/");
					Import(nameBuffer1, nameBuffer2);
				} else {
					// printf("Importing %s at %s...\n", nameBuffer2, nameBuffer1);

					FILE *input = fopen(nameBuffer2, "rb");
					if (!input) printf("Warning: Could not open file!\n");
					else {
						fseek(input, 0, SEEK_END);
						uint64_t fileLength = ftell(input);
						fseek(input, 0, SEEK_SET);
						void *data = malloc(fileLength);
						fread(data, 1, fileLength, input);
						fclose(input);

						AddFile(target, dir->d_name, ESFS_FILE_TYPE_FILE);
						ResizeFile(nameBuffer1, fileLength);
						WriteFile(nameBuffer1, data, fileLength);

						free(data);
					}
				}
			}
		}

		closedir(d);
	}

}

int main(int argc, char **argv) {
	randomFile = fopen("/dev/urandom", "rb");

	if (argc < 4) {
		printf("Usage: <drive> <partition_offset> <command> <options>\n");
		exit(1);
	}

	char *driveFilename = argv[1];
	partitionOffset = ParseSizeString(argv[2]);
	char *command = argv[3];

	argc -= 4;
	argv += 4;

	drive = fopen(driveFilename, "a+b");
	if (drive) fclose(drive);
	drive = fopen(driveFilename, "r+b");

	if (!drive) {
		printf("Error: Could not open drive file.\n");
		exit(1);
	}

	fseek(drive, 0, SEEK_SET);

#define IS_COMMAND(_a) 0 == strcmp(command, _a)
#define CHECK_ARGS(_n, _u) if (argc != _n) { printf("Usage: <drive> %s\n", _u); exit(1); }

	if (IS_COMMAND("format")) {
		CHECK_ARGS(3, "format <size> <name> <kernel>");
		uint64_t driveSize = ParseSizeString(argv[0]);

		if (driveSize < ESFS_DRIVE_MINIMUM_SIZE) {
			printf("Error: Cannot create a drive of %d bytes (too small).\n", driveSize);
			exit(1);
		}

		if (truncate(driveFilename, driveSize)) {
			printf("Error: Could not change the file's size to %d bytes.\n", driveSize);
			exit(1);
		}

		if (strlen(argv[1]) > ESFS_MAXIMUM_VOLUME_NAME_LENGTH) {
			printf("Error: Volume name '%s' is too long; must be <= %d bytes.\n", argv[1], ESFS_MAXIMUM_VOLUME_NAME_LENGTH);
			exit(1);
		}

		FILE *input = fopen(argv[2], "rb");

		if (!input) {
			printf("Error: Could not open input file.\n");
			exit(1);
		}

		fseek(input, 0, SEEK_END);
		uint64_t fileLength = ftell(input);
		fseek(input, 0, SEEK_SET);
		void *data = malloc(fileLength);
		fread(data, 1, fileLength, input);
		fclose(input);

		FormatVolume(driveSize, argv[1], data, fileLength);

		free(data);
	} else if (IS_COMMAND("tree")) {
		CHECK_ARGS(1, "tree <path>");

		MountVolume();
		Tree(argv[0], 0);
		UnmountVolume();
	} else if (IS_COMMAND("available-extents")) {
		CHECK_ARGS(1, "available-extents <group>");

		MountVolume();
		AvailableExtents(ParseSizeString(argv[0]));
		UnmountVolume();
	} else if (IS_COMMAND("create")) {
		CHECK_ARGS(3, "create <path> <name> <file/directory>");

		MountVolume();
		AddFile(argv[0], argv[1], argv[2][0] == 'f' ? ESFS_FILE_TYPE_FILE : (argv[2][0] == 'd' ? ESFS_FILE_TYPE_DIRECTORY : 0));
		UnmountVolume();
	} else if (IS_COMMAND("resize")) {
		CHECK_ARGS(2, "resize <path> <size>");

		MountVolume();
		ResizeFile(argv[0], ParseSizeString(argv[1]));
		UnmountVolume();
	} else if (IS_COMMAND("read")) {
		CHECK_ARGS(2, "read <path> <output_file>");

		FILE *output = fopen(argv[1], "wb");

		if (!output) {
			printf("Error: Could not open output file.\n");
			exit(1);
		}

		MountVolume();
		ReadFile(argv[0], output);
		UnmountVolume();
	} else if (IS_COMMAND("write")) {
		CHECK_ARGS(2, "write <path> <input_file>");

		FILE *input = fopen(argv[1], "rb");

		if (!input) {
			printf("Error: Could not open input file.\n");
			exit(1);
		}

		fseek(input, 0, SEEK_END);
		uint64_t fileLength = ftell(input);
		fseek(input, 0, SEEK_SET);
		void *data = malloc(fileLength);
		fread(data, 1, fileLength, input);
		fclose(input);

		MountVolume();
		WriteFile(argv[0], data, fileLength);
		UnmountVolume();

		free(data);
	} else if (IS_COMMAND("import")) {
		CHECK_ARGS(2, "import <target_path> <folder>");

		MountVolume();
		Import(argv[0], argv[1]);
		UnmountVolume();
	} else {
		printf("Unrecognised command '%s'.\n", command);
		exit(1);
	}

	return 0;
}

#endif 
