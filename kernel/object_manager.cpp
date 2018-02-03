#ifndef IMPLEMENTATION

#define CLOSABLE_OBJECT_TYPES ((KernelObjectType) \
		(KERNEL_OBJECT_MUTEX | KERNEL_OBJECT_PROCESS | KERNEL_OBJECT_THREAD \
		 | KERNEL_OBJECT_SHMEM | KERNEL_OBJECT_NODE | KERNEL_OBJECT_EVENT \
		 | KERNEL_OBJECT_SURFACE | KERNEL_OBJECT_WINDOW | KERNEL_OBJECT_IO_REQUEST))

enum KernelObjectType {
	KERNEL_OBJECT_PROCESS 		= 0x00000001,
	KERNEL_OBJECT_THREAD		= 0x00000002,
	KERNEL_OBJECT_SURFACE		= 0x00000004,
	KERNEL_OBJECT_WINDOW		= 0x00000008,
	KERNEL_OBJECT_MUTEX		= 0x00000010,
	KERNEL_OBJECT_SHMEM		= 0x00000020,
	KERNEL_OBJECT_NODE		= 0x00000040,
	KERNEL_OBJECT_EVENT		= 0x00000080,
	KERNEL_OBJECT_IO_REQUEST	= 0x00000100,
};

struct Handle {
	KernelObjectType type;
	void *object;
	uint64_t flags;

	volatile unsigned lock; // Must be 0 to close the handle.
			        // Incremented when the handle is used in a system call.
	volatile unsigned closing;

	bool readOnly;
};

size_t totalHandleCount;

void CloseHandleToObject(void *object, KernelObjectType type, uint64_t flags = 0);

enum ResolveHandleReason {
	RESOLVE_HANDLE_TO_USE,
	RESOLVE_HANDLE_TO_CLOSE,
};

struct HandleTableL3 {
#define HANDLE_TABLE_L3_ENTRIES 2048
	Handle t[HANDLE_TABLE_L3_ENTRIES];
};

struct HandleTableL2 {
#define HANDLE_TABLE_L2_ENTRIES 64
	HandleTableL3 *t[HANDLE_TABLE_L2_ENTRIES];
	size_t u[HANDLE_TABLE_L2_ENTRIES];
};

struct HandleTableL1 {
#define HANDLE_TABLE_L1_ENTRIES 64
	HandleTableL2 *t[HANDLE_TABLE_L1_ENTRIES];
	size_t u[HANDLE_TABLE_L1_ENTRIES];
};

struct HandleTable {
	HandleTableL1 l1r;
	Handle *linear;
	Mutex lock;
	Process *process;

	OSHandle OpenHandle(Handle &handle);
	void CloseHandle(OSHandle handle);

	// Resolve the handle if it is valid and return the type in type.
	// The initial value of type is used as a mask of expected object types for the handle.
	void *ResolveHandle(OSHandle handle, KernelObjectType &type, ResolveHandleReason reason = RESOLVE_HANDLE_TO_USE, Handle **handleData = nullptr); 
	void CompleteHandle(void *object, OSHandle handle); // Decrements handle lock.

	void Destroy(); 
};

void InitialiseObjectManager();
uintptr_t emptyHandlePage; // When we get a page fault in the linear handle list, use this to generate the fatal error.

#endif

#ifdef IMPLEMENTATION

// A lock used to change the handle count on mutexes and events.
Mutex objectHandleCountChange;

void CloseHandleToObject(void *object, KernelObjectType type, uint64_t flags) {
	switch (type) {
		case KERNEL_OBJECT_MUTEX: {
			objectHandleCountChange.Acquire();

			Mutex *mutex = (Mutex *) object;
			mutex->handles--;

			bool deallocate = !mutex->handles;

			objectHandleCountChange.Release();

			if (deallocate) {
				OSHeapFree(mutex, sizeof(Mutex));
			}
		} break;

		case KERNEL_OBJECT_EVENT: {
			objectHandleCountChange.Acquire();

			Event *event = (Event *) object;
			event->handles--;

			bool deallocate = !event->handles;

			objectHandleCountChange.Release();

			if (deallocate) {
				OSHeapFree(event, sizeof(Event));
			}
		} break;

		case KERNEL_OBJECT_PROCESS: {
			scheduler.lock.Acquire();
			RegisterAsyncTask(CloseHandleToProcess, object, (Process *) object, true);
			scheduler.lock.Release();
		} break;

		case KERNEL_OBJECT_THREAD: {
			scheduler.lock.Acquire();
			RegisterAsyncTask(CloseHandleToThread, object, kernelProcess, true);
			scheduler.lock.Release();
		} break;

		case KERNEL_OBJECT_SHMEM: {
			SharedMemoryRegion *region = (SharedMemoryRegion *) object;
			region->mutex.Acquire();
			bool destroy = region->handles == 1;
			region->handles--;
			// Print("%d handles remaining\n", region->handles);
			region->mutex.Release();

			if (destroy) {
				sharedMemoryManager.DestroySharedMemory(region);
			}
		} break;

		case KERNEL_OBJECT_NODE: {
			vfs.CloseNode((Node *) object, flags);
		} break;

		case KERNEL_OBJECT_SURFACE: {
			Surface *surface = (Surface *) object;
			surface->mutex.Acquire();
			bool destroy = surface->handles == 1;
			surface->handles--;
			surface->mutex.Release();

			if (destroy) {
				surface->Destroy();
			}
		} break;

		case KERNEL_OBJECT_WINDOW: {
			Window *window = (Window *) object;
			windowManager.mutex.Acquire();
			bool destroy = window->handles == 1;
			window->handles--;
			Print("closing window handle, %d remain...\n", window->handles);
			windowManager.mutex.Release();

			if (destroy) {
				Print("destroying window...\n");
				window->Destroy();
			}
		} break;

		case KERNEL_OBJECT_IO_REQUEST: {
			IORequest *request = (IORequest *) object;
			request->mutex.Acquire();
			bool destroy = request->CloseHandle(true /* Cancel the request; processes can only have 1 handle to an IO request. */);
			request->mutex.Release();

			if (destroy) {
				OSHeapFree(request, sizeof(IORequest));
			}
		} break;

		default: {
			KernelPanic("CloseHandleToObject - Cannot close object of type %d.\n", type);
		} break;
	}
}

void HandleTable::CloseHandle(OSHandle handle) {
	lock.Acquire();

	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	HandleTableL1 *l1 = &l1r;
	HandleTableL2 *l2 = l1->t[l1Index];
	HandleTableL3 *l3 = l2->t[l2Index];

	Handle *_handle = l3->t + l3Index;
	KernelObjectType type = _handle->type;
	uint64_t flags = _handle->flags;
	void *object = _handle->object;

	ZeroMemory(_handle, sizeof(Handle));

	l1->u[l1Index]--;
	l2->u[l2Index]--;

	lock.Release();

	CloseHandleToObject(object, type, flags);
}

void *HandleTable::ResolveHandle(OSHandle handle, KernelObjectType &type, ResolveHandleReason reason, Handle **handleData) {
	// Check that the handle is within the correct bounds.
	if (!handle || handle >= HANDLE_TABLE_L1_ENTRIES * HANDLE_TABLE_L2_ENTRIES * HANDLE_TABLE_L3_ENTRIES) {
		return nullptr;
	}

	if (!linear) {
		// We haven't opened any handles yet!
		return nullptr;
	}

	// Special handles.
	if (reason == RESOLVE_HANDLE_TO_USE) { // We can't close these handles.
		if (handle == OS_CURRENT_THREAD && (type & KERNEL_OBJECT_THREAD)) {
			type = KERNEL_OBJECT_THREAD;
			return GetCurrentThread();
		} else if (handle == OS_CURRENT_PROCESS && (type & KERNEL_OBJECT_PROCESS)) {
			type = KERNEL_OBJECT_PROCESS;
			return GetCurrentThread()->process;
		} else if (handle == OS_SURFACE_UI_SHEET && (type & KERNEL_OBJECT_SURFACE)) {
			type = KERNEL_OBJECT_SURFACE;
			return &uiSheetSurface;
		} else if (handle == OS_SURFACE_WALLPAPER && (type & KERNEL_OBJECT_SURFACE)) {
			type = KERNEL_OBJECT_SURFACE;
			return &wallpaperSurface;
		}
	}

	lock.Acquire();
	Defer(lock.Release());

#if 0
	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	HandleTableL1 *l1 = &l1r;
	HandleTableL2 *l2 = l1->t[l1Index];
	if (!l2) return nullptr;
	HandleTableL3 *l3 = l2->t[l2Index];
	if (!l3) return nullptr;

	Handle *_handle = l3->t + l3Index;
#endif

	Handle *_handle = linear + handle;

	if ((_handle->type & type) && (_handle->object)) {
		// Increment the handle's lock, so it cannot be closed until the system call has completed.
		type = _handle->type;

		if (reason == RESOLVE_HANDLE_TO_USE) {
			if (_handle->closing) {
				return nullptr; // The handle is being closed.
			} else {
				_handle->lock++;
				if (handleData) *handleData = _handle;
				return _handle->object;
			}
		} else if (reason == RESOLVE_HANDLE_TO_CLOSE) {
			if (_handle->lock) {
				return nullptr; // The handle was locked and can't be closed.
			} else {
				_handle->closing = true;
				if (handleData) *handleData = _handle;
				return _handle->object;
			}
		} else {
			KernelPanic("HandleTable::ResolveHandle - Unsupported ResolveHandleReason.\n");
			return nullptr;
		}
	} else {
		return nullptr;
	}
}

void HandleTable::CompleteHandle(void *object, OSHandle handle) {
	if (!object) {
		// The object returned by ResolveHandle was invalid, so we don't need to complete the handle.
		return;
	}

	// We've already checked that the handle is valid during ResolveHandle,
	// and because the lock was incremented we know that it is still valid.
	
	lock.Acquire();
	Defer(lock.Release());
	
#if 0
	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	if (!l1Index) return; // Special handle.

	HandleTableL1 *l1 = &l1r;
	HandleTableL2 *l2 = l1->t[l1Index];
	HandleTableL3 *l3 = l2->t[l2Index];

	Handle *_handle = l3->t + l3Index;
#endif
	Handle *_handle = linear + handle;
	_handle->lock--;
}

OSHandle HandleTable::OpenHandle(Handle &handle) {
	lock.Acquire();
	Defer(lock.Release());

	if (!handle.object) {
		KernelPanic("HandleTable::OpenHandle - Invalid object.\n");
	}

	if (!linear) {
		linear = (Handle *) kernelVMM.Allocate("HTL", HANDLE_TABLE_L1_ENTRIES * HANDLE_TABLE_L2_ENTRIES * HANDLE_TABLE_L3_ENTRIES * sizeof(Handle),
				VMM_MAP_STRICT, VMM_REGION_HANDLE_TABLE, 0, VMM_REGION_FLAG_SUPERVISOR, this);
	}

	handle.closing = false;
	handle.lock = 0;

	HandleTableL1 *l1 = &l1r;
	uintptr_t l1Index = HANDLE_TABLE_L1_ENTRIES;

	for (uintptr_t i = 1 /* The first set of handles are reserved. */; i < HANDLE_TABLE_L1_ENTRIES; i++) {
		if (l1->u[i] != HANDLE_TABLE_L2_ENTRIES * HANDLE_TABLE_L3_ENTRIES) {
			l1->u[i]++;
			l1Index = i;
			break;
		}
	}

	if (l1Index == HANDLE_TABLE_L1_ENTRIES) {
		return OS_INVALID_HANDLE;
	}

	if (!l1->t[l1Index]) {
		l1->t[l1Index] = (HandleTableL2 *) OSHeapAllocate(sizeof(HandleTableL2), true);
	}

	HandleTableL2 *l2 = l1->t[l1Index];
	uintptr_t l2Index = HANDLE_TABLE_L2_ENTRIES;

	for (uintptr_t i = 0; i < HANDLE_TABLE_L2_ENTRIES; i++) {
		if (l2->u[i] != HANDLE_TABLE_L3_ENTRIES) {
			l2->u[i]++;
			l2Index = i;
			break;
		}
	}

	if (l2Index == HANDLE_TABLE_L2_ENTRIES) {
		KernelPanic("HandleTable::OpenHandle - Unexpected lack of free handles.\n");
	}

	bool mapHL3 = false;

	if (!l2->t[l2Index]) {
		l2->t[l2Index] = (HandleTableL3 *) kernelVMM.Allocate("HL3", sizeof(HandleTableL3), VMM_MAP_ALL);
		mapHL3 = true;
	}

	HandleTableL3 *l3 = l2->t[l2Index];
	uintptr_t l3Index = HANDLE_TABLE_L3_ENTRIES;

	for (uintptr_t i = 0; i < HANDLE_TABLE_L3_ENTRIES; i++) {
		if (!l3->t[i].object) {
			l3Index = i;
			break;
		}
	}

	if (l3Index == HANDLE_TABLE_L3_ENTRIES) {
		KernelPanic("HandleTable::OpenHandle - Unexpected lack of free handles.\n");
	}

	Handle *_handle = l3->t + l3Index;
	*_handle = handle;

	OSHandle index = (l3Index) + (l2Index * HANDLE_TABLE_L3_ENTRIES) + (l1Index * HANDLE_TABLE_L3_ENTRIES * HANDLE_TABLE_L2_ENTRIES);

	if (mapHL3) {
		size_t count = sizeof(HandleTableL3) / PAGE_SIZE;

		for (uintptr_t i = 0; i < count; i++) {
			VirtualAddressSpace *kernelAddressSpace = kernelVMM.virtualAddressSpace;
			kernelAddressSpace->lock.Acquire();
			uintptr_t n = kernelAddressSpace->Get((uintptr_t) l3 + i * PAGE_SIZE);
			kernelAddressSpace->lock.Release();
			VirtualAddressSpace *addressSpace = process->vmm->virtualAddressSpace;
			addressSpace->lock.Acquire();
			addressSpace->Map(n, (uintptr_t) (linear + (l2Index * HANDLE_TABLE_L3_ENTRIES) + (l1Index * HANDLE_TABLE_L3_ENTRIES * HANDLE_TABLE_L2_ENTRIES)) + i * PAGE_SIZE, VMM_REGION_FLAG_SUPERVISOR);
			addressSpace->lock.Release();
		}
	}

	__sync_fetch_and_add(&totalHandleCount, 1);

	return index;
}

void HandleTable::Destroy() {
	HandleTableL1 *l1 = &l1r;

	Print("--- Destroying handle table...\n");

	for (uintptr_t i = 1; i < HANDLE_TABLE_L1_ENTRIES; i++) {
		if (l1->u[i]) {
			HandleTableL2 *l2 = l1->t[i];

			for (uintptr_t j = 0; j < HANDLE_TABLE_L2_ENTRIES; j++) {
				if (l2->u[j]) {
					HandleTableL3 *l3 = l2->t[j];

					for (uintptr_t k = 0; k < HANDLE_TABLE_L3_ENTRIES; k++) {
						Handle *handle = l3->t + k;

						if (handle->object) {
							if (handle->lock) {
								KernelPanic("HandleTable::Destroy - Handle in table was locked.\n");
							}

							if (handle->type & CLOSABLE_OBJECT_TYPES) {
								KernelLog(LOG_VERBOSE, "Destroying handle to object %x of type %d...\n", handle->object, handle->type);
								CloseHandleToObject(handle->object, handle->type);
							} else {
								KernelPanic("HandleTable::Destroy - Handle type %d cannot be closed.\n", handle->type);
							}
						}
					}

					kernelVMM.Free(l3);
				}
			}

			OSHeapFree(l2, sizeof(HandleTableL2));
		}
	}

	Print("--- Handle table destroyed.\n");
}

void InitialiseObjectManager() {
	emptyHandlePage = pmm.AllocatePage(true);
}

#endif
