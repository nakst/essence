uintptr_t DoSyscall(uintptr_t index,
		uintptr_t argument0, uintptr_t argument1,
		uintptr_t argument2, uintptr_t argument3);

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

			Process *process = (Process *) object;
			process->handles--;

			bool deallocate = !process->handles;

			scheduler.lock.Release();

			if (deallocate) {
				scheduler.RemoveProcess(process);
			}
		} break;

		case KERNEL_OBJECT_THREAD: {
			scheduler.lock.Acquire();
			RegisterAsyncTask(CloseThreadHandle, object, &((Thread *) object)->process->vmm->virtualAddressSpace);
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
			KernelPanic("DoSyscall - Cannot close object of type %d.\n", type);
		} break;
	}
}

void Process::CloseHandle(OSHandle handle) {
	handleTable.lock.Acquire();

	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	HandleTableL1 *l1 = &handleTable;
	HandleTableL2 *l2 = l1->t[l1Index];
	HandleTableL3 *l3 = l2->t[l2Index];

	Handle *_handle = l3->t + l3Index;
	KernelObjectType type = _handle->type;
	void *object = _handle->object;

	ZeroMemory(_handle, sizeof(Handle));

	l1->u[l1Index]--;
	l2->u[l2Index]--;

	handleTable.lock.Release();

	CloseHandleToObject(object, type);
}

void *Process::ResolveHandle(OSHandle handle, KernelObjectType &type, ResolveHandleReason reason, Handle **handleData) {
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

	handleTable.lock.Acquire();
	Defer(handleTable.lock.Release());

	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	HandleTableL1 *l1 = &handleTable;
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
			KernelPanic("Process::ResolveHandle - Unsupported ResolveHandleReason.\n");
			return nullptr;
		}
	} else {
		return nullptr;
	}
}

void Process::CompleteHandle(void *object, OSHandle handle) {
	if (!object) {
		// The object returned by ResolveHandle was invalid, so we don't need to complete the handle.
		return;
	}

	// We've already checked that the handle is valid during ResolveHandle,
	// and because the lock was incremented we know that it is still valid.
	
	handleTable.lock.Acquire();
	Defer(handleTable.lock.Release());
	
	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	if (!l1Index) return; // Special handle.

	HandleTableL1 *l1 = &handleTable;
	HandleTableL2 *l2 = l1->t[l1Index];
	HandleTableL3 *l3 = l2->t[l2Index];

	Handle *_handle = l3->t + l3Index;
	_handle->lock--;
}

OSHandle Process::OpenHandle(Handle &handle) {
	handleTable.lock.Acquire();
	Defer(handleTable.lock.Release());

	if (!handle.object) {
		KernelPanic("Process::OpenHandle - Invalid object.\n");
	}

	handle.closing = false;
	handle.lock = 0;

	HandleTableL1 *l1 = &handleTable;
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
		KernelPanic("Process::OpenHandle - Unexpected lack of free handles.\n");
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
		KernelPanic("Process::OpenHandle - Unexpected lack of free handles.\n");
	}

	Handle *_handle = l3->t + l3Index;
	*_handle = handle;

	OSHandle index = (l3Index) + (l2Index * HANDLE_TABLE_L3_ENTRIES) + (l1Index * HANDLE_TABLE_L3_ENTRIES * HANDLE_TABLE_L2_ENTRIES);
	return index;
}

bool Process::SendMessage(OSMessage &_message) {
	messageQueueMutex.Acquire();
	Defer(messageQueueMutex.Release());

	if (messageQueue.count == MESSAGE_QUEUE_MAX_LENGTH) {
		return false;
	}

	Message *message = (Message *) messagePool.Add();
	CopyMemory(&message->data, &_message, sizeof(OSMessage));
	message->item.thisItem = message;
	messageQueue.InsertEnd(&message->item);

	if (!messageQueueIsNotEmpty.Poll()) {
		messageQueueIsNotEmpty.Set();
	}

	return true;
}

uintptr_t DoSyscall(uintptr_t index,
		uintptr_t argument0, uintptr_t argument1,
		uintptr_t argument2, uintptr_t argument3) {
	(void) argument0;
	(void) argument1;
	(void) argument2;
	(void) argument3;

	// Interrupts need to be enabled during system calls,
	// because many of them block on mutexes or events.
	ProcessorEnableInterrupts();

	Thread *currentThread = ProcessorGetLocalStorage()->currentThread;
	Process *currentProcess = currentThread->process;
	VMM *currentVMM = currentProcess->vmm;

	if (currentThread->terminating) {
		// The thread has been terminated.
		// Yield the scheduler so it can be removed.
		ProcessorFakeTimerInterrupt();
	}

	currentThread->terminatableState = THREAD_IN_SYSCALL;

	OSError returnValue = OS_ERROR_UNKNOWN_SYSCALL;
#define SYSCALL_RETURN(value) {returnValue = value; goto end;}

	switch (index) {
		case OS_SYSCALL_PRINT: {
			VMMRegion *region = currentVMM->FindAndLockRegion(argument0, argument1);
			if (!region) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region));
			Print("%s", argument1, argument0);
			SYSCALL_RETURN(OS_SUCCESS);
	     	} break;

		case OS_SYSCALL_ALLOCATE: {
			uintptr_t address = (uintptr_t) currentVMM->Allocate(argument0);
			SYSCALL_RETURN(address);
		} break;

		case OS_SYSCALL_FREE: {
			void *object;
			VMMRegionType type;

			OSError error = currentVMM->Free((void *) argument0, &object, &type);

			if (error == OS_SUCCESS) {
				if (type == vmmRegionShared) {
					if (!object) KernelPanic("DoSyscall - Object from a freed shared memory region was null.\n");
					CloseHandleToObject(object, KERNEL_OBJECT_SHMEM);
				}
			}
	
			SYSCALL_RETURN(error);
		} break;

		case OS_SYSCALL_CREATE_PROCESS: {
			if (argument1 > MAX_PATH) SYSCALL_RETURN(OS_ERROR_PATH_LENGTH_EXCEEDS_LIMIT);

			VMMRegion *region1 = currentVMM->FindAndLockRegion(argument0, argument1);
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			VMMRegion *region2 = currentVMM->FindAndLockRegion(argument2, sizeof(OSProcessInformation));
			if (!region2) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region2));

			OSProcessInformation *process = (OSProcessInformation *) argument2;
			Process *processObject = scheduler.SpawnProcess((char *) argument0, argument1, false, (void *) argument3);

			if (!processObject) {
				SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
			} else {
				Handle handle = {};
				handle.type = KERNEL_OBJECT_PROCESS;
				handle.object = processObject;

				Handle handle2 = {};
				handle2.type = KERNEL_OBJECT_THREAD;
				handle2.object = processObject->executableMainThread;

				// Register processObject as a handle.
				process->handle = currentProcess->OpenHandle(handle); 
				process->pid = processObject->id;
				process->mainThread.handle = currentProcess->OpenHandle(handle2);
				process->mainThread.tid = processObject->executableMainThread->id;

				SYSCALL_RETURN(OS_SUCCESS);
			}
		} break;

		case OS_SYSCALL_GET_CREATION_ARGUMENT: {
			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->ResolveHandle(argument0, type);
			if (!process) SYSCALL_RETURN(0);
			Defer(currentProcess->CompleteHandle(process, argument0));

			uintptr_t creationArgument = (uintptr_t) process->creationArgument;
			SYSCALL_RETURN(creationArgument);
		} break;

		case OS_SYSCALL_CREATE_SURFACE: {
			Surface *surface = (Surface *) graphics.surfacePool.Add();
			if (!surface->Initialise(currentVMM, argument0, argument1, false)) {
				SYSCALL_RETURN(OS_INVALID_HANDLE);
			}

			Handle _handle = {};
			_handle.type = KERNEL_OBJECT_SURFACE;
			_handle.object = surface;
			// TODO Prevent the program from deallocating the surface's linear buffer.
			SYSCALL_RETURN(currentProcess->OpenHandle(_handle));
		} break;

		case OS_SYSCALL_GET_LINEAR_BUFFER: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(surface, argument0));

			OSLinearBuffer *linearBuffer = (OSLinearBuffer *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) linearBuffer, sizeof(OSLinearBuffer));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			if (surface->memoryInKernelAddressSpace) {
				SYSCALL_RETURN(OS_ERROR_BUFFER_NOT_ACCESSIBLE);
			}

			linearBuffer->width = surface->resX;
			linearBuffer->height = surface->resY;
			linearBuffer->buffer = surface->linearBuffer;
			linearBuffer->stride = surface->stride;
			linearBuffer->colorFormat = OS_COLOR_FORMAT_32_XRGB;

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_INVALIDATE_RECTANGLE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(surface, argument0));

			OSRectangle *rectangle = (OSRectangle *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) rectangle, sizeof(OSRectangle));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			surface->InvalidateRectangle(*rectangle);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_COPY_TO_SCREEN: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(surface, argument0));

			OSPoint *point = (OSPoint *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) point, sizeof(OSPoint));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			graphics.frameBuffer.Copy(*surface, *point, OSRectangle(0, surface->resX, 0, surface->resY), true, (uint16_t) argument2);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_FILL_RECTANGLE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(surface, argument0));

			_OSRectangleAndColor *arg = (_OSRectangleAndColor *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) arg, sizeof(_OSRectangleAndColor));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			surface->FillRectangle(arg->rectangle, arg->color);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_FORCE_SCREEN_UPDATE: {
			graphics.UpdateScreen();
		} break;

		case OS_SYSCALL_COPY_SURFACE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;

			Surface *destination = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->ResolveHandle(argument1, type);
			if (!source) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(source, argument1));

			OSPoint *point = (OSPoint *) argument2;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) point, sizeof(OSPoint));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			destination->Copy(*source, *point, OSRectangle(0, source->resX, 0, source->resY), true, SURFACE_COPY_WITHOUT_DEPTH_CHECKING);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_DRAW_SURFACE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;

			Surface *destination = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->ResolveHandle(argument1, type);
			if (!source) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(source, argument1));

			_OSDrawSurfaceArguments *arguments = (_OSDrawSurfaceArguments *) argument2;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) arguments, sizeof(_OSDrawSurfaceArguments));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			destination->Draw(*source, arguments->destination, arguments->source, arguments->border, (OSDrawMode) argument3);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_CLEAR_MODIFIED_REGION: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *destination = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(destination, argument0));

			destination->mutex.Acquire();
			destination->ClearModifiedRegion();
			destination->mutex.Release();

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_GET_MESSAGE: {
			OSMessage *returnMessage = (OSMessage *) argument0;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) returnMessage, sizeof(OSMessage));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			{
				currentProcess->messageQueueMutex.Acquire();
				Defer(currentProcess->messageQueueMutex.Release());

				if (currentProcess->messageQueue.count) {
					Message *message = (Message *) currentProcess->messageQueue.firstItem->thisItem;
					currentProcess->messageQueue.Remove(currentProcess->messageQueue.firstItem);
					CopyMemory(returnMessage, &message->data, sizeof(OSMessage));

					if (!currentProcess->messageQueue.count) {
						currentProcess->messageQueueIsNotEmpty.Reset();
					}
				} else {
					SYSCALL_RETURN(OS_ERROR_NO_MESSAGES_AVAILABLE);
				}
			}

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_WAIT_MESSAGE: {
			while (!currentProcess->messageQueue.count) {
				currentThread->terminatableState = THREAD_USER_BLOCK_REQUEST;
				currentProcess->messageQueueIsNotEmpty.Wait(argument0 /*Timeout*/);
			}

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_SEND_MESSAGE: {
			OSMessage *message = (OSMessage *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) message, sizeof(OSMessage));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->ResolveHandle(argument0, type);
			if (!process) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(process, argument0));

			{
				currentProcess->messageQueueMutex.Acquire();
				Defer(currentProcess->messageQueueMutex.Release());

				SYSCALL_RETURN(process->SendMessage(*message) ? OS_SUCCESS : OS_ERROR_MESSAGE_QUEUE_FULL);
			}

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_CREATE_WINDOW: {
			OSWindow *osWindow = (OSWindow *) argument0;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) osWindow, sizeof(OSWindow));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			Window *window = windowManager.CreateWindow(currentProcess, argument1, argument2);

			if (!window) {
				SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
			} else {
				window->apiWindow = osWindow;

				Handle _handle = {};

				_handle.type = KERNEL_OBJECT_WINDOW;
				_handle.object = window;
				osWindow->handle = currentProcess->OpenHandle(_handle);

				_handle.type = KERNEL_OBJECT_SURFACE;
				_handle.object = &window->surface;
				osWindow->surface = currentProcess->OpenHandle(_handle);
				
				OSMessage message = {};
				message.type = OS_MESSAGE_WINDOW_CREATED;
				message.targetWindow = window->apiWindow;
				window->owner->SendMessage(message);

				SYSCALL_RETURN(OS_SUCCESS);
			}
		} break;

		case OS_SYSCALL_UPDATE_WINDOW: {
			KernelObjectType type = KERNEL_OBJECT_WINDOW;
			Window *window = (Window *) currentProcess->ResolveHandle(argument0, type);
			if (!window) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(window, argument0));

			window->Update();

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_CREATE_MUTEX: {
			Mutex *mutex = (Mutex *) scheduler.globalMutexPool.Add();
			mutex->handles = 1;
			if (!mutex) SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
			Handle handle = {};
			handle.type = KERNEL_OBJECT_MUTEX;
			handle.object = mutex;
			SYSCALL_RETURN(currentProcess->OpenHandle(handle));
		} break;

		case OS_SYSCALL_ACQUIRE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->ResolveHandle(argument0, type);
			if (!mutex) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(mutex, argument0));

			if (mutex->owner == currentThread) SYSCALL_RETURN(OS_ERROR_MUTEX_ALREADY_ACQUIRED);
			currentThread->terminatableState = THREAD_USER_BLOCK_REQUEST;
			mutex->Acquire();
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_RELEASE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->ResolveHandle(argument0, type);
			if (!mutex) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(mutex, argument0));

			if (mutex->owner != currentThread) SYSCALL_RETURN(OS_ERROR_MUTEX_NOT_ACQUIRED_BY_THREAD);
			mutex->Release();
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_CLOSE_HANDLE: {
			KernelObjectType type = (KernelObjectType) (KERNEL_OBJECT_MUTEX | KERNEL_OBJECT_PROCESS 
					| KERNEL_OBJECT_THREAD | KERNEL_OBJECT_SHMEM); 
			void *object = currentProcess->ResolveHandle(argument0, type, RESOLVE_HANDLE_TO_CLOSE);

			if (!object) {
				SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			}

			currentProcess->CloseHandle(argument0);
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_TERMINATE_THREAD: {
			KernelObjectType type = KERNEL_OBJECT_THREAD;
			Thread *thread = (Thread *) currentProcess->ResolveHandle(argument0, type);
			if (!thread) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(thread, argument0));

			scheduler.TerminateThread(thread);
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_CREATE_THREAD: {
			VMMRegion *region2 = currentVMM->FindAndLockRegion(argument2, sizeof(OSThreadInformation));
			if (!region2) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region2));

			OSThreadInformation *thread = (OSThreadInformation *) argument2;
			Thread *threadObject = scheduler.SpawnThread(argument0, argument3, currentProcess, true, true);

			if (!threadObject) {
				SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE);
			} else {
				Handle handle = {};
				handle.type = KERNEL_OBJECT_THREAD;
				handle.object = threadObject;

				// Register processObject as a handle.
				thread->handle = currentProcess->OpenHandle(handle); 
				thread->tid = threadObject->id;

				SYSCALL_RETURN(OS_SUCCESS);
			}
		} break;

		case OS_SYSCALL_READ_ENTIRE_FILE: {
			VMMRegion *region1 = currentVMM->FindAndLockRegion(argument0, argument1);
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			VMMRegion *region2 = currentVMM->FindAndLockRegion(argument2, sizeof(size_t *));
			if (!region2) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region2));

			File *file = vfs.OpenFile((char *) argument0, argument1);

			if (!file) SYSCALL_RETURN(0);
			if (!file->fileSize) SYSCALL_RETURN(0);

			size_t fileSize = file->fileSize;
			uint8_t *buffer = (uint8_t *) currentVMM->Allocate(fileSize);
			if (!buffer) SYSCALL_RETURN(0);
			bool success = file->Read(0, fileSize, buffer);

			vfs.CloseFile(file);

			if (!success) {
				currentVMM->Free(buffer);
				SYSCALL_RETURN(0);
			} else {
				*((size_t *) argument2) = fileSize;
				SYSCALL_RETURN((uintptr_t) buffer);
			}
		} break;

		case OS_SYSCALL_CREATE_SHARED_MEMORY: {
			if (argument0 > OS_SHARED_MEMORY_MAXIMUM_SIZE) {
				SYSCALL_RETURN(OS_ERROR_SHARED_MEMORY_REGION_TOO_LARGE);
			}

			if (argument2 > OS_SHARED_MEMORY_NAME_MAX_LENGTH) {
				SYSCALL_RETURN(OS_ERROR_PATH_LENGTH_EXCEEDS_LIMIT);
			}

			if (argument1 && !argument2) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);

			VMMRegion *region1 = argument1 ? currentVMM->FindAndLockRegion(argument1, argument2) : nullptr;
			if (!region1 && argument1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(if (region1) currentVMM->UnlockRegion(region1));

			SharedMemoryRegion *region = sharedMemoryManager.CreateSharedMemory(argument0, (char *) argument1, argument2);
			if (!region) SYSCALL_RETURN(OS_INVALID_HANDLE);
			Handle handle = {};
			handle.type = KERNEL_OBJECT_SHMEM;
			handle.object = region;
			SYSCALL_RETURN(currentProcess->OpenHandle(handle));
		} break;

		case OS_SYSCALL_MAP_SHARED_MEMORY: {
			KernelObjectType type = KERNEL_OBJECT_SHMEM;
			SharedMemoryRegion *region = (SharedMemoryRegion *) currentProcess->ResolveHandle(argument0, type);
			if (!region) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->CompleteHandle(region, argument0));

			region->mutex.Acquire();
			region->handles++;
			region->mutex.Release();

			uintptr_t address = (uintptr_t) currentVMM->Allocate(argument2, vmmMapLazy, vmmRegionShared, argument1, VMM_REGION_FLAG_CACHABLE, region);

			if (!address) {
				CloseHandleToObject(region, KERNEL_OBJECT_SHMEM);
			}

			SYSCALL_RETURN(address);
		} break;

		case OS_SYSCALL_OPEN_NAMED_SHARED_MEMORY: {
			if (argument1 > OS_SHARED_MEMORY_NAME_MAX_LENGTH) {
				SYSCALL_RETURN(OS_ERROR_PATH_LENGTH_EXCEEDS_LIMIT);
			}

			VMMRegion *region1 = currentVMM->FindAndLockRegion(argument0, argument1);
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			SharedMemoryRegion *region = sharedMemoryManager.LookupSharedMemory((char *) argument0, argument1);
			if (!region) SYSCALL_RETURN(OS_INVALID_HANDLE);

			Handle handle = {};
			handle.type = KERNEL_OBJECT_SHMEM;
			handle.object = region;
			SYSCALL_RETURN(currentProcess->OpenHandle(handle));
		} break;
	}

	end:;

	currentThread->terminatableState = THREAD_TERMINATABLE;

	if (currentThread->terminating) {
		// The thread has been terminated.
		// Yield the scheduler so it can be removed.
		ProcessorFakeTimerInterrupt();
	}
	
	return returnValue;
}

#endif
