#ifdef IMPLEMENTATION

typedef struct {
	uint32_t magicNumber; // 0x7F followed by 'ELF'
	uint8_t bits; // 1 = 32 bit, 2 = 64 bit
	uint8_t endianness; // 1 = LE, 2 = BE
	uint8_t version1;
	uint8_t abi; // 0 = System V
	uint8_t _unused0[8];
	uint16_t type; // 1 = relocatable, 2 = executable, 3 = shared
	uint16_t instructionSet; // 0x03 = x86, 0x28 = ARM, 0x3E = x86-64, 0xB7 = AArch64
	uint32_t version2;

#ifdef ARCH_32
	uint32_t entry;
	uint32_t programHeaderTable;
	uint32_t sectionHeaderTable;
	uint32_t flags;
	uint16_t headerSize;
	uint16_t programHeaderEntrySize;
	uint16_t programHeaderEntries;
	uint16_t sectionHeaderEntrySize;
	uint16_t sectionHeaderEntries;
	uint16_t sectionNameIndex;
#else
	uint64_t entry;
	uint64_t programHeaderTable;
	uint64_t sectionHeaderTable;
	uint32_t flags;
	uint16_t headerSize;
	uint16_t programHeaderEntrySize;
	uint16_t programHeaderEntries;
	uint16_t sectionHeaderEntrySize;
	uint16_t sectionHeaderEntries;
	uint16_t sectionNameIndex;
#endif
} ElfHeader;

#ifdef ARCH_32
typedef struct {
	uint32_t type; // 0 = unused, 1 = load, 2 = dynamic, 3 = interp, 4 = note
	uint32_t fileOffset;
	uint32_t virtualAddress;
	uint32_t _unused0;
	uint32_t dataInFile;
	uint32_t segmentSize;
	uint32_t flags; // 1 = executable, 2 = writable, 4 = readable
	uint32_t alignment;
} ElfProgramHeader;
#else
typedef struct {
	uint32_t type; // 0 = unused, 1 = load, 2 = dynamic, 3 = interp, 4 = note
	uint32_t flags; // 1 = executable, 2 = writable, 4 = readable
	uint64_t fileOffset;
	uint64_t virtualAddress;
	uint64_t _unused0;
	uint64_t dataInFile;
	uint64_t segmentSize;
	uint64_t alignment;
} ElfProgramHeader;
#endif

uintptr_t LoadELF(char *imageName, size_t imageNameLength) {
	Process *thisProcess = ProcessorGetLocalStorage()->currentThread->process;

	File *file = vfs.OpenFile(imageName, imageNameLength);

	if (!file) {
		// We couldn't open the ELF.
		return 0;
	}

	Defer(vfs.CloseFile(file));

	bool s;

	ElfHeader header;
	s = file->Read(0, sizeof(ElfHeader), (uint8_t *) &header);
	if (!s) return 0;

	size_t programHeaderEntrySize = header.programHeaderEntrySize;

	if (header.magicNumber != 0x464C457F) return 0;
	if (header.bits != 2) return 0;
	if (header.endianness != 1) return 0;
	if (header.abi != 0) return 0;
	if (header.type != 2) return 0;
	if (header.instructionSet != 0x3E) return 0;

	ElfProgramHeader *programHeaders = (ElfProgramHeader *) kernelVMM.Allocate(programHeaderEntrySize * header.programHeaderEntries);
	Defer(kernelVMM.Free(programHeaders));

	s = file->Read(header.programHeaderTable, programHeaderEntrySize * header.programHeaderEntries, (uint8_t *) programHeaders);
	if (!s) return 0;

	for (uintptr_t i = 0; i < header.programHeaderEntries; i++) {
		ElfProgramHeader *header = (ElfProgramHeader *) ((uint8_t *) programHeaders + programHeaderEntrySize * i);
		if (header->type != 1) continue;

		void *segment = (void *) header->virtualAddress;

		thisProcess->vmm->lock.Acquire();
		bool success = thisProcess->vmm->AddRegion((uintptr_t) segment, (header->segmentSize / PAGE_SIZE) + 1, 0, vmmRegionStandard, vmmMapAll, true);
		thisProcess->vmm->lock.Release();

		if (!success) {
			return 0;
		}

		// TODO Memory-map the file.
		s = file->Read(header->fileOffset, header->dataInFile, (uint8_t *) segment);
		if (!s) return 0;
	}

	return header.entry;
}

#endif
