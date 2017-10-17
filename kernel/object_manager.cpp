#ifndef IMPLEMENTATION

#define CLOSABLE_OBJECT_TYPES ((KernelObjectType) (KERNEL_OBJECT_MUTEX | KERNEL_OBJECT_PROCESS | KERNEL_OBJECT_THREAD | KERNEL_OBJECT_SHMEM))

enum KernelObjectType {
	KERNEL_OBJECT_PROCESS 	= 0x00000001,
	KERNEL_OBJECT_THREAD	= 0x00000002,
	KERNEL_OBJECT_SURFACE	= 0x00000004,
	KERNEL_OBJECT_WINDOW	= 0x00000008,
	KERNEL_OBJECT_MUTEX	= 0x00000010,
	KERNEL_OBJECT_SHMEM	= 0x00000020,
};

struct Handle {
	KernelObjectType type;
	void *object;
	volatile unsigned lock; // Must be 0 to close the handle.
			        // Incremented when the handle is used in a system call.
	volatile unsigned closing;

	bool readOnly;
};

void CloseHandleToObject(void *object, KernelObjectType type);

enum ResolveHandleReason {
	RESOLVE_HANDLE_TO_USE,
	RESOLVE_HANDLE_TO_CLOSE,
};

struct HandleTableL3 {
#define HANDLE_TABLE_L3_ENTRIES 512
	Handle t[HANDLE_TABLE_L3_ENTRIES];
};

struct HandleTableL2 {
#define HANDLE_TABLE_L2_ENTRIES 512
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

	Mutex lock;

	OSHandle OpenHandle(Handle &handle);
	void CloseHandle(OSHandle handle);

	// Resolve the handle if it is valid and return the type in type.
	// The initial value of type is used as a mask of expected object types for the handle.
	void *ResolveHandle(OSHandle handle, KernelObjectType &type, ResolveHandleReason reason = RESOLVE_HANDLE_TO_USE, Handle **handleData = nullptr); 
	void CompleteHandle(void *object, OSHandle handle); // Decrements handle lock.

	void Destroy(); 
};

#endif

#ifdef IMPLEMENTATION

void CloseHandleToObject(void *object, KernelObjectType type) {
	switch (type) {
		case KERNEL_OBJECT_MUTEX: {
			scheduler.lock.Acquire();

			Mutex *mutex = (Mutex *) object;
			mutex->handles--;

			bool deallocate = !mutex->handles;

			scheduler.lock.Release();

			if (deallocate) {
				scheduler.globalMutexPool.Remove(mutex);
			}
		} break;

		case KERNEL_OBJECT_PROCESS: {
			scheduler.lock.Acquire();
			RegisterAsyncTask(CloseHandleToProcess, object, &((Process *) object)->vmm->virtualAddressSpace);
			scheduler.lock.Release();
		} break;

		case KERNEL_OBJECT_THREAD: {
			scheduler.lock.Acquire();
			RegisterAsyncTask(CloseHandleToThread, object, &((Thread *) object)->process->vmm->virtualAddressSpace);
			scheduler.lock.Release();
		} break;

		case KERNEL_OBJECT_SHMEM: {
			SharedMemoryRegion *region = (SharedMemoryRegion *) object;
			region->mutex.Acquire();
			bool destroy = region->handles == 1;
			region->handles--;
			region->mutex.Release();

			if (destroy) {
				sharedMemoryManager.DestroySharedMemory(region);
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
	void *object = _handle->object;

	ZeroMemory(_handle, sizeof(Handle));

	l1->u[l1Index]--;
	l2->u[l2Index]--;

	lock.Release();

	CloseHandleToObject(object, type);
}

void *HandleTable::ResolveHandle(OSHandle handle, KernelObjectType &type, ResolveHandleReason reason, Handle **handleData) {
	// Check that the handle is within the correct bounds.
	if (!handle || handle >= HANDLE_TABLE_L1_ENTRIES * HANDLE_TABLE_L2_ENTRIES * HANDLE_TABLE_L3_ENTRIES) {
		return nullptr;
	}

	// Special handles.
	if (reason == RESOLVE_HANDLE_TO_USE) { // We can't close these handles.
		if (handle == OS_CURRENT_THREAD && (type & KERNEL_OBJECT_THREAD)) {
			type = KERNEL_OBJECT_THREAD;
			return ProcessorGetLocalStorage()->currentThread;
		} else if (handle == OS_CURRENT_PROCESS && (type & KERNEL_OBJECT_PROCESS)) {
			type = KERNEL_OBJECT_PROCESS;
			return ProcessorGetLocalStorage()->currentThread->process;
		} else if (handle == OS_SURFACE_UI_SHEET && (type & KERNEL_OBJECT_SURFACE)) {
			type = KERNEL_OBJECT_SURFACE;
			return &uiSheetSurface;
		}
	}

	lock.Acquire();
	Defer(lock.Release());

	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	HandleTableL1 *l1 = &l1r;
	HandleTableL2 *l2 = l1->t[l1Index];
	if (!l2) return nullptr;
	HandleTableL3 *l3 = l2->t[l2Index];
	if (!l3) return nullptr;

	Handle *_handle = l3->t + l3Index;

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
	
	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	if (!l1Index) return; // Special handle.

	HandleTableL1 *l1 = &l1r;
	HandleTableL2 *l2 = l1->t[l1Index];
	HandleTableL3 *l3 = l2->t[l2Index];

	Handle *_handle = l3->t + l3Index;
	_handle->lock--;
}

OSHandle HandleTable::OpenHandle(Handle &handle) {
	lock.Acquire();
	Defer(lock.Release());

	if (!handle.object) {
		KernelPanic("HandleTable::OpenHandle - Invalid object.\n");
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

	if (!l2->t[l2Index]) {
		l2->t[l2Index] = (HandleTableL3 *) OSHeapAllocate(sizeof(HandleTableL3), true);
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
	return index;
}

void HandleTable::Destroy() {
	HandleTableL1 *l1 = &l1r;

	KernelLog(LOG_VERBOSE, "Destroying handle table...\n");

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
							}
						}
					}

					OSHeapFree(l3);
				}
			}

			OSHeapFree(l2);
		}
	}
}

#endif
