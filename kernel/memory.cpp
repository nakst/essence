#ifndef IMPLEMENTATION

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
	uintptr_t AllocatePage(bool zeroPage); 
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

	Mutex lock; 
};

extern PMM pmm;

enum VMMRegionType {
	VMM_REGION_FREE,
	VMM_REGION_STANDARD, // Allocate and zero on write
	VMM_REGION_PHYSICAL,
	VMM_REGION_SHARED,  
	VMM_REGION_COPY, // Copy the mappings from another address; should only be used by IO manager.
	VMM_REGION_HANDLE_TABLE, // A special mapping type for per-process handle tables.
};

// TODO Why are these in lowercase?!

enum VMMMapPolicy {
	VMM_MAP_LAZY,
	VMM_MAP_ALL, // These are not stored in the region lookup array
	VMM_MAP_STRICT,
};

#define VMM_REGION_FLAG_NOT_CACHABLE (0)
#define VMM_REGION_FLAG_CACHABLE     (1)
#define VMM_REGION_FLAG_SUPERVISOR   (32)
#define VMM_REGION_FLAG_READ_ONLY    (64)
#define VMM_REGION_FLAG_OVERWRITABLE (128)

struct VMMRegion {
	bool used;

	uintptr_t baseAddress;
	size_t pageCount;

	// Incremented when a syscall is using the region.
	// To remove the region this must be 0.
	// Must be modified using __sync_fetch_and_*** sinec the VMM's lock *doesn't* need to be acquired.
	volatile unsigned lock;

	unsigned flags;
	LinkedItem<VMMRegion> item; // Only used by some objects.

	// Fields used to distinguish regions in lookupRegions
	uintptr_t offset; // TODO Since we only merge standard and free regions, can we remove offset and object from the comparison?
	void *object;
	VMMRegionType type;
	VMMMapPolicy mapPolicy;
};

struct VirtualAddressSpace {
	void Map(uintptr_t physicalAddress,
		 uintptr_t virtualAddress,
		 unsigned flags);
	void Remove(uintptr_t virtualAddress, size_t pageCount);
	uintptr_t Get(uintptr_t virtualAddress, bool force = false);
	
	bool userland;
	Mutex lock;

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

struct VMMRegionReference {
	// Because the regions array is dynamic, 
	// we have to store pointers to VMMRegion that aren't under the VMM's lock as VMM/index pairs.
	struct VMM *vmm;
	uintptr_t index;
};

struct VMM {
	void Initialise();
	void Destroy(); 

	void *Allocate(const char *reason, size_t size, VMMMapPolicy mapPolicy = VMM_MAP_LAZY, VMMRegionType type = VMM_REGION_STANDARD, uintptr_t offset = 0, unsigned flags = VMM_REGION_FLAG_CACHABLE, void *object = nullptr);
	OSError Free(void *address, void **object = nullptr, VMMRegionType *type = nullptr, bool skipVirtualAddressSpaceUpdate = false);
	bool HandlePageFault(uintptr_t address, size_t limit = 0, bool lookupRegionsOnly = true, struct CacheBlockFault *fault = nullptr);
	VMMRegionReference FindAndLockRegion(uintptr_t address, size_t size);
	void UnlockRegion(VMMRegionReference reference);

	void LogRegions();
	void LogRegionMappings(uintptr_t address);

	VirtualAddressSpace virtualAddressSpace;
	Mutex lock; 

	bool AddRegion(uintptr_t baseAddress, size_t pageCount, uintptr_t offset, VMMRegionType type, VMMMapPolicy mapPolicy, unsigned flags, void *object);
	uintptr_t FindEmptySpaceInRegionArray(VMMRegion *region, VMMRegion *&array, size_t &arrayAllocated);
	bool HandlePageFaultInRegion(uintptr_t page, VMMRegion *region, size_t limit = 0, struct CacheBlockFault *fault = nullptr);
	VMMRegion *FindRegion(uintptr_t address, VMMRegion *array, size_t arrayAllocated);
	void MergeIdenticalAdjacentRegions(VMMRegion *region, VMMRegion *array, size_t arrayAllocated);
	void SplitRegion(VMMRegion *region, uintptr_t address, bool keepAbove, VMMRegion *array, size_t &arrayAllocated);

	// This contains the canonical "split" regions.
	VMMRegion *regions;
	size_t regionsAllocated;

	// This contains the regions that can be used for working out how to resolve a page fault.
	// i.e. If 2 regions are identical then they will be merged in this list.
	VMMRegion *lookupRegions;
	size_t lookupRegionsAllocated;

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
	struct Node *file;

	bool named : 1;
	bool big : 1;

	// If big = false,
	// what follows is a list of physical addresses,
	// that contain the memory in the region.

	// If big = true,
	// what follows is a list of virtual addresses,
	// that are pointers to lists of physical addresses,
	// each covering 256MB.
#define BIG_SHARED_MEMORY (256 * 1024 * 1024)

#define SHARED_ADDRESS_PRESENT (1)
#define SHARED_ADDRESS_READING (2)

	// At the end of a physical address listing,
	// are the reading events for the cache blocks,
	// if file is not nullptr.
};

struct NamedSharedMemoryRegion {
	SharedMemoryRegion *region;
	char name[OS_SHARED_MEMORY_NAME_MAX_LENGTH];
	size_t nameLength;
};

struct CacheBlockFault {
	// When we fault in a shared memory region that contains cache blocks for a file,
	// we need to tell the rest of the page fault handling code that we need to read in the file.

	bool Handle();

#define CACHE_BLOCK_FAULT_NONE (0)
#define CACHE_BLOCK_FAULT_READ (1)
	int type;
	
	VMM *vmm;
	uintptr_t regionIndex;

#if 0
	uintptr_t offset;
	uintptr_t *physicalAddresses;
#endif
};

struct SharedMemoryManager {
	SharedMemoryRegion *CreateSharedMemory(size_t sizeBytes, char *name = nullptr, size_t nameLength = 0, Node *file = nullptr);
	SharedMemoryRegion *LookupSharedMemory(char *name, size_t nameLength);
	void DestroySharedMemory(SharedMemoryRegion *region);

	NamedSharedMemoryRegion *namedSharedMemoryRegions;
	size_t namedSharedMemoryRegionsCount, namedSharedMemoryRegionsAllocated;

	Mutex mutex;
};

SharedMemoryManager sharedMemoryManager;

struct Pool {
	void Initialise(size_t _elementSize);
	void *Add(); // Aligned to the size of a pointer.
	void Remove(void *element);

	size_t elementSize;

#define POOL_CACHE_COUNT (16)
	void *cache[POOL_CACHE_COUNT];
	size_t cacheEntries;
	Mutex mutex;
};

#define ZERO_MEMORY_REGION_PAGES (16)
void ZeroPhysicalMemory(uintptr_t page, size_t pageCount);
void *zeroMemoryRegion;

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
		AddRegion(0xFFFF900000000000, 0x600000000000 >> PAGE_BITS /* 96TiB */, 0, VMM_REGION_FREE, VMM_MAP_LAZY, true, nullptr);

		virtualAddressSpace.cr3 = ProcessorReadCR3();
#endif
	} else {
		virtualAddressSpace.userland = true;

		// Initialise the virtual address space for a new process.
#ifdef ARCH_X86_64
		AddRegion(0x100000000000, 0x600000000000 >> PAGE_BITS /* 96TiB */, 0, VMM_REGION_FREE, VMM_MAP_LAZY, true, nullptr);

		virtualAddressSpace.cr3 = pmm.AllocatePage(true);
		// KernelLog(LOG_INFO, "cr3 = %x\n", virtualAddressSpace.cr3);
		pageTable = (uint64_t *) kernelVMM.Allocate("VMM", PAGE_SIZE, VMM_MAP_ALL, VMM_REGION_PHYSICAL, (uintptr_t) virtualAddressSpace.cr3);
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
		for (uintptr_t i = 0; i < regionsAllocated; i++) {
			VMMRegion *region = regions + i;

			if (region->used && region->type != VMM_REGION_FREE) {
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

#if ARCH_X86_64
	scheduler.lock.Acquire();
	RegisterAsyncTask(CleanupVirtualAddressSpace, (void *) virtualAddressSpace.cr3, kernelProcess, true);
	scheduler.lock.Release();
#endif

	// KernelLog(LOG_VERBOSE, "VMM destroyed,\n");
}

uintptr_t VMM::FindEmptySpaceInRegionArray(VMMRegion *region, VMMRegion *&array, size_t &arrayAllocated) {
	uintptr_t regionIndex = -1;
	retry:;

	for (uintptr_t i = 0; i < arrayAllocated; i++) {
		if (!array[i].used) {
			regionIndex = i;
			break;
		}
	}

	if (regionIndex == (uintptr_t) -1) {
		if (this == &kernelVMM) {
			KernelPanic("VMM::FindEmptySpaceInRegionArray - Maximum kernel VMM regions (%d) exceeded\n", arrayAllocated);
		} else {
			if (arrayAllocated) {
				size_t oldAllocated = arrayAllocated;
				arrayAllocated = oldAllocated * 2;
				VMMRegion *old = array;
				array = (VMMRegion *) kernelVMM.Allocate("VMM", arrayAllocated * sizeof(VMMRegion), VMM_MAP_ALL);
				CopyMemory(array, old, oldAllocated * sizeof(VMMRegion));
				kernelVMM.Free(old);
			} else {
				arrayAllocated = 256;
				array = (VMMRegion *) kernelVMM.Allocate("VMM", arrayAllocated * sizeof(VMMRegion), VMM_MAP_ALL);
			}
		}

		goto retry;
	}

	array[regionIndex] = *region;
	array[regionIndex].used = true;

	return regionIndex;
}

bool VMM::AddRegion(uintptr_t baseAddress, size_t pageCount, uintptr_t offset, VMMRegionType type, VMMMapPolicy mapPolicy, unsigned flags, void *object) {
	lock.AssertLocked();

	if (FindRegion(baseAddress, regions, regionsAllocated)) {
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

	if (type != VMM_REGION_FREE) {
		allocatedVirtualMemory += pageCount * PAGE_SIZE;
	}

	// TODO This seems to be returning overlapping regions?? See ../todo_list.txt for repro.
	uintptr_t regionIndex = FindEmptySpaceInRegionArray(&region, regions, regionsAllocated);

	{
		VMMRegion *region = regions + regionIndex;
		region->item.thisItem = region;

		// Print("Created region %x\n", region);
	}

	if (type != VMM_REGION_FREE && mapPolicy != VMM_MAP_ALL) {
		uintptr_t lookupRegionIndex = FindEmptySpaceInRegionArray(&region, lookupRegions, lookupRegionsAllocated);
		MergeIdenticalAdjacentRegions(lookupRegions + lookupRegionIndex, lookupRegions, lookupRegionsAllocated);
	}

	if (type == VMM_REGION_SHARED) {
		SharedMemoryRegion *region = (SharedMemoryRegion *) object;
		region->mutex.Acquire();
		region->handles++; // Increment the number of handles to the shared memory region.
	}

	// Map the first few pages in to reduce the number of initial page faults.
	if (type != VMM_REGION_FREE && mapPolicy != VMM_MAP_STRICT) {
		HandlePageFaultInRegion(baseAddress, regions + regionIndex);
	}

	if (type == VMM_REGION_SHARED) {
		SharedMemoryRegion *region = (SharedMemoryRegion *) object;
		region->mutex.Release();
	}

	return true;
}

void *VMM::Allocate(const char *reason, size_t size, VMMMapPolicy mapPolicy, VMMRegionType type, uintptr_t offset, unsigned flags, void *object) {
	// Print("allocating memory, %d bytes, reason %z, ba = %d\n", size, reason, pmm.pagesAllocated * PAGE_SIZE);

	if (!size) return nullptr;
	size_t pageCount = (((offset & (PAGE_SIZE - 1)) + size - 1) >> PAGE_BITS) + 1;

	if (type == VMM_REGION_COPY) {
		if (this != &kernelVMM) {
			KernelPanic("VMM::Allocate - Attempt to allocate a copy region in a user VMM.\n");
		}

		Process *currentProcess = GetCurrentThread()->process;
#ifdef ARCH_X86_64
		VMM *vmm = offset >= 0xFFFF800000000000 ? &kernelVMM : currentProcess->vmm;
#endif

		{
			vmm->lock.Acquire();
			Defer(vmm->lock.Release());

			CacheBlockFault fault = {};
			if (!vmm->HandlePageFault(offset, pageCount, false, &fault) || !fault.Handle()) {
				KernelPanic("VMM::Allocate - Could not page fault copy source region %x in VMM %x.\n", offset, vmm);
			}
		}

		VMMRegionReference reference = vmm->FindAndLockRegion(offset, size);
		object = (void *) reference.index;

		if (!reference.vmm) {
			KernelLog(LOG_WARNING, "VMM::Allocate - Could not lock copy buffer.\n");
			return nullptr;
		}
	} else if (type == VMM_REGION_HANDLE_TABLE) {
		if (!(flags & VMM_REGION_FLAG_SUPERVISOR)) {
			KernelPanic("VMM::Allocate - Expected supervisor flag for handle table region.\n");
		} else if (mapPolicy != VMM_MAP_STRICT) {
			KernelPanic("VMM::Allocate - Expected strict map policy for handle table region.\n");
		}
	} else if (type == VMM_REGION_SHARED) {
		// Print("Mapping shared region of object %x\n", object);
	}

	lock.Acquire();
	Defer(lock.Release());

#if 0
	if (type == VMM_REGION_STANDARD) {
		KernelLog(LOG_VERBOSE, "VMM::Allocate - %z, reason = %z, this = %x, size = %d\n", this == &kernelVMM ? "KERN" : "user", reason, this, size);
	}
#else
	(void) reason;
#endif

	ValidateCurrentVMM(this);

	VMMRegion *region;
	uintptr_t baseAddress;

	{
		// TODO Should we look for the smallest/largest/first/last/etc. region?
		uintptr_t i = 0;

		for (; i < regionsAllocated; i++) {
			if (regions[i].used && regions[i].type == VMM_REGION_FREE
					&& regions[i].pageCount > pageCount) {
				break;
			}
		}

		if (i == regionsAllocated) {
			return nullptr;
		}

		region = regions + i;
		baseAddress = region->baseAddress;
		region->baseAddress += pageCount << PAGE_BITS;
		region->pageCount -= pageCount;
	}

	bool success = AddRegion(baseAddress, pageCount, offset & ~(PAGE_SIZE - 1), type, mapPolicy, flags, object);
	if (!success) return nullptr;
	void *address = (void *) (baseAddress + (offset & (PAGE_SIZE - 1))); 

	return address;
}

void VMM::MergeIdenticalAdjacentRegions(VMMRegion *region, VMMRegion *array, size_t arrayAllocated) {
	lock.AssertLocked();

	intptr_t remove1 = -1, remove2 = -1;

	for (uintptr_t i = 0; i < arrayAllocated && (remove1 != -1 || remove2 != 1); i++) {
		VMMRegion *r = array + i;

		if (!r->used) continue;
		if (r->type != region->type) continue;
		if (r == region) continue;

		if (r->type != VMM_REGION_STANDARD && r->type != VMM_REGION_FREE) {
			// We can only merge standard and free regions.
			continue;
		}

		if (r->type == VMM_REGION_STANDARD) {
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

	if (remove1 != -1) array[remove1].used = false;
	if (remove2 != -1) array[remove2].used = false;
}

void VMM::SplitRegion(VMMRegion *region, uintptr_t address, bool keepAbove, VMMRegion *array, size_t &arrayAllocated) {
	lock.AssertLocked();

	if (region->baseAddress == address) {
		return;
	}

	if (region->baseAddress + (region->pageCount << PAGE_BITS) == address) {
		return;
	}

	VMMRegion *newRegion = array + FindEmptySpaceInRegionArray(region, array, arrayAllocated);

	if (keepAbove) {
		VMMRegion *t = newRegion;
		newRegion = region;
		region = t;
	}

	newRegion->baseAddress = address;
	newRegion->pageCount -= (address - region->baseAddress) >> PAGE_BITS;
	region->pageCount -= newRegion->pageCount;
}

void CloseHandleToSharedMemoryRegionAfterVMMFree(void *argument) {
	CloseHandleToObject(argument, KERNEL_OBJECT_SHMEM);
}

OSError VMM::Free(void *address, void **object, VMMRegionType *type, bool skipVirtualAddressSpaceUpdate) {
	if (!address) {
		return OS_FATAL_ERROR_INVALID_MEMORY_REGION;
	}

	ValidateCurrentVMM(this);

	lock.Acquire();
	Defer(lock.Release());

	uintptr_t baseAddress = (uintptr_t) address;
	VMMRegion *region = FindRegion(baseAddress, regions, regionsAllocated);

	if (!region) {
		return OS_FATAL_ERROR_INVALID_MEMORY_REGION;
	} else if (region->lock) {
		return OS_FATAL_ERROR_MEMORY_REGION_LOCKED_BY_KERNEL;
	} else if (region->type == VMM_REGION_FREE) {
		KernelPanic("VMM::Free - Trying to free region that has already been freed.\n");
	}

	// Print("Freeing region %x\n", region);

	if (object) *object = region->object;
	if (type) *type = region->type;

	if (region->type == VMM_REGION_SHARED) {
		if (!region->object) KernelPanic("VMM::Free - Object from a freed shared memory region was null.\n");

		// Print("Freeing region for shared object %x\n", region->object);
		// SharedMemoryRegion *sharedRegion = (SharedMemoryRegion *) region->object;

		scheduler.lock.Acquire();
		// We have to do it like this because if this is the last handles then the we'll need to deallocate the shared memory,
		// which we'll need to the VMM's lock.
		RegisterAsyncTask(CloseHandleToSharedMemoryRegionAfterVMMFree, region->object, kernelProcess, true);
		scheduler.lock.Release();
	} else if (region->type == VMM_REGION_COPY) {
		VMMRegion *region2 = regions + (uintptr_t) region->object;
		__sync_fetch_and_sub(&region2->lock, 1);
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
					case VMM_REGION_STANDARD: {
						pmm.FreePage(mappedAddress);
						allocatedPhysicalMemory -= PAGE_SIZE;
					} break;

					case VMM_REGION_PHYSICAL: {
						// Do nothing.
					} break;

					case VMM_REGION_SHARED: {
						// This memory will be freed when the last handle to the shared memory region is closed.
					} break;

					case VMM_REGION_COPY: {
						// Do nothing.
						// KernelLog(LOG_VERBOSE, "VMM_REGION_COPY: Deallocate %x from %x.\n", mappedAddress, address);
					} break;

					case VMM_REGION_HANDLE_TABLE: {
						// Do nothing.
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

	if (mapPolicy != VMM_MAP_ALL) {
		VMMRegion *lookupRegion = FindRegion(baseAddress, lookupRegions, lookupRegionsAllocated);
		SplitRegion(lookupRegion, baseAddress, true, lookupRegions, lookupRegionsAllocated);
		SplitRegion(lookupRegion, baseAddress + (regionPageCount << PAGE_BITS), false, lookupRegions, lookupRegionsAllocated);
		lookupRegion->used = false;
	}

	region->type = VMM_REGION_FREE;
	MergeIdenticalAdjacentRegions(region, regions, regionsAllocated);

	return OS_SUCCESS;
}

bool VMM::HandlePageFaultInRegion(uintptr_t page, VMMRegion *region, size_t limit, CacheBlockFault *fault) {
	(void) fault;
	lock.AssertLocked();

	uintptr_t pageInRegion = (page - region->baseAddress) >> PAGE_BITS;

	uintptr_t postCount = 16;
	if (region->mapPolicy == VMM_MAP_STRICT) postCount = 1;
	if (limit) postCount = limit;

	for (uintptr_t address = page, i = 0; 
			(region->mapPolicy == VMM_MAP_ALL || i < postCount) && (pageInRegion + i < region->pageCount); 
			address += PAGE_SIZE, i++) {
		virtualAddressSpace.lock.Acquire();
		uintptr_t existingTranslation = virtualAddressSpace.Get(address);
		virtualAddressSpace.lock.Release();

		if (!existingTranslation) {
			// This page needs mapping.
		} else {
			// Skip the page.
			continue;
		}

		switch (region->type) {
			case VMM_REGION_STANDARD: {
				uintptr_t physicalPage = pmm.AllocatePage(true);
				allocatedPhysicalMemory += PAGE_SIZE;
				virtualAddressSpace.lock.Acquire();
				virtualAddressSpace.Map(physicalPage, address, region->flags);
				virtualAddressSpace.lock.Release();
			} break;

			case VMM_REGION_PHYSICAL: {
				virtualAddressSpace.lock.Acquire();
				virtualAddressSpace.Map(address - region->baseAddress + region->offset, address, region->flags);
				virtualAddressSpace.lock.Release();
			} break;

			case VMM_REGION_SHARED: {
				SharedMemoryRegion *sharedRegion = (SharedMemoryRegion *) region->object;
				sharedRegion->mutex.AssertLocked();

				uintptr_t *volatile addresses = (uintptr_t *) (sharedRegion + 1);
				uintptr_t index;

				uintptr_t base = (address - region->baseAddress + region->offset);

				if (sharedRegion->big) {
					uintptr_t group = base / BIG_SHARED_MEMORY;

					if (!addresses[group]) {
						addresses[group] = (uintptr_t) kernelVMM.Allocate("BigShare", BIG_SHARED_MEMORY / PAGE_SIZE * sizeof(uintptr_t), VMM_MAP_ALL);
					}

					addresses = (uintptr_t *) addresses[group];
					index = (base % BIG_SHARED_MEMORY) >> PAGE_BITS;
				} else {
					index = base >> PAGE_BITS;
				}

				if (addresses[index] & SHARED_ADDRESS_PRESENT) {
					virtualAddressSpace.lock.Acquire();
					virtualAddressSpace.Map(addresses[index], address, region->flags);
					virtualAddressSpace.lock.Release();
				} else {
					// NOTE Duplicated from above.
					uintptr_t physicalPage = pmm.AllocatePage(true);
					allocatedPhysicalMemory += PAGE_SIZE;
					virtualAddressSpace.lock.Acquire();
					virtualAddressSpace.Map(physicalPage, address, region->flags);
					virtualAddressSpace.lock.Release();

					// Store the address.
					addresses[index] = physicalPage | SHARED_ADDRESS_PRESENT;
				}
			} break;

			case VMM_REGION_COPY: {
				virtualAddressSpace.lock.Acquire();
				uintptr_t physicalAddress = virtualAddressSpace.Get(address - region->baseAddress + region->offset);
				// KernelLog(LOG_VERBOSE, "VMM_REGION_COPY: %x\n", physicalAddress);
				if (!physicalAddress) KernelPanic("VMM::HandlePageFaultInRegion - Copy region page (%x/%x) was unmapped.\n", address, address - region->baseAddress + region->offset);
				virtualAddressSpace.Map(physicalAddress, address, region->flags);
				virtualAddressSpace.lock.Release();
				// KernelLog(LOG_VERBOSE, "VMM_REGION_COPY: %x (from %x) -> %x\n", physicalAddress, address - region->baseAddress + region->offset, address);
			} break;

			case VMM_REGION_HANDLE_TABLE: {
				// This means we're trying to access an invalid handle.
				virtualAddressSpace.lock.Acquire();
				virtualAddressSpace.Map(emptyHandlePage, address, region->flags);
				virtualAddressSpace.lock.Release();
			} break;

			default: {
				return false;
			} break;
		}
	}

	return true;
}

VMMRegion *VMM::FindRegion(uintptr_t address, VMMRegion *array, size_t arrayAllocated) {
	lock.AssertLocked();

	for (uintptr_t i = 0; i < arrayAllocated; i++) {
		VMMRegion *region = array + i;

		if (region->used && region->baseAddress <= address
				&& region->baseAddress + (region->pageCount << PAGE_BITS) > address) {
			return region;
		}
	}

	return nullptr;
}

VMMRegionReference VMM::FindAndLockRegion(uintptr_t address, size_t size) {
	VMMRegionReference reference = {};

	if (!address) return reference;

	lock.Acquire();
	Defer(lock.Release());

	VMMRegion *region = FindRegion(address, regions, regionsAllocated);

	if (!region) {
		return reference;
	}

	uintptr_t addressEnd = address + size;

	// Overflow prevention.
	if (region->baseAddress > addressEnd) {
		return reference;
	}

	if (region->baseAddress + (region->pageCount << PAGE_BITS) < addressEnd) {
		return reference;
	}

	__sync_fetch_and_add(&region->lock, 1);

	reference.vmm = this;
	reference.index = region - regions;
	return reference;
}

void VMM::UnlockRegion(VMMRegionReference reference) {
	if (!reference.vmm) return;

	if (reference.vmm != this) {
		KernelPanic("VMM::UnlockRegion - Region reference VMM mismatch.\n");
	}

	lock.Acquire();
	Defer(lock.Release());

	VMMRegion *region = regions + reference.index;

	if (!region->lock) {
		KernelPanic("VMM::UnlockRegion - Region not locked.\n");
	}

	__sync_fetch_and_sub(&region->lock, 1);
}

bool VMM::HandlePageFault(uintptr_t address, size_t limit, bool lookupRegionsOnly, CacheBlockFault *fault) {
	lock.AssertLocked();

	uintptr_t page = address & ~(PAGE_SIZE - 1);
	VMMRegion *region;

	if (lookupRegionsOnly) {
		region = FindRegion(address, lookupRegions, lookupRegionsAllocated);
	} else {
		region = FindRegion(address, regions, regionsAllocated);
	}

	if (region) {
		if (region->type == VMM_REGION_SHARED) {
			SharedMemoryRegion *_region = (SharedMemoryRegion *) region->object;
			_region->mutex.Acquire();
			__sync_fetch_and_add(&region->lock, 1);
		}

		bool result = HandlePageFaultInRegion(page, region, limit, fault);

		if (region->type == VMM_REGION_SHARED) {
			SharedMemoryRegion *_region = (SharedMemoryRegion *) region->object;
			_region->mutex.Release();

			if (fault->type == CACHE_BLOCK_FAULT_NONE) {
				__sync_fetch_and_sub(&region->lock, 1);
			}
		}

		return result;
	} else {
#if 0
		Print("Could not find region, lookupRegionsOnly = %d\n", lookupRegionsOnly);
		for (uintptr_t i = 0; i < regionsCount; i++) {
			Print("region %d, from %x %d pages type %d\n", i, regions[i].baseAddress, regions[i].pageCount, regions[i].type);
		} 
#endif

		return false;
	}
}

bool CacheBlockFault::Handle() {
	bool result = true;
	if (type != CACHE_BLOCK_FAULT_READ) return result;

#if 0
	// This can't be closed because we have a lock on the region which has a handle to the shared memory object.
	SharedMemoryRegion *region = (SharedMemoryRegion *) vmmRegion->object;

	void *buffer = OSHeapAllocate(CACHE_BLOCK_SIZE, false);

	{
		IORequest *request = (IORequest *) ioRequestPool.Add();
		request->handles = 1;
		request->type = IO_REQUEST_READ;
		request->node = region->file;
		request->offset = offset;
		request->count = CACHE_BLOCK_SIZE;
		request->buffer = buffer;
		request->Start();
		request->complete.Wait(OS_WAIT_NO_TIMEOUT); // No mutexes acquired!! (so no deadlock...)
		OSError error = request->error;
		CloseHandleToObject(request, KERNEL_OBJECT_IO_REQUEST);
		result = error == OS_SUCCESS;
	}

	if (!result) goto frFail;

	region->mutex.Acquire(); 

	if (region->sizeBytes > offset) {
		for (uintptr_t i = 0; i < CACHE_BLOCK_SIZE / PAGE_SIZE; i++) {
			if (!(physicalAddresses[i] & SHARED_ADDRESS_READING)) {
				// Page fault collision! Another thread got here before us...
				goto pfCollision;
			}

			// Mark the cache block as no longer being read.
			physicalAddresses[i] &= ~SHARED_ADDRESS_READING;
		}

		// Copy in the data we read!
		void *destination = offset + (uint8_t *) region->file->cacheData;
		CopyMemory(destination, buffer, CACHE_BLOCK_SIZE);
	}

	pfCollision:;
	region->mutex.Release();
	frFail:;
	OSHeapFree(buffer);
#endif

	vmm->lock.Acquire();
	__sync_fetch_and_sub(&vmm->regions[regionIndex].lock, 1);
	vmm->lock.Release();

	return result;
}

#ifdef ARCH_X86_64
bool HandlePageFault(uintptr_t page) {
	// Print("HandlePageFault, %x\n", page);

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

	VMM *vmm;

	if (page >= 0xFFFF810000000000 && page < 0xFFFF900000000000) {
		uintptr_t frame = pmm.AllocatePage(false);

		kernelVMM.virtualAddressSpace.lock.Acquire();
		Defer(kernelVMM.virtualAddressSpace.lock.Release());

		kernelVMM.virtualAddressSpace.Map(frame, page, true);
		ZeroMemory((void *) (page & ~(PAGE_SIZE - 1)), PAGE_SIZE);

		return true;
	} else if (page >= 0xFFFF900000000000 && page < 0xFFFFF00000000000) {
		if (GetLocalStorage()->spinlockCount)
			KernelPanic("HandlePageFault - Page fault (%x) occurred in critical section.\n", page);

		vmm = &kernelVMM;
		goto normalFault;
	} else if (page >= 0xFFFFFF0000000000 && page < 0xFFFFFF0100000000) {
		kernelVMM.virtualAddressSpace.lock.Acquire();
		Defer(kernelVMM.virtualAddressSpace.lock.Release());

		kernelVMM.virtualAddressSpace.Map(page - 0xFFFFFF0000000000, page, 0 /* we use this for MMIO, so don't do caching! */);
		return true;
	} else if (page < 0x0000800000000000) {
		Process *currentProcess = GetCurrentThread()->process;

		if (GetLocalStorage()->spinlockCount)
			KernelPanic("HandlePageFault - Page fault (%x) occurred in critical section.\n", page);

		vmm = currentProcess->vmm;
		goto normalFault;
	} else {
		return false;
	}

	normalFault:;

	{
		CacheBlockFault fault = {};
		vmm->lock.Acquire();
		bool result = vmm->HandlePageFault(page, 0, true, &fault);
		vmm->lock.Release();
		if (!result) return false;
		result = fault.Handle();
		return result;
	}
}
#endif

uintptr_t PMM::AllocateContiguous64KB() {
	lock.Acquire();
	Defer(lock.Release());

	pagesAllocated += 16;

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

	pagesAllocated += 32;

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

uintptr_t PMM::AllocatePage(bool zeroPage) {
	lock.Acquire();

	pagesAllocated++;

	if (pageStackIndex) {
		returnFromStack:
		pageStackIndex--;
		uintptr_t address = pageStack[pageStackIndex];
		lock.Release();
		if (zeroPage) ZeroPhysicalMemory(address, 1);
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
	zeroMemoryRegion = kernelVMM.Allocate("ZeroMemReg", ZERO_MEMORY_REGION_PAGES * PAGE_SIZE, VMM_MAP_STRICT, 
			VMM_REGION_PHYSICAL, 0, VMM_REGION_FLAG_OVERWRITABLE | VMM_REGION_FLAG_CACHABLE, nullptr);

	physicalMemoryHighest += PAGE_SIZE << 3;

	bitsetPages = physicalMemoryHighest >> PAGE_BITS;
	bitsetGroups = bitsetPages / BITSET_GROUP_PAGES + 1;

	bitsetPageUsage = (uint32_t *) kernelVMM.Allocate("PMM", (bitsetPages >> 3) + (bitsetGroups * 2), VMM_MAP_ALL);
	bitsetGroupUsage = (uint16_t *) bitsetPageUsage + (bitsetPages >> 4);

	while (physicalMemoryRegionsPagesCount) {
		startPageCount++;

		uintptr_t page = AllocatePage(false);
		lock.Acquire();
		FreePage(page, true);
		lock.Release();
	}

	for (uintptr_t i = 0x100; i < 0x200; i++) {
		if (PAGE_TABLE_L4[i] == 0) {
			PAGE_TABLE_L4[i] = pmm.AllocatePage(true) | 3;
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

	// Print("Remove, %x -> %x\n", _virtualAddress, _virtualAddress + (pageCount << 12));

	uintptr_t virtualAddressU = _virtualAddress;
	_virtualAddress &= 0x0000FFFFFFFFF000;

	for (uintptr_t i = 0; i < pageCount; i++) {
		uintptr_t virtualAddress = (i << PAGE_BITS) + _virtualAddress;

		if (Get(virtualAddress)) {
			uintptr_t indexL1 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 0);
			PAGE_TABLE_L1[indexL1] = 0;

			uint64_t invalidateAddress = (i << PAGE_BITS) + virtualAddressU;

			ProcessorInvalidatePage(invalidateAddress);
		}
	}

	// TODO Only send the IPI to the processors that are actually executing threads with the virtual address space.
	// 	Currently we only support the kernel's virtual address space, so this'll apply to all processors.
	// 	If we use Intel's PCID then we may have to send this to all processors anyway.
	// 	And we'll probably also have to be careful with shared memory regions.
	//	...actually I think we might not bother doing this.
	if (scheduler.processors > 1) {
		ipiLock.Acquire();
		tlbShootdownVirtualAddress = virtualAddressU;
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

	// Print("Map, %x -> %x\n", virtualAddress, physicalAddress);

	uintptr_t oldVirtualAddress = virtualAddress;
	physicalAddress &= 0xFFFFFFFFFFFFF000;
	virtualAddress  &= 0x0000FFFFFFFFF000;

	uintptr_t indexL4 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 3);
	uintptr_t indexL3 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 2);
	uintptr_t indexL2 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 1);
	uintptr_t indexL1 = virtualAddress >> (PAGE_BITS + ENTRIES_PER_PAGE_TABLE_BITS * 0);

	if ((PAGE_TABLE_L4[indexL4] & 1) == 0) {
		PAGE_TABLE_L4[indexL4] = pmm.AllocatePage(false) | 7;
		ProcessorInvalidatePage((uintptr_t) (PAGE_TABLE_L3 + indexL3));
		ZeroMemory((void *) ((uintptr_t) (PAGE_TABLE_L3 + indexL3) & ~(PAGE_SIZE - 1)), PAGE_SIZE);
	}

	if ((PAGE_TABLE_L3[indexL3] & 1) == 0) {
		PAGE_TABLE_L3[indexL3] = pmm.AllocatePage(false) | 7;
		ProcessorInvalidatePage((uintptr_t) (PAGE_TABLE_L2 + indexL2));
		ZeroMemory((void *) ((uintptr_t) (PAGE_TABLE_L2 + indexL2) & ~(PAGE_SIZE - 1)), PAGE_SIZE);
	}

	if ((PAGE_TABLE_L2[indexL2] & 1) == 0) {
		PAGE_TABLE_L2[indexL2] = pmm.AllocatePage(false) | 7;
		ProcessorInvalidatePage((uintptr_t) (PAGE_TABLE_L1 + indexL1));
		ZeroMemory((void *) ((uintptr_t) (PAGE_TABLE_L1 + indexL1) & ~(PAGE_SIZE - 1)), PAGE_SIZE);
	}

	if ((PAGE_TABLE_L1[indexL1] & 1) && !(flags & VMM_REGION_FLAG_OVERWRITABLE)) {
		KernelPanic("VirtualAddressSpace::Map - Attempt to map to %x address %x that has already been mapped in address space %x to %x.\n", 
				physicalAddress, virtualAddress, ProcessorReadCR3(), PAGE_TABLE_L1[indexL1] & (~(PAGE_SIZE - 1)));
	}

	uintptr_t value = physicalAddress | ((userland && !(flags & VMM_REGION_FLAG_SUPERVISOR)) ? 7 : 0x103) | (flags & VMM_REGION_FLAG_CACHABLE ? 0 : 24);
	if (flags & VMM_REGION_FLAG_READ_ONLY) value &= ~2;
	PAGE_TABLE_L1[indexL1] = value;

	if (flags & VMM_REGION_FLAG_OVERWRITABLE) {
		ProcessorInvalidatePage(oldVirtualAddress);
	} else {
		// Not technically required, but helps to increase performance.
		ProcessorInvalidatePage(oldVirtualAddress);
	}
}
#endif

void Pool::Initialise(size_t _elementSize) {
	ZeroMemory(cache, sizeof(cache));
	elementSize = _elementSize;
}

void *Pool::Add() {
	if (!elementSize) KernelPanic("Pool::Add - Pool uninitialised.\n");

	mutex.Acquire();
	Defer(mutex.Release());

	void *address;

#if 1
	if (cacheEntries) {
		address = cache[--cacheEntries];
		ZeroMemory(address, elementSize);
	} else {
		address = OSHeapAllocate(elementSize, true);
	}
#else
	address = OSHeapAllocate(elementSize, true);
#endif

	return address;
}

void Pool::Remove(void *address) {
	mutex.Acquire();
	Defer(mutex.Release());

#if 1
	if (cacheEntries == POOL_CACHE_COUNT) {
		OSHeapFree(address, elementSize);
	} else {
		cache[cacheEntries++] = address;
	}
#else
	OSHeapFree(address, elementSize);
#endif
}

void *_ArrayAdd(void **array, size_t &arrayCount, size_t &arrayAllocated, void *item, size_t itemSize) {
	if (arrayCount == arrayAllocated) {
		if (arrayAllocated) {
			arrayAllocated *= 2;
		} else {
			arrayAllocated = 16;
		}
		
		void *old = *array;
		void *replacement = kernelVMM.Allocate("DynArray", itemSize * arrayAllocated);
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

SharedMemoryRegion *SharedMemoryManager::CreateSharedMemory(size_t sizeBytes, char *name, size_t nameLength, Node *file) {
	mutex.Acquire();
	Defer(mutex.Release());

	// sizeBytes must be at PAGE_SIZE granularity.
	sizeBytes = (sizeBytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	// KernelLog(LOG_VERBOSE, "SharedMemoryManager::CreateSharedMemory - %d bytes\n", sizeBytes);

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

	SharedMemoryRegion *region;

	if (pages > BIG_SHARED_MEMORY / PAGE_SIZE) {
		size_t pageGroups = pages / (BIG_SHARED_MEMORY / PAGE_SIZE) + 1;
		region = (SharedMemoryRegion *) OSHeapAllocate(pageGroups * sizeof(void *) + sizeof(SharedMemoryRegion), false);
		ZeroMemory(region, pageGroups * sizeof(void *) + sizeof(SharedMemoryRegion));
		region->sizeBytes = sizeBytes;
		region->big = true;
	} else {
		region = (SharedMemoryRegion *) OSHeapAllocate(pages * sizeof(void *) + sizeof(SharedMemoryRegion), false);
		ZeroMemory(region, pages * sizeof(void *) + sizeof(SharedMemoryRegion));
		region->sizeBytes = sizeBytes;
	}

	if (namedRegion) {
		region->named = true;
		namedRegion->region = region;
	} else {
		region->named = false;
	}

	region->handles = 1;
	region->file = file;

	// Print("Created shared memory region %x\n", region);
	
	return region;
}

SharedMemoryRegion *SharedMemoryManager::LookupSharedMemory(char *name, size_t nameLength) {
	mutex.Acquire();
	Defer(mutex.Release());

	for (uintptr_t i = 0; i < namedSharedMemoryRegionsCount; i++) {
		if (namedSharedMemoryRegions[i].nameLength == nameLength 
				&& !CompareBytes(namedSharedMemoryRegions[i].name, name, nameLength)) {
			SharedMemoryRegion *region = namedSharedMemoryRegions[i].region;
			region->mutex.Acquire();
			region->handles++;
			region->mutex.Release();
			return region;
		}
	}

	return nullptr;
}

void SharedMemoryManager::DestroySharedMemory(SharedMemoryRegion *region) {
	if (region->handles) {
		KernelPanic("SharedMemoryManager::DestroySharedMemory - Region has handles.\n");
	}
	
	if (region->named) {
		mutex.Acquire();
		Defer(mutex.Release());

		for (uintptr_t i = 0; i < namedSharedMemoryRegionsCount; i++) {
			if (namedSharedMemoryRegions[i].region == region) {
				// Replace this with the last entry and decrement the count.
				namedSharedMemoryRegions[i] = namedSharedMemoryRegions[--namedSharedMemoryRegionsCount];
			}
		}
	}

	// KernelLog(LOG_VERBOSE, "Freeing shared memory region.... (%d bytes)\n", region->sizeBytes);

	size_t pages = region->sizeBytes / PAGE_SIZE;
	if (region->sizeBytes & (PAGE_SIZE - 1)) pages++;

	if (region->big) {
		size_t pageGroups = pages / (BIG_SHARED_MEMORY / PAGE_SIZE) + 1;
		uintptr_t **addresses = (uintptr_t **) (region + 1);

		pmm.lock.Acquire();
		for (uintptr_t i = 0; i < pageGroups; i++) {
			if (!addresses[i]) continue;
			for (uintptr_t j = 0; j < (BIG_SHARED_MEMORY / PAGE_SIZE); j++) {
				uintptr_t address = addresses[i][j];
				if (!address) continue;
				pmm.FreePage(address);
			}
		}
		pmm.lock.Release();

		for (uintptr_t i = 0; i < pageGroups; i++) {
			if (!addresses[i]) continue;
			kernelVMM.Free(addresses[i]);
		}
	} else {
		uintptr_t *addresses = (uintptr_t *) (region + 1);

		pmm.lock.Acquire();
		for (uintptr_t i = 0; i < pages; i++) {
			uintptr_t address = addresses[i];
			if (!address) continue;
			pmm.FreePage(address);
		}
		pmm.lock.Release();
	}

	// Print("Destroyed shared memory region %x\n", region);

	OSHeapFree(region);
}

Mutex zeroPhysicalMemoryLock;
Spinlock zeroPhysicalMemoryProcessorLock;

void ZeroPhysicalMemory(uintptr_t page, size_t pageCount) {
	zeroPhysicalMemoryLock.Acquire();

	repeat:;
	size_t doCount = pageCount > ZERO_MEMORY_REGION_PAGES ? ZERO_MEMORY_REGION_PAGES : pageCount;
	pageCount -= doCount;

	{
		VirtualAddressSpace &vas = kernelVMM.virtualAddressSpace;
		void *region = zeroMemoryRegion;

		vas.lock.Acquire();

		for (uintptr_t i = 0; i < doCount; i++) {
			kernelVMM.virtualAddressSpace.Map(page + PAGE_SIZE * i, (uintptr_t) region + PAGE_SIZE * i, VMM_REGION_FLAG_CACHABLE | VMM_REGION_FLAG_OVERWRITABLE);
		}

		vas.lock.Release();

		zeroPhysicalMemoryProcessorLock.Acquire();

		for (uintptr_t i = 0; i < doCount; i++) {
			ProcessorInvalidatePage((uintptr_t) region + doCount * PAGE_SIZE);
		}

		ZeroMemory(region, doCount * PAGE_SIZE);

		zeroPhysicalMemoryProcessorLock.Release();
	}

	if (pageCount) {
		page += ZERO_MEMORY_REGION_PAGES * PAGE_SIZE;
		goto repeat;
	}

	zeroPhysicalMemoryLock.Release();
}


#endif
