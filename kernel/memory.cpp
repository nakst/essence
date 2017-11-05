#ifndef IMPLEMENTATION

#define USE_OLD_POOLS 0

#ifdef ARCH_X86_64
#define PAGE_BITS (12)
#define PAGE_SIZE (1 << PAGE_BITS)
#endif

struct PhysicalMemoryRegion {
	uint64_t baseAddress;

	// The memory map the BIOS provides gives the information in pages.
	uint64_t pageCount;
};

extern PhysicalMemoryRegion *physicalMemoryRegions;
extern size_t physicalMemoryRegionsCount;
extern size_t physicalMemoryRegionsPagesCount;
extern size_t physicalMemoryOriginalPagesCount;
extern size_t physicalMemoryRegionsIndex;
extern uintptr_t physicalMemoryHighest;

#ifdef ARCH_X86_64
extern bool pagingNXESupport;
extern bool pagingPCIDSupport;
extern bool pagingSMEPSupport;
extern bool pagingTCESupport;

extern "C" void processorGDTR();
extern "C" void SetupProcessor2();
extern "C" void ProcessorInstallTSS(uint32_t *gdt, uint32_t *tss);
#endif

struct PMM {
	uintptr_t AllocatePage();
	uintptr_t AllocateContiguous64KB();
	uintptr_t AllocateContiguous128KB();
	void FreePage(uintptr_t address, bool bypassStack = false);
	void Initialise();

	uintptr_t pagesAllocated;
	uintptr_t startPageCount;

#define PMM_PAGE_STACK_SIZE 1024
	uintptr_t pageStack[1024];
	uintptr_t pageStackIndex;

#define BITSET_GROUP_PAGES 4096
	uint32_t *bitsetPageUsage;
	uint16_t *bitsetGroupUsage; 
	size_t bitsetPages; 
	size_t bitsetGroups;

	Spinlock lock; 
};

extern PMM pmm;

enum VMMRegionType {
	vmmRegionFree,
	vmmRegionStandard, // Allocate and zero on write
	vmmRegionPhysical,
	vmmRegionShared,  
};

enum VMMMapPolicy {
	vmmMapLazy,
	vmmMapAll, // These are not stored in the region lookup array
};

#define VMM_REGION_FLAG_NOT_CACHABLE (0)
#define VMM_REGION_FLAG_CACHABLE (1)

struct VMMRegion {
	uintptr_t baseAddress;
	size_t pageCount;

	// Incremented when a syscall is using the region.
	// To remove the region this must be 0.
	volatile unsigned lock;

	// Fields used to distinguish regions in lookupRegions
	uintptr_t offset;
	void *object;
	VMMRegionType type;
	VMMMapPolicy mapPolicy;
	unsigned flags;
};

struct VirtualAddressSpace {
	void Map(uintptr_t physicalAddress,
		 uintptr_t virtualAddress,
		 unsigned flags);
	void Remove(uintptr_t virtualAddress, size_t pageCount);
	uintptr_t Get(uintptr_t virtualAddress, bool force = false);
	
	bool userland;
	Spinlock lock;

#ifdef ARCH_X86_64
#define VIRTUAL_ADDRESS_SPACE_IDENTIFIER(x) ((x)->cr3)
	uintptr_t cr3;
	
#define PAGE_TABLE_L4 ((volatile uint64_t *) 0xFFFFFFFFFFFFF000)
#define PAGE_TABLE_L3 ((volatile uint64_t *) 0xFFFFFFFFFFE00000)
#define PAGE_TABLE_L2 ((volatile uint64_t *) 0xFFFFFFFFC0000000)
#define PAGE_TABLE_L1 ((volatile uint64_t *) 0xFFFFFF8000000000)
#define ENTRIES_PER_PAGE_TABLE (512)
#define ENTRIES_PER_PAGE_TABLE_BITS (9)
#endif
};

struct VMM {
	void Initialise();
	void Destroy(); // This MUST be called with the kernelVMM active!

	void *Allocate(size_t size, VMMMapPolicy mapPolicy = vmmMapLazy, VMMRegionType type = vmmRegionStandard, uintptr_t offset = 0, unsigned flags = VMM_REGION_FLAG_CACHABLE, void *object = nullptr);
	OSError Free(void *address, void **object = nullptr, VMMRegionType *type = nullptr, bool skipVirtualAddressSpaceUpdate = false);
	bool HandlePageFault(uintptr_t address);
	VMMRegion *FindAndLockRegion(uintptr_t address, size_t size);
	void UnlockRegion(VMMRegion *region);

	void LogRegions();
	void LogRegionMappings(uintptr_t address);

	VirtualAddressSpace virtualAddressSpace;
	Mutex lock; 

	bool AddRegion(uintptr_t baseAddress, size_t pageCount, uintptr_t offset, VMMRegionType type, VMMMapPolicy mapPolicy, unsigned flags, void *object);
	uintptr_t AddRegion(VMMRegion *region, VMMRegion *&array, size_t &arrayCount, size_t &arrayAllocated);
	bool HandlePageFaultInRegion(uintptr_t page, VMMRegion *region);
	VMMRegion *FindRegion(uintptr_t address, VMMRegion *array, size_t arrayCount);
	void MergeIdenticalAdjacentRegions(VMMRegion *region, VMMRegion *array, size_t &arrayCount);
	void SplitRegion(VMMRegion *region, uintptr_t address, bool keepAbove, VMMRegion *array, size_t &arrayCount, size_t &arrayAllocated);

	// This contains the canonical "split" regions.
	VMMRegion *regions;
	size_t regionsCount, regionsAllocated;

	// This contains the regions that can be used for working out how to resolve a page fault.
	// i.e. If 2 regions are identical then they will be merged in this list.
	VMMRegion *lookupRegions;
	size_t lookupRegionsCount, lookupRegionsAllocated;

	uintptr_t allocatedVirtualMemory, allocatedPhysicalMemory;

#ifdef ARCH_X86_64
	uint64_t *pageTable;
#endif
};

extern VMM kernelVMM;

struct SharedMemoryRegion {
	Mutex mutex;
	volatile size_t handles;
	size_t sizeBytes;
	bool named;
	
	// Follows is a list of physical addresses,
	// that contain the memory in the region.
};

struct NamedSharedMemoryRegion {
	SharedMemoryRegion *region;
	char name[OS_SHARED_MEMORY_NAME_MAX_LENGTH];
	size_t nameLength;
};

struct SharedMemoryManager {
	SharedMemoryRegion *CreateSharedMemory(size_t sizeBytes, char *name = nullptr, size_t nameLength = 0);
	SharedMemoryRegion *LookupSharedMemory(char *name, size_t nameLength);
	void DestroySharedMemory(SharedMemoryRegion *region);

	NamedSharedMemoryRegion *namedSharedMemoryRegions;
	size_t namedSharedMemoryRegionsCount, namedSharedMemoryRegionsAllocated;

	Mutex mutex;
};

SharedMemoryManager sharedMemoryManager;

#if USE_OLD_POOLS
#define POOL_GROUP_PADDING 32
struct PoolGroup {
	union {
		struct {
			struct PoolGroup *next;
			struct PoolGroup **reference;
			size_t elementsUsed;
		};

		uint8_t _padding[POOL_GROUP_PADDING];
	};

	uint8_t data[PAGE_SIZE * 32 - POOL_GROUP_PADDING];
};

struct Pool {
	void Initialise(size_t _elementSize);
	void *Add(); // Aligned to the size of a pointer.
	void Remove(void *element);

	Mutex lock; 

	size_t elementSize;

#define POOL_STACK_SIZE 1024
	void *stack[POOL_STACK_SIZE];
	size_t stackCount;

	PoolGroup *availableGroups;
	PoolGroup *fullGroups; // TODO This list isn't used anymore; clean it up.

	size_t elementsPerGroup;
	size_t bitsetLength;

	bool concurrentModificationCheck;
};
#else
struct Pool {
	void Initialise(size_t _elementSize);
	void *Add(); // Aligned to the size of a pointer.
	void Remove(void *element);
	size_t elementSize;
};
#endif

#endif

#ifdef IMPLEMENTATION

PMM pmm;
VMM kernelVMM;

void VMM::Initialise() {
	lock.Acquire();
	Defer(lock.Release());

	if (this == &kernelVMM) {
		virtualAddressSpace.userland = false;

#ifdef ARCH_X86_64
		// See HandlePageFault for a memory map.
		
		regions = (VMMRegion *) 0xFFFF8F0000000000;
		// regionsAllocated = 0x8000000000 / sizeof(VMMRegion);
		regionsAllocated = 65536; // Use a sane maximum to detect memory leaks easier.
		lookupRegions = (VMMRegion *) 0xFFFF8F8000000000;
		// regionsAllocated = 0x8000000000 / sizeof(VMMRegion);
		lookupRegionsAllocated = 65536; 
		AddRegion(0xFFFF900000000000, 0x600000000000 >> PAGE_BITS /* 96TiB */, 0, vmmRegionFree, vmmMapLazy, true, nullptr);

		virtualAddressSpace.cr3 = ProcessorReadCR3();
#endif
	} else {
		virtualAddressSpace.userland = true;

		// Initialise the virtual address space for a new process.
#ifdef ARCH_X86_64
		AddRegion(0x100000000000, 0x600000000000 >> PAGE_BITS /* 96TiB */, 0, vmmRegionFree, vmmMapLazy, true, nullptr);

		virtualAddressSpace.cr3 = pmm.AllocatePage();
		// KernelLog(LOG_INFO, "cr3 = %x\n", virtualAddressSpace.cr3);
		pageTable = (uint64_t *) kernelVMM.Allocate(PAGE_SIZE, vmmMapAll, vmmRegionPhysical, (uintptr_t) virtualAddressSpace.cr3);
		ZeroMemory(pageTable + 0x000, PAGE_SIZE / 2);
		CopyMemory(pageTable + 0x100, (uint64_t *) (PAGE_TABLE_L4 + 0x100), PAGE_SIZE / 2);
		pageTable[512 - 1] = virtualAddressSpace.cr3 | 3;
#endif
	}
}

void ValidateCurrentVMM(VMM *target) {
	if (target != &kernelVMM 
			&& target != GetCurrentThread()->process->vmm 
			&& &target->virtualAddressSpace != GetCurrentThread()->asyncTempAddressSpace) {
		KernelPanic("ValidateCurrentVMM - Attempt to modify a VMM with different VMM active.\n");
	}
}

#ifdef ARCH_X86_64
void CleanupVirtualAddressSpace(void *argument) {
	pmm.lock.Acquire();
	// KernelLog(LOG_INFO, "Removing virtual address space page %x...\n", argument);
	// KernelLog(LOG_INFO, "Current CR3 is %x\n", ProcessorGetAddressSpace());
	pmm.FreePage((uintptr_t) argument);
	pmm.lock.Release();
}
#endif

void VMM::Destroy() {
	ValidateCurrentVMM(this);

	while (true) {
		uintptr_t regionsFreed = 0;
		for (uintptr_t i = 0; i < regionsCount; i++) {
			VMMRegion *region = regions + i;

			if (region->type != vmmRegionFree) {
				regionsFreed++;
				// KernelLog(LOG_VERBOSE, "Freeing VMM region at %x...\n", region->baseAddress);
				Free((void *) region->baseAddress);
			}
		}
		if (!regionsFreed) break;
	}

	// KernelLog(LOG_VERBOSE, "Freeing region arrays...\n");

	if (regions) kernelVMM.Free(regions);
	if (lookupRegions) kernelVMM.Free(lookupRegions);

#ifdef ARCH_X86_64
	// KernelLog(LOG_VERBOSE, "Freeing virtual address space...\n");
	pmm.lock.Acquire();
	for (uintptr_t i = 0; i < 256; i++) {
		if (PAGE_TABLE_L4[i]) {
			for (uintptr_t j = i * 512; j < (i + 1) * 512; j++) {
				if (PAGE_TABLE_L3[j]) {
					for (uintptr_t k = j * 512; k < (j + 1) * 512; k++) {
						if (PAGE_TABLE_L2[k]) {
							pmm.FreePage(PAGE_TABLE_L2[k] & (~0xFFF));
						}
					}
					pmm.FreePage(PAGE_TABLE_L3[j] & (~0xFFF));
				}
			}
			pmm.FreePage(PAGE_TABLE_L4[i] & (~0xFFF));
		}
	}
	pmm.lock.Release();

	kernelVMM.Free(pageTable);
#endif

	// TODO Work out why enabling this can cause triple faults?
	// 	It looks like the VAS may still be used causing fetch PFs?
	// 	--> On async task threads??!?
#if ARCH_X86_64
	scheduler.lock.Acquire();
	RegisterAsyncTask(CleanupVirtualAddressSpace, (void *) virtualAddressSpace.cr3, kernelProcess);
	scheduler.lock.Release();
#endif

	// KernelLog(LOG_VERBOSE, "VMM destroyed,\n");
}

uintptr_t VMM::AddRegion(VMMRegion *region, VMMRegion *&array, size_t &arrayCount, size_t &arrayAllocated) {
	if (arrayCount == arrayAllocated) {
		if (this == &kernelVMM) {
			KernelPanic("VMM::AddRegion - Maximum kernel VMM regions (%d) exceeded\n", arrayAllocated);
		} else {
			if (arrayCount) {
				arrayAllocated *= 2;
				VMMRegion *old = array;
				array = (VMMRegion *) kernelVMM.Allocate(arrayAllocated * sizeof(VMMRegion), vmmMapAll);
				CopyMemory(array, old, arrayCount * sizeof(VMMRegion));
				kernelVMM.Free(old);
			} else {
				arrayAllocated = 256;
				array = (VMMRegion *) kernelVMM.Allocate(arrayAllocated * sizeof(VMMRegion), vmmMapAll);
			}
		}
	}

	uintptr_t regionIndex = arrayCount;
	array[regionIndex] = *region;
	arrayCount++;

	return regionIndex;
}

bool VMM::AddRegion(uintptr_t baseAddress, size_t pageCount, uintptr_t offset, VMMRegionType type, VMMMapPolicy mapPolicy, unsigned flags, void *object) {
	lock.AssertLocked();

	if (FindRegion(baseAddress, regions, regionsCount)) {
		// This new region intersects an already existing region.
		// Fail.
		return false;
	}

	VMMRegion region = {};
	region.baseAddress = baseAddress;
	region.pageCount = pageCount;
	region.offset = offset;
	region.type = type;
	region.mapPolicy = mapPolicy;
	region.flags = flags;
	region.object = object;

	if (type != vmmRegionFree) {
		allocatedVirtualMemory += pageCount * PAGE_SIZE;
	}

	uintptr_t regionIndex = AddRegion(&region, regions, regionsCount, regionsAllocated);

	if (type != vmmRegionFree && mapPolicy != vmmMapAll) {
		uintptr_t lookupRegionIndex = AddRegion(&region, lookupRegions, lookupRegionsCount, lookupRegionsAllocated);
		MergeIdenticalAdjacentRegions(lookupRegions + lookupRegionIndex, lookupRegions, lookupRegionsCount);
	}

	// Map the first few pages in to reduce the number of initial page faults.
	if (type != vmmRegionFree) {
		if (type == vmmRegionShared) {
			SharedMemoryRegion *region = (SharedMemoryRegion *) object;
			region->mutex.Acquire();
			region->handles++; // Increment the number of handles to the shared memory region.
		}

		virtualAddressSpace.lock.Acquire();
		Defer(virtualAddressSpace.lock.Release());

		HandlePageFaultInRegion(baseAddress, regions + regionIndex);

		if (type == vmmRegionShared) {
			SharedMemoryRegion *region = (SharedMemoryRegion *) object;
			region->mutex.Release();
		}
	}

	return true;
}

void *VMM::Allocate(size_t size, VMMMapPolicy mapPolicy, VMMRegionType type, uintptr_t offset, unsigned flags, void *object) {
	lock.Acquire();
	Defer(lock.Release());

	ValidateCurrentVMM(this);

	if (!size) return nullptr;

	size_t pageCount = size >> PAGE_BITS;
	if (size & (PAGE_SIZE - 1)) pageCount++;

	// TODO Should we look for the smallest/largest/first/last/etc. region?
	uintptr_t i = 0;

	for (; i < regionsCount; i++) {
		if (regions[i].type == vmmRegionFree
				&& regions[i].pageCount > pageCount) {
			break;
		}
	}

	if (i == regionsCount) {
		return nullptr;
	}

	VMMRegion *region = regions + i;
	uintptr_t baseAddress = region->baseAddress;
	region->baseAddress += pageCount << PAGE_BITS;
	region->pageCount -= pageCount;

	bool success = AddRegion(baseAddress, pageCount, offset & ~(PAGE_SIZE - 1), type, mapPolicy, flags, object);
	if (!success) return nullptr;
	void *address = (void *) (baseAddress + (offset & (PAGE_SIZE - 1)));

	// KernelLog(LOG_VERBOSE, "Allocated %x -> %x (%d bytes)\n", baseAddress, baseAddress + size, size);
	return address;
}

void VMM::MergeIdenticalAdjacentRegions(VMMRegion *region, VMMRegion *array, size_t &arrayCount) {
	lock.AssertLocked();

	intptr_t remove1 = -1, remove2 = -1;

	for (uintptr_t i = 0; i < arrayCount && (remove1 != -1 || remove2 != 1); i++) {
		VMMRegion *r = array + i;

		if (r->type != region->type) continue;
		if (r == region) continue;

		if (r->type != vmmRegionStandard && r->type != vmmRegionFree) {
			// We can only merge standard and free regions.
			continue;
		}

		if (r->type == vmmRegionStandard) {
			if (r->mapPolicy != region->mapPolicy) continue;
			if (r->offset != region->offset) continue;
			if (r->flags != region->flags) continue;
			if (r->object != region->object) continue;
		}

		if (r->baseAddress == region->baseAddress + (region->pageCount << PAGE_BITS)) {
			region->pageCount += r->pageCount;
			remove1 = i;
		} else if (region->baseAddress == r->baseAddress + (r->pageCount << PAGE_BITS)){ 
			region->pageCount += r->pageCount;
			region->baseAddress = r->baseAddress;
			remove2 = i;
		}
	}

	if (remove1 != -1) array[remove1] = array[--arrayCount];
	if (remove2 == (intptr_t) arrayCount) remove2 = remove1;
	if (remove2 != -1) array[remove2] = array[--arrayCount];
}

void VMM::SplitRegion(VMMRegion *region, uintptr_t address, bool keepAbove, VMMRegion *array, size_t &arrayCount, size_t &arrayAllocated) {
	lock.AssertLocked();

	if (region->baseAddress == address) {
		return;
	}

	if (region->baseAddress + (region->pageCount << PAGE_BITS) == address) {
		return;
	}

	VMMRegion *newRegion = array + AddRegion(region, array, arrayCount, arrayAllocated);

	if (keepAbove) {
		VMMRegion *t = newRegion;
		newRegion = region;
		region = t;
	}

	newRegion->baseAddress = address;
	newRegion->pageCount -= (address - region->baseAddress) >> PAGE_BITS;
	region->pageCount -= newRegion->pageCount;
}

OSError VMM::Free(void *address, void **object, VMMRegionType *type, bool skipVirtualAddressSpaceUpdate) {
	if (!address) {
		return OS_ERROR_INVALID_MEMORY_REGION;
	}

	ValidateCurrentVMM(this);

	lock.Acquire();
	Defer(lock.Release());

	uintptr_t baseAddress = (uintptr_t) address;
	VMMRegion *region = FindRegion(baseAddress, regions, regionsCount);

	if (!region) {
		return OS_ERROR_INVALID_MEMORY_REGION;
	} else if (region->lock) {
		return OS_ERROR_MEMORY_REGION_LOCKED_BY_KERNEL;
	}

	if (object) *object = region->object;
	if (type) *type = region->type;

	if (region->type == vmmRegionShared) {
		if (!region->object) KernelPanic("VMM::Free - Object from a freed shared memory region was null.\n");
		CloseHandleToObject(region->object, KERNEL_OBJECT_SHMEM);
	}

	allocatedVirtualMemory -= region->pageCount * PAGE_SIZE;

	{
		pmm.lock.Acquire();
		Defer(pmm.lock.Release());

		virtualAddressSpace.lock.Acquire();
		Defer(virtualAddressSpace.lock.Release());

		for (uintptr_t address = region->baseAddress;
				address < region->baseAddress + (region->pageCount << PAGE_BITS);
				address += PAGE_SIZE) {
			// TODO Optimise this for non-existant page tables.
			uintptr_t mappedAddress = virtualAddressSpace.Get(address);

			if (mappedAddress) {
				switch (region->type) {
					case vmmRegionStandard: {
						pmm.FreePage(mappedAddress);
						allocatedPhysicalMemory -= PAGE_SIZE;
					} break;

					case vmmRegionPhysical: {
						// Do nothing.
					} break;

					case vmmRegionShared: {
						// This memory will be freed when the last handle to the shared memory region is closed.
					} break;

					default: {
						KernelPanic("VMM::Free - Cannot free VMMRegion of type %d\n", region->type);
					} break;
				}
			}
		}

		if (!skipVirtualAddressSpaceUpdate) {
			virtualAddressSpace.Remove(region->baseAddress, region->pageCount);
		}
	}

	size_t regionPageCount = region->pageCount;
	VMMMapPolicy mapPolicy = region->mapPolicy;

	if (mapPolicy != vmmMapAll) {
		VMMRegion *lookupRegion = FindRegion(baseAddress, lookupRegions, lookupRegionsCount);
		SplitRegion(lookupRegion, baseAddress, true, lookupRegions, lookupRegionsCount, lookupRegionsAllocated);
		SplitRegion(lookupRegion, baseAddress + (regionPageCount << PAGE_BITS), false, lookupRegions, lookupRegionsCount, lookupRegionsAllocated);
		*lookupRegion = lookupRegions[--lookupRegionsCount];
	}

	region->type = vmmRegionFree;
	MergeIdenticalAdjacentRegions(region, regions, regionsCount);

	return OS_SUCCESS;
}

bool VMM::HandlePageFaultInRegion(uintptr_t page, VMMRegion *region) {
	lock.AssertLocked();
	virtualAddressSpace.lock.AssertLocked();

	uintptr_t pageInRegion = (page - region->baseAddress) >> PAGE_BITS;

	for (uintptr_t address = page, i = 0; 
			// If we need to map all pages in now, then do that.
			// Otherwise, just map in this page and any in the following 16 that haven't been mapped already.
			// (But make sure we don't go outside the region).
			(region->mapPolicy == vmmMapAll || i < 16) && pageInRegion + i < region->pageCount; 
			address += PAGE_SIZE, i++) {

		switch (region->type) {
			case vmmRegionStandard: {
				if (address == page || !virtualAddressSpace.Get(address)) {
					uintptr_t physicalPage = pmm.AllocatePage();
					allocatedPhysicalMemory += PAGE_SIZE;
					virtualAddressSpace.Map(physicalPage, address, region->flags);
					ZeroMemory((void *) address, PAGE_SIZE); // TODO "Clean" physical pages during idle.
				} else {
					// Ignore pages that have already been mapped.
				}
			} break;

			case vmmRegionPhysical: {
				virtualAddressSpace.Map(address - region->baseAddress + region->offset, address, region->flags);
			} break;

			case vmmRegionShared: {
				if (address == page || !virtualAddressSpace.Get(address)) {
					SharedMemoryRegion *sharedRegion = (SharedMemoryRegion *) region->object;
					sharedRegion->mutex.AssertLocked();
					uintptr_t *volatile addresses = (uintptr_t *) (sharedRegion + 1);
					uintptr_t index = (address - region->baseAddress + region->offset) >> PAGE_BITS;

					if (addresses[index]) {
						virtualAddressSpace.Map(addresses[index], address, region->flags);
					} else {
						// NOTE Duplicated from above.
						uintptr_t physicalPage = pmm.AllocatePage();
						allocatedPhysicalMemory += PAGE_SIZE;
						virtualAddressSpace.Map(physicalPage, address, region->flags);
						ZeroMemory((void *) address, PAGE_SIZE); // TODO "Clean" physical pages during idle.
						addresses[index] = physicalPage;
					}
				} else {
					// Ignore pages that have already been mapped.
				}
			} break;

			default: {
				 return false;
			} break;
		}
	}

	return true;
}

VMMRegion *VMM::FindRegion(uintptr_t address, VMMRegion *array, size_t arrayCount) {
	lock.AssertLocked();

	for (uintptr_t i = 0; i < arrayCount; i++) {
		VMMRegion *region = array + i;

		if (region->baseAddress <= address
				&& region->baseAddress + (region->pageCount << PAGE_BITS) > address) {
			return region;
		}
	}

	return nullptr;
}

VMMRegion *VMM::FindAndLockRegion(uintptr_t address, size_t size) {
	if (!address) return nullptr;

	lock.Acquire();
	Defer(lock.Release());

	VMMRegion *region = FindRegion(address, regions, regionsCount);

	if (!region) {
		return nullptr;
	}

	uintptr_t addressEnd = address + size;

	// Overflow prevention.
	if (region->baseAddress > addressEnd) {
		return nullptr;
	}

	if (region->baseAddress + (region->pageCount << PAGE_BITS) < addressEnd) {
		return nullptr;
	}

	region->lock++;
	return region;
}

void VMM::UnlockRegion(VMMRegion *region) {
	if (!region) return;

	lock.Acquire();
	Defer(lock.Release());

	if (!region->lock) {
		KernelPanic("VMM::UnlockRegion - Region not locked.\n");
	}

	region->lock--;
}

bool VMM::HandlePageFault(uintptr_t address) {
	lock.AssertLocked();

	uintptr_t page = address & ~(PAGE_SIZE - 1);
	VMMRegion *region = FindRegion(address, lookupRegions, lookupRegionsCount);

	if (region) {

		if (region->type == vmmRegionShared) {
			SharedMemoryRegion *_region = (SharedMemoryRegion *) region->object;
			_region->mutex.Acquire();
		}

		virtualAddressSpace.lock.Acquire();
		bool result = HandlePageFaultInRegion(page, region);
		virtualAddressSpace.lock.Release();

		if (region->type == vmmRegionShared) {
			SharedMemoryRegion *_region = (SharedMemoryRegion *) region->object;
			_region->mutex.Release();
		}

		return result;
	} else return false;
}

#ifdef ARCH_X86_64
bool HandlePageFault(uintptr_t page) {
	// FFFF_8000_0000_0000 -> FFFF_8100_0000_0000	kernel image
	// FFFF_8E00_0000_0000 -> FFFF_8F00_0000_0000	kernel heap
	// FFFF_8F00_0000_0000 -> FFFF_9000_0000_0000	kernel VMM
	// FFFF_9000_0000_0000 -> FFFF_F000_0000_0000	kernel virtual address space
	// FFFF_F000_0000_0000 -> FFFF_FF00_0000_0000	unused
	// FFFF_FF00_0000_0000 -> FFFF_FF01_0000_0000	identity mapped 4GB
	// 	TODO Think about whether I really want this ^^^ (it won't really work on 32-bit processors)
	// FFFF_FF01_0000_0000 -> FFFF_FF80_0000_0000	unused
	// FFFF_FF80_0000_0000 -> FFFF_FFFF_FFFF_FFFF	paging tables

	{
		VirtualAddressSpace *virtualAddressSpace;

		if (page < 0x0000800000000000) {
			virtualAddressSpace = &GetCurrentThread()->process->vmm->virtualAddressSpace;
		} else {
			virtualAddressSpace = &kernelVMM.virtualAddressSpace;
		}

		if (virtualAddressSpace->Get(page, true)) {
			// If the processor "remembers" a non-present page then we'll get a page fault here.
			// ...so invalidate the page.
			ProcessorInvalidatePage(page);
			return true;
		}
	}

	if (page >= 0xFFFF810000000000 && page < 0xFFFF900000000000) {
		kernelVMM.virtualAddressSpace.lock.Acquire();
		Defer(kernelVMM.virtualAddressSpace.lock.Release());

		kernelVMM.virtualAddressSpace.Map(pmm.AllocatePage(), page, true);
		ZeroMemory((void *) (page & ~(PAGE_SIZE - 1)), PAGE_SIZE);
		return true;
	} else if (page >= 0xFFFF900000000000 && page < 0xFFFFF00000000000) {
		if (GetLocalStorage()->spinlockCount)
			KernelPanic("HandlePageFault - Page fault occurred in critical section.\n");

		ProcessorEnableInterrupts();

		kernelVMM.lock.Acquire();
		Defer(kernelVMM.lock.Release());

		bool result = kernelVMM.HandlePageFault(page);

		ProcessorDisableInterrupts();
		return result;
	} else if (page >= 0xFFFFFF0000000000 && page < 0xFFFFFF0100000000) {
		kernelVMM.virtualAddressSpace.lock.Acquire();
		Defer(kernelVMM.virtualAddressSpace.lock.Release());

		kernelVMM.virtualAddressSpace.Map(page - 0xFFFFFF0000000000, page, 0 /* we use this for MMIO, so don't do caching! */);
		return true;
	} else if (page < 0x0000800000000000) {
		Process *currentProcess = GetCurrentThread()->process;
		VMM *vmm = currentProcess->vmm;

		if (GetLocalStorage()->spinlockCount)
			KernelPanic("HandlePageFault - Page fault occurred in critical section.\n");

		ProcessorEnableInterrupts();

		vmm->lock.Acquire();
		Defer(vmm->lock.Release());

		bool result = vmm->HandlePageFault(page);

		ProcessorDisableInterrupts();
		return result;
	} else {
		return false;
	}
}
#endif

uintptr_t PMM::AllocateContiguous64KB() {
	lock.Acquire();
	Defer(lock.Release());

	for (uintptr_t i = 0; i < bitsetGroups; i++) {
		if (bitsetGroupUsage[i] >= 16) {
			for (uintptr_t j = 0; j < BITSET_GROUP_PAGES; j += 16) {
				uintptr_t page = i * BITSET_GROUP_PAGES + j;

				if (((uint16_t *) bitsetPageUsage)[page >> 4] == (uint16_t) (-1)) {
					uintptr_t address = page << PAGE_BITS;

					((uint16_t *) bitsetPageUsage)[page >> 4] = 0;
					bitsetGroupUsage[i] -= 16;

					return address;
				}
			}
		}
	}

	return 0;
}

uintptr_t PMM::AllocateContiguous128KB() {
	lock.Acquire();
	Defer(lock.Release());

	for (uintptr_t i = 0; i < bitsetGroups; i++) {
		if (bitsetGroupUsage[i] >= 32) {
			for (uintptr_t j = 0; j < BITSET_GROUP_PAGES; j += 32) {
				uintptr_t page = i * BITSET_GROUP_PAGES + j;

				if (bitsetPageUsage[page >> 5] == (uint32_t) (-1)) {
					uintptr_t address = page << PAGE_BITS;

					bitsetPageUsage[page >> 5] = 0;
					bitsetGroupUsage[i] -= 32;

					return address;
				}
			}
		}
	}

	return 0;
}

uintptr_t PMM::AllocatePage() {
	lock.Acquire();
	Defer(lock.Release());

	pagesAllocated++;

	if (pageStackIndex) {
		returnFromStack:
		pageStackIndex--;
		uintptr_t address = pageStack[pageStackIndex];
		return address;
	} else if (physicalMemoryRegionsPagesCount) {
		uintptr_t i = physicalMemoryRegionsIndex;

		while (!physicalMemoryRegions[i].pageCount) {
			i++;

			if (i == physicalMemoryRegionsCount) {
				KernelPanic("PMM::AllocatePhysicalPage - expected more pages in physical regions");
			}
		}

		uintptr_t addingPageCount = PMM_PAGE_STACK_SIZE;

		PhysicalMemoryRegion *region = physicalMemoryRegions + i;

		if (region->pageCount < addingPageCount) {
			addingPageCount = region->pageCount;
		}

		for (uintptr_t i = 0; i < addingPageCount; i++) {
			pageStack[pageStackIndex++] = region->baseAddress;
			region->baseAddress += PAGE_SIZE;
			region->pageCount--;
			physicalMemoryRegionsPagesCount--;
		}

		physicalMemoryRegionsIndex = i;
		goto returnFromStack;
	} else {
		for (uintptr_t i = 0; i < bitsetGroups; i++) {
			if (bitsetGroupUsage[i]) {
				for (uintptr_t j = 0; j < BITSET_GROUP_PAGES && bitsetGroupUsage[i] && pageStackIndex != PMM_PAGE_STACK_SIZE; j++) {
					uintptr_t page = i * BITSET_GROUP_PAGES + j;

					if (bitsetPageUsage[page >> 5] & (1 << (page & 31))) {
						uintptr_t address = page << PAGE_BITS;

						pageStack[pageStackIndex] = address;
						pageStackIndex++;

						bitsetPageUsage[page >> 5] &= ~(1 << (page & 31));
						bitsetGroupUsage[i]--;
					}
				}

				goto returnFromStack;
			}
		}
	}

	KernelPanic("PMM::AllocatePage - Out of physical memory\n");
	return 0; 
}

void PMM::Initialise() {
	physicalMemoryHighest += PAGE_SIZE << 3;

	bitsetPages = physicalMemoryHighest >> PAGE_BITS;
	bitsetGroups = bitsetPages / BITSET_GROUP_PAGES + 1;

	bitsetPageUsage = (uint32_t *) kernelVMM.Allocate((bitsetPages >> 3) + (bitsetGroups * 2), vmmMapAll);
	bitsetGroupUsage = (uint16_t *) bitsetPageUsage + (bitsetPages >> 4);

	while (physicalMemoryRegionsPagesCount) {
		startPageCount++;

		uintptr_t page = AllocatePage();
		lock.Acquire();
		FreePage(page, true);
		lock.Release();
	}

	for (uintptr_t i = 0x100; i < 0x200; i++) {
		if (PAGE_TABLE_L4[i] == 0) {
			PAGE_TABLE_L4[i] = pmm.AllocatePage() | 0x103;
			ZeroMemory((void *) (PAGE_TABLE_L3 + (i << ENTRIES_PER_PAGE_TABLE_BITS)), PAGE_SIZE);
		}
	}
}

void PMM::FreePage(uintptr_t address, bool bypassStack) {
	lock.AssertLocked();

	pagesAllocated--;

	if (!address) {
		KernelPanic("PMM::FreePage - address was 0\n");
	}

	if (pageStackIndex == PMM_PAGE_STACK_SIZE || bypassStack) {
		bitsetPageUsage[address >> (PAGE_BITS + 5)] |= 1 << ((address >> PAGE_BITS) & 31);
		bitsetGroupUsage[(address >> PAGE_BITS) / BITSET_GROUP_PAGES]++;
	} else {
		pageStack[pageStackIndex] = address;
		pageStackIndex++;
	}
}

#ifdef ARCH_X86_64
uintptr_t VirtualAddressSpace::Get(uintptr_t virtualAddress, bool force) {
	if (!force) {
		lock.AssertLocked();
	}

	virtualAddress  &= 0x0000FFFFFFFFF000;

	uintptr_t indexL1 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 0);
	uintptr_t indexL2 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 1);
	uintptr_t indexL3 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 2);
	uintptr_t indexL4 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 3);

	if ((PAGE_TABLE_L4[indexL4] & 1) == 0) {
		return 0;
	}

	if ((PAGE_TABLE_L3[indexL3] & 1) == 0) {
		return 0;
	}

	if ((PAGE_TABLE_L2[indexL2] & 1) == 0) {
		return 0;
	}

	uintptr_t physicalAddress = PAGE_TABLE_L1[indexL1];

	if (physicalAddress & 1) {
		return physicalAddress & 0xFFFFFFFFFFFFF000;
	} else {
		return 0;
	}
}

void VirtualAddressSpace::Remove(uintptr_t _virtualAddress, size_t pageCount) {
	lock.AssertLocked();

	uintptr_t virtualAddressU = _virtualAddress;
	_virtualAddress &= 0x0000FFFFFFFFF000;

	for (uintptr_t i = 0; i < pageCount; i++) {
		uintptr_t virtualAddress = (i << PAGE_BITS) + _virtualAddress;

		if (Get(virtualAddress)) {
			uintptr_t indexL1 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 0);
			PAGE_TABLE_L1[indexL1] = 0;
			ProcessorInvalidatePage((i << PAGE_BITS) + virtualAddressU);
		}
	}

	// TODO Only send the IPI to the processors that are actually executing threads with the virtual address space.
	// 	Currently we only support the kernel's virtual address space, so this'll apply to all processors.
	// 	If we use Intel's PCID then we may have to send this to all processors anyway.
	// 	And we'll probably also have to be careful with shared memory regions.
	//	...actually I think we might not bother doing this.
	if (scheduler.processors > 1) {
		ipiLock.Acquire();
		tlbShootdownVirtualAddress = _virtualAddress;
		tlbShootdownPageCount = pageCount;
		tlbShootdownRemainingProcessors = scheduler.processors - 1;
		ProcessorSendIPI(TLB_SHOOTDOWN_IPI);
		while (tlbShootdownRemainingProcessors);
		ipiLock.Release();
	}
}

void VirtualAddressSpace::Map(uintptr_t physicalAddress, uintptr_t virtualAddress, unsigned flags) {
	// TODO Use the no-execute bit.
	// TODO Support read-only pages.

	lock.AssertLocked();

	if ((virtualAddress & 0xFFFF000000000000) == 0
			&& ProcessorReadCR3() != cr3) {
		KernelPanic("VirtualAddressSpace::Map - Attempt to map page into other address space.\n");
	}

	physicalAddress &= 0xFFFFFFFFFFFFF000;
	virtualAddress  &= 0x0000FFFFFFFFF000;

	uintptr_t indexL4 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 3);
	uintptr_t indexL3 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 2);
	uintptr_t indexL2 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 1);
	uintptr_t indexL1 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 0);

	if ((PAGE_TABLE_L4[indexL4] & 1) == 0) {
		PAGE_TABLE_L4[indexL4] = pmm.AllocatePage() | 7;
		ZeroMemory((void *) ((uintptr_t) (PAGE_TABLE_L3 + indexL3) & ~(PAGE_SIZE - 1)), PAGE_SIZE);
	}

	if ((PAGE_TABLE_L3[indexL3] & 1) == 0) {
		PAGE_TABLE_L3[indexL3] = pmm.AllocatePage() | 7;
		ZeroMemory((void *) ((uintptr_t) (PAGE_TABLE_L2 + indexL2) & ~(PAGE_SIZE - 1)), PAGE_SIZE);
	}

	if ((PAGE_TABLE_L2[indexL2] & 1) == 0) {
		PAGE_TABLE_L2[indexL2] = pmm.AllocatePage() | 7;
		ZeroMemory((void *) ((uintptr_t) (PAGE_TABLE_L1 + indexL1) & ~(PAGE_SIZE - 1)), PAGE_SIZE);
	}

	if (PAGE_TABLE_L1[indexL1] & 1) {
		KernelPanic("VirtualAddressSpace::Map - Attempt to map to %x address %x that has already been mapped in address space %x to %x.\n", 
				physicalAddress, virtualAddress, ProcessorReadCR3(), PAGE_TABLE_L1[indexL1] & (~(PAGE_SIZE - 1)));
	}

	uintptr_t value = physicalAddress | (userland ? 7 : 0x103) | (flags & VMM_REGION_FLAG_CACHABLE ? 0 : 24);
	PAGE_TABLE_L1[indexL1] = value;
}
#endif

#if USE_OLD_POOLS 
void Pool::Initialise(size_t _elementSize) {
	if (elementSize) {
		KernelPanic("Pool::Initialise - Attempt to reinitilise a pool.\n");
	}

	elementSize = _elementSize + sizeof(PoolGroup *); // Each element starts with a pointer to the its parent PoolGroup.
	elementsPerGroup = sizeof(PoolGroup) / elementSize;
	bitsetLength = ((elementsPerGroup / 8) & ~(POOL_GROUP_PADDING - 1)) + POOL_GROUP_PADDING; // Ensure that it is aligned correctly.
	elementsPerGroup -= ((bitsetLength + POOL_GROUP_PADDING) / elementSize) + 1;

	if (elementsPerGroup < 4) {
		KernelPanic("Pool::Initialise - elementsPerGroups too low for size %d\n", elementSize);
	}
}

void *Pool::Add() {
	lock.Acquire();
	Defer(lock.Release());

	if (concurrentModificationCheck) {
		KernelPanic("Pool::Add - Concurrent modification of pool.\n");
	}

	concurrentModificationCheck = true;
	Defer(concurrentModificationCheck = false);

	if (!elementSize) {
		KernelPanic("Pool::Add - Attempt to use a pool before initilisation.\n");
	}

	if (stackCount) {
		addFromStack:
		void *item = stack[--stackCount];
		return item;
	} else if (availableGroups) {
		findElementsInGroups:

		PoolGroup *group = availableGroups;
		bool remove = true;

		for (uintptr_t i = 0; i < elementsPerGroup; i++) {
			if ((group->data[i >> 3] & (1 << (i & 7))) == 0) {
				group->data[i >> 3] |= (1 << (i & 7));
				void **element = (void **) (group->data + bitsetLength + i * elementSize);
				element[0] = group; // Save a pointer to the group for fast deallocation.
				stack[stackCount++] = element + 1;
				group->elementsUsed++;

				// If the stack's full but there were more elements in this group,
				// then don't remove it from the availableGroups list.
				if (stackCount == POOL_STACK_SIZE && group->elementsUsed != elementsPerGroup) {
					remove = false;
					break;
				}
			}
		}

		if (remove) {
			availableGroups = group->next;

			group->next = fullGroups;
			fullGroups = group;

			group->reference = &fullGroups;
			if (group->next) group->next->reference = &group->next;
		}

		goto addFromStack;
	} else {
		availableGroups = (PoolGroup *) kernelVMM.Allocate(sizeof(PoolGroup));
		availableGroups->reference = &availableGroups;

		goto findElementsInGroups;
	}
}

void Pool::Remove(void *element) {
	lock.Acquire();
	Defer(lock.Release());

	if (!elementSize) {
		KernelPanic("Pool::Remove - Attempt to use a pool before initilisation.\n");
	}

	// Make sure that the memory is cleared before we try to use it again.
	ZeroMemory((void **) element - 1, elementSize); 

#if 1
	if (stackCount != POOL_STACK_SIZE) {
		stack[stackCount++] = element;
		return;
	} else {
#else
	{
#endif
		element = (void **) element - 1;
		PoolGroup *group = (PoolGroup *) *((void **) element);
		bool wasFull = group->elementsUsed == elementsPerGroup;
		group->elementsUsed--;

		uintptr_t indexInGroup = ((uintptr_t) element - (uintptr_t) group - bitsetLength - POOL_GROUP_PADDING) / elementSize;
		group->data[indexInGroup >> 3] &= ~(1 << (indexInGroup & 7));

		// If the group was previously full,
		// then we will want to add the group to the available groups list.
		// If it wasn't previously full,
		// then the group is already in the available groups list.
		if (wasFull) {
			*group->reference = group->next;

			group->reference = &availableGroups;
			group->next = availableGroups;

			if (availableGroups) availableGroups->reference = &group->next;
			availableGroups = group;
		}

		return;
	}
}
#else
void Pool::Initialise(size_t _elementSize) {
	elementSize = _elementSize;
}

void *Pool::Add() {
	if (!elementSize) KernelPanic("Pool::Add - Pool uninitialised.\n");
	void *address = OSHeapAllocate(elementSize, true);
	return address;
}

void Pool::Remove(void *address) {
	OSHeapFree(address, elementSize);
}
#endif

void *_ArrayAdd(void **array, size_t &arrayCount, size_t &arrayAllocated, void *item, size_t itemSize) {
	if (arrayCount == arrayAllocated) {
		if (arrayAllocated) {
			arrayAllocated *= 2;
		} else {
			arrayAllocated = 16;
		}
		
		void *old = *array;
		void *replacement = kernelVMM.Allocate(itemSize * arrayAllocated);
		CopyMemory(replacement, old, arrayCount * itemSize);
		*array = replacement;
		kernelVMM.Free(old);
	} 

	void *position = (uint8_t *) *array + itemSize * arrayCount;
	CopyMemory(position, item, itemSize);

	arrayCount++;
	return position;
}

#define ArrayAdd(_array, _item) _ArrayAdd((void **) &(_array), (_array ## Count), (_array ## Allocated), &(_item), sizeof(_item))

SharedMemoryRegion *SharedMemoryManager::CreateSharedMemory(size_t sizeBytes, char *name, size_t nameLength) {
	mutex.Acquire();
	Defer(mutex.Release());

	NamedSharedMemoryRegion *namedRegion = nullptr;

	if (name) {
		if (!nameLength) {
			KernelPanic("SharedMemoryManager::CreateSharedMemory - Invalid name length.\n");
		}

		for (uintptr_t i = 0; i < namedSharedMemoryRegionsCount; i++) {
			if (namedSharedMemoryRegions[i].nameLength == nameLength 
					&& !CompareBytes(namedSharedMemoryRegions[i].name, name, nameLength)) {
				// Name overlap.
				return nullptr;
			}
		}

		NamedSharedMemoryRegion region = {};
		region.nameLength = nameLength;
		CopyMemory(region.name, name, nameLength);
		namedRegion = (NamedSharedMemoryRegion *) ArrayAdd(namedSharedMemoryRegions, region);
	}

	size_t pages = sizeBytes / PAGE_SIZE;
	if (sizeBytes & (PAGE_SIZE - 1)) pages++;

	// Allocate enough space for the SharedMemoryRegion itself,
	// and the list of physical addresses.
	SharedMemoryRegion *region = (SharedMemoryRegion *) OSHeapAllocate(pages * sizeof(void *) + sizeof(SharedMemoryRegion), true);
	region->sizeBytes = sizeBytes;

	if (namedRegion) {
		region->named = true;
		region->handles = 2; // Named regions must always be present.
		namedRegion->region = region;
	} else {
		region->named = false;
		region->handles = 1;
	}

	return region;
}

SharedMemoryRegion *SharedMemoryManager::LookupSharedMemory(char *name, size_t nameLength) {
	mutex.Acquire();
	Defer(mutex.Release());

	for (uintptr_t i = 0; i < namedSharedMemoryRegionsCount; i++) {
		if (namedSharedMemoryRegions[i].nameLength == nameLength 
				&& !CompareBytes(namedSharedMemoryRegions[i].name, name, nameLength)) {
			return namedSharedMemoryRegions[i].region;
		}
	}

	return nullptr;
}

void SharedMemoryManager::DestroySharedMemory(SharedMemoryRegion *region) {
	if (region->handles) {
		KernelPanic("SharedMemoryManager::DestroySharedMemory - Region has handles.\n");
	}
	
	if (region->named) {
		KernelPanic("SharedMemoryManager::DestroySharedMemory - Cannot destroy named regions (yet).\n");
	}

	size_t pages = region->sizeBytes / PAGE_SIZE;
	if (region->sizeBytes & (PAGE_SIZE - 1)) pages++;

	uintptr_t *addresses = (uintptr_t *) (region + 1);

	// KernelLog(LOG_VERBOSE, "Freeing shared memory region....\n");

	pmm.lock.Acquire();
	for (uintptr_t i = 0; i < pages; i++) {
		uintptr_t address = addresses[i];
		pmm.FreePage(address);
	}
	pmm.lock.Release();

	OSHeapFree(region);
}

#endif
