struct OSHeapRegion {
	union {
		uint16_t next;
		uint16_t size;
	};

	uint16_t previous;
	uint16_t offset;
	uint16_t used;

	// Valid if the region is not in use.
	OSHeapRegion *regionListNext;
	OSHeapRegion **regionListReference;
};

static uintptr_t OSHeapCalculateIndex(uintptr_t size) {
	int x = __builtin_clz(size);
	uintptr_t msb = sizeof(unsigned int) * 8 - x - 1;
	return msb - 4;
}

static OSHeapRegion *heapRegions[12];

#ifdef KERNEL
Mutex heapMutex;
#define OS_HEAP_ACQUIRE_MUTEX() heapMutex.Acquire()
#define OS_HEAP_RELEASE_MUTEX() heapMutex.Release()
#define OS_HEAP_PANIC(n) KernelPanic("Heap panic (%d).\n", n)
#define OS_HEAP_ALLOCATE_CALL(x) kernelVMM.Allocate("Heap", x)
#define OS_HEAP_FREE_CALL(x) kernelVMM.Free(x)
#else
static OSHandle heapMutex;
#define OS_HEAP_ACQUIRE_MUTEX() OSAcquireMutex(heapMutex)
#define OS_HEAP_RELEASE_MUTEX() OSReleaseMutex(heapMutex)
#define OS_HEAP_PANIC(n) { OSPrint("Heap panic, %d\n", n); Panic(); }
#define OS_HEAP_ALLOCATE_CALL(x) OSAllocate(x)
#define OS_HEAP_FREE_CALL(x) OSFree(x)
#endif

#define OS_HEAP_REGION_HEADER(region) ((OSHeapRegion *) ((uint8_t *) region - 0x10))
#define OS_HEAP_REGION_DATA(region) ((uint8_t *) region + 0x10)
#define OS_HEAP_REGION_NEXT(region) ((OSHeapRegion *) ((uint8_t *) region + region->next))
#define OS_HEAP_REGION_PREVIOUS(region) (region->previous ? ((OSHeapRegion *) ((uint8_t *) region - region->previous)) : nullptr)

#ifndef KERNEL
static void OSHeapInitialise() {
	heapMutex = OSCreateMutex();
}
#endif

static void OSHeapRemoveFreeRegion(OSHeapRegion *region) {
	if (!region->regionListReference || region->used) {
		OS_HEAP_PANIC(0);
	}

	*region->regionListReference = region->regionListNext;

	if (region->regionListNext) {
		region->regionListNext->regionListReference = region->regionListReference;
	}

	region->regionListReference = nullptr;
}

static void OSHeapAddFreeRegion(OSHeapRegion *region) {
	if (region->used || region->size < 32) {
		OS_HEAP_PANIC(1);
	}

	int index = OSHeapCalculateIndex(region->size);
	region->regionListNext = heapRegions[index];
	if (region->regionListNext) region->regionListNext->regionListReference = &region->regionListNext;
	heapRegions[index] = region;
	region->regionListReference = heapRegions + index;
}

void *OSHeapAllocate(size_t size, bool zeroMemory) {
	if (!size) return nullptr;

#ifndef KERNEL
	// OSPrint("Allocate: %d\n", size);
#else
	// Print("Allocate: %d\n", size);
#endif

	size_t originalSize = size;

	size += 0x10; // Region metadata.
	size = (size + 0x1F) & ~0x1F; // Allocation granularity: 32 bytes.

	if (size >= 32768) {
		// This is a very large allocation, so allocate it by itself.
		// We don't need to zero this memory. (It'll be done by the PMM).
		OSHeapRegion *region = (OSHeapRegion *) OS_HEAP_ALLOCATE_CALL(size);
		region->used = 0xABCD;
		if (!region) return nullptr; 
		
		void *address = OS_HEAP_REGION_DATA(region);
		return address;
	}

	OS_HEAP_ACQUIRE_MUTEX();

	static bool concurrentModificationCheck = false;
	if (concurrentModificationCheck) OS_HEAP_PANIC(2);
	concurrentModificationCheck = true;

	OSHeapRegion *region = nullptr;

	for (int i = OSHeapCalculateIndex(size); i < 12; i++) {
		if (heapRegions[i] == nullptr || heapRegions[i]->size < size) {
			continue;
		}

		region = heapRegions[i];
		OSHeapRemoveFreeRegion(region);
		goto foundRegion;
	}

	region = (OSHeapRegion *) OS_HEAP_ALLOCATE_CALL(65536);
	if (!region) {
		concurrentModificationCheck = false;
		OS_HEAP_RELEASE_MUTEX();
		return nullptr; 
	}
	region->size = 65536 - 32;

	// Prevent OSHeapFree trying to merge off the end of the block.
	{
		OSHeapRegion *endRegion = OS_HEAP_REGION_NEXT(region);
		endRegion->used = 0xABCD;
	}

	foundRegion:

	if (region->used || region->size < size) {
		OS_HEAP_PANIC(4);
	}

	if (region->size == size) {
		// If the size of this region is equal to the size of the region we're trying to allocate,
		// return this region immediately.
		region->used = 0xABCD;
		concurrentModificationCheck = false;
		OS_HEAP_RELEASE_MUTEX();
		if (zeroMemory) CF(ZeroMemory)(OS_HEAP_REGION_DATA(region), originalSize);

		void *address = OS_HEAP_REGION_DATA(region);
		return address;
	}

	// Split the region into 2 parts.
	
	OSHeapRegion *allocatedRegion = region;
	size_t oldSize = allocatedRegion->size;
	allocatedRegion->size = size;
	allocatedRegion->used = 0xABCD;

	OSHeapRegion *freeRegion = OS_HEAP_REGION_NEXT(allocatedRegion);
	freeRegion->size = oldSize - size;
	freeRegion->previous = size;
	freeRegion->offset = allocatedRegion->offset + size;
	freeRegion->used = false;
	OSHeapAddFreeRegion(freeRegion);

	OSHeapRegion *nextRegion = OS_HEAP_REGION_NEXT(freeRegion);
	nextRegion->previous = freeRegion->size;

	concurrentModificationCheck = false;
	OS_HEAP_RELEASE_MUTEX();
	if (zeroMemory) CF(ZeroMemory)(OS_HEAP_REGION_DATA(allocatedRegion), originalSize);

	void *address = OS_HEAP_REGION_DATA(region);
	return address;
}

#ifdef KERNEL
void OSHeapFree(void *address, size_t expectedSize = 0) {
#else
void OSHeapFree(void *address) {
	size_t expectedSize = 0;
#endif
	if (!address && expectedSize) OS_HEAP_PANIC(10);
	if (!address) return;

	OSHeapRegion *region = OS_HEAP_REGION_HEADER(address);
	if (region->used != 0xABCD) OS_HEAP_PANIC(region->used);

#ifndef KERNEL
	// OSPrint("Free: %x (%d bytes)\n", address, region->size);
#else
	// Print("free %x\n", address);
#endif

	bool expectingSize = expectedSize != 0;
	expectedSize += 0x10; // Region metadata.
	expectedSize = (expectedSize + 0x1F) & ~0x1F; // Allocation granularity: 32 bytes.

	if (!region->size) {
		// The region was allocated by itself.
		OS_HEAP_FREE_CALL(region);
		return;
	}

	OS_HEAP_ACQUIRE_MUTEX();

	region->used = false;
	if (expectingSize && region->size != expectedSize) OS_HEAP_PANIC(6);

	// Attempt to merge with the next region.

	OSHeapRegion *nextRegion = OS_HEAP_REGION_NEXT(region);

	if (nextRegion && !nextRegion->used) {
		OSHeapRemoveFreeRegion(nextRegion);

		// Merge the regions.
		region->size += nextRegion->size;
		OS_HEAP_REGION_NEXT(nextRegion)->previous = region->size;
	}

	// Attempt to merge with the previous region.

	OSHeapRegion *previousRegion = OS_HEAP_REGION_PREVIOUS(region);

	if (previousRegion && !previousRegion->used) {
		OSHeapRemoveFreeRegion(previousRegion);

		// Merge the regions.
		previousRegion->size += region->size;
		OS_HEAP_REGION_NEXT(region)->previous = previousRegion->size;
		region = previousRegion;
	}

	if (region->size == 65536 - 32) {
		if (region->offset) OS_HEAP_PANIC(7);

		// The memory block is empty.
		OS_HEAP_FREE_CALL(region);
		OS_HEAP_RELEASE_MUTEX();
		return;
	}

	// Put the free region in the region list.
	OSHeapAddFreeRegion(region);
	OS_HEAP_RELEASE_MUTEX();
}

#if 0
void *OSHeapDuplicate(void *address) {
	if (!address) return nullptr;

	OSHeapRegion *region = OS_HEAP_REGION_HEADER(address);
	if (region->used != 0xABCD) OS_HEAP_PANIC();

	if (!region->size) {
		// The region was allocated by itself.
		// TODO Implement this.
		OS_HEAP_PANIC();
	}

	void *duplicate = OSHeapAllocate(region->size, false);
	CF(CopyMemory)(duplicate, address, region->size);
	return duplicate;
}
#endif
