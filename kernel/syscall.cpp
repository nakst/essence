uintptr_t DoSyscall(uintptr_t index,
		uintptr_t argument0, uintptr_t argument1,
		uintptr_t argument2, uintptr_t argument3);

#ifdef IMPLEMENTATION

void Process::CloseHandle(OSHandle handle) {
	handleTable.lock.Acquire();
	Defer(handleTable.lock.Release());

	uintptr_t l1Index = ((handle / HANDLE_TABLE_L3_ENTRIES) / HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l2Index = ((handle / HANDLE_TABLE_L3_ENTRIES) % HANDLE_TABLE_L2_ENTRIES);
	uintptr_t l3Index = ((handle % HANDLE_TABLE_L3_ENTRIES));

	HandleTableL1 *l1 = &handleTable;
	HandleTableL2 *l2 = l1->t[l1Index];
	HandleTableL3 *l3 = l2->t[l2Index];

	Handle *_handle = l3->t + l3Index;
	ZeroMemory(_handle, sizeof(Handle));

	l1->u[l1Index]--;
	l2->u[l2Index]--;
}

void *Process::ResolveHandle(OSHandle handle, KernelObjectType &type, ResolveHandleReason reason) {
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
				return _handle->object;
			}
		} else if (reason == RESOLVE_HANDLE_TO_CLOSE) {
			if (_handle->lock) {
				return nullptr; // The handle was locked and can't be closed.
			} else {
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
		l1->t[l1Index] = (HandleTableL2 *) kernelVMM.Allocate(sizeof(HandleTableL2));
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
		l2->t[l2Index] = (HandleTableL3 *) kernelVMM.Allocate(sizeof(HandleTableL3));
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

	switch (index) {
		case OS_SYSCALL_PRINT: {
			VMMRegion *region = currentVMM->FindAndLockRegion(argument0, argument1);
			if (!region) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region));
			Print("%s", argument1, argument0);
			return OS_SUCCESS;
	     	} break;

		case OS_SYSCALL_ALLOCATE: {
			uintptr_t address = (uintptr_t) currentVMM->Allocate(argument0);
			return address;
		} break;

		case OS_SYSCALL_FREE: {
			OSError error = currentVMM->Free((void *) argument0);
			return error;
		} break;

		case OS_SYSCALL_CREATE_PROCESS: {
			if (argument1 > MAX_PATH) return OS_ERROR_PATH_LENGTH_EXCEEDS_LIMIT;

			VMMRegion *region1 = currentVMM->FindAndLockRegion(argument0, argument1);
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			VMMRegion *region2 = currentVMM->FindAndLockRegion(argument2, sizeof(OSProcessInformation));
			if (!region2) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region2));

			OSProcessInformation *process = (OSProcessInformation *) argument2;
			Process *processObject = scheduler.SpawnProcess((char *) argument0, argument1, false, (void *) argument3);

			if (!processObject) {
				return OS_ERROR_UNKNOWN_OPERATION_FAILURE;
			} else {
				Handle handle = {};
				handle.type = KERNEL_OBJECT_PROCESS;
				handle.object = processObject;

				// Register processObject as a handle.
				process->handle = currentProcess->OpenHandle(handle); 
				process->pid = processObject->id;

				return OS_SUCCESS;
			}
		} break;

		case OS_SYSCALL_GET_CREATION_ARGUMENT: {
			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->ResolveHandle(argument0, type);
			if (!process) return 0;
			Defer(currentProcess->CompleteHandle(process, argument0));

			uintptr_t creationArgument = (uintptr_t) process->creationArgument;
			return creationArgument;
		} break;

		case OS_SYSCALL_CREATE_SURFACE: {
			Surface *surface = (Surface *) graphics.surfacePool.Add();
			if (!surface->Initialise(currentVMM, argument0, argument1, false)) {
				return OS_INVALID_HANDLE;
			}

			Handle _handle = {};
			_handle.type = KERNEL_OBJECT_SURFACE;
			_handle.object = surface;
			// TODO Prevent the program from deallocating the surface's linear buffer.
			return currentProcess->OpenHandle(_handle);
		} break;

		case OS_SYSCALL_GET_LINEAR_BUFFER: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(surface, argument0));

			OSLinearBuffer *linearBuffer = (OSLinearBuffer *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) linearBuffer, sizeof(OSLinearBuffer));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			linearBuffer->width = surface->resX;
			linearBuffer->height = surface->resY;
			linearBuffer->buffer = surface->linearBuffer;
			linearBuffer->colorFormat = OS_COLOR_FORMAT_32_XRGB;

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_INVALIDATE_RECTANGLE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(surface, argument0));

			OSRectangle *rectangle = (OSRectangle *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) rectangle, sizeof(OSRectangle));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			surface->InvalidateRectangle(*rectangle);

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_COPY_TO_SCREEN: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(surface, argument0));

			OSPoint *point = (OSPoint *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) point, sizeof(OSPoint));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			graphics.frameBuffer.Copy(*surface, *point, OSRectangle(0, surface->resX, 0, surface->resY), true, (uint16_t) argument2);

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_FILL_RECTANGLE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!surface) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(surface, argument0));

			_OSRectangleAndColor *arg = (_OSRectangleAndColor *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) arg, sizeof(_OSRectangleAndColor));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			surface->FillRectangle(arg->rectangle, arg->color);

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_FORCE_SCREEN_UPDATE: {
			graphics.UpdateScreen();
		} break;

		case OS_SYSCALL_COPY_SURFACE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;

			Surface *destination = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!destination) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->ResolveHandle(argument1, type);
			if (!source) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(source, argument1));

			OSPoint *point = (OSPoint *) argument2;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) point, sizeof(OSPoint));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			destination->Copy(*source, *point, OSRectangle(0, source->resX, 0, source->resY), true, SURFACE_COPY_WITHOUT_DEPTH_CHECKING);

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_DRAW_SURFACE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;

			Surface *destination = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!destination) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->ResolveHandle(argument1, type);
			if (!source) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(source, argument1));

			_OSDrawSurfaceArguments *arguments = (_OSDrawSurfaceArguments *) argument2;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) arguments, sizeof(_OSDrawSurfaceArguments));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			destination->Draw(*source, arguments->destination, arguments->source, arguments->border, (OSDrawMode) argument3);

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_CLEAR_MODIFIED_REGION: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *destination = (Surface *) currentProcess->ResolveHandle(argument0, type);
			if (!destination) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(destination, argument0));

			destination->mutex.Acquire();
			destination->ClearModifiedRegion();
			destination->mutex.Release();

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_GET_MESSAGE: {
			OSMessage *returnMessage = (OSMessage *) argument0;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) returnMessage, sizeof(OSMessage));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
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
					return OS_ERROR_NO_MESSAGES_AVAILABLE;
				}
			}

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_WAIT_MESSAGE: {
			// TODO OSTerminateThread needs to be able to wake up a thread here.

			while (!currentProcess->messageQueue.count) {
				currentProcess->messageQueueIsNotEmpty.Wait(argument0 /*Timeout*/);
			}

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_SEND_MESSAGE: {
			OSMessage *message = (OSMessage *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) message, sizeof(OSMessage));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->ResolveHandle(argument0, type);
			if (!process) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(process, argument0));

			{
				currentProcess->messageQueueMutex.Acquire();
				Defer(currentProcess->messageQueueMutex.Release());

				return process->SendMessage(*message) ? OS_SUCCESS : OS_ERROR_MESSAGE_QUEUE_FULL;
			}

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_CREATE_WINDOW: {
			OSWindow *osWindow = (OSWindow *) argument0;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) osWindow, sizeof(OSWindow));
			if (!region1) return OS_ERROR_INVALID_BUFFER;
			Defer(currentVMM->UnlockRegion(region1));

			Window *window = windowManager.CreateWindow(currentProcess, argument1, argument2);

			if (!window) {
				return OS_ERROR_UNKNOWN_OPERATION_FAILURE;
			} else {
				Handle _handle = {};

				_handle.type = KERNEL_OBJECT_WINDOW;
				_handle.object = window;
				osWindow->handle = currentProcess->OpenHandle(_handle);

				_handle.type = KERNEL_OBJECT_SURFACE;
				_handle.object = &window->surface;
				osWindow->surface = currentProcess->OpenHandle(_handle);
				
				return OS_SUCCESS;
			}
		} break;

		case OS_SYSCALL_UPDATE_WINDOW: {
			KernelObjectType type = KERNEL_OBJECT_WINDOW;
			Window *window = (Window *) currentProcess->ResolveHandle(argument0, type);
			if (!window) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(window, argument0));

			window->Update();

			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_CREATE_MUTEX: {
			Mutex *mutex = (Mutex *) scheduler.globalMutexPool.Add();
			mutex->handles = 1;
			if (!mutex) return OS_ERROR_UNKNOWN_OPERATION_FAILURE;
			Handle handle = {};
			handle.type = KERNEL_OBJECT_MUTEX;
			handle.object = mutex;
			return currentProcess->OpenHandle(handle);
		} break;

		case OS_SYSCALL_ACQUIRE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->ResolveHandle(argument0, type);
			if (!mutex) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(mutex, argument0));

			// TODO OSTerminateThread needs to be able to wake up a thread here.
			if (mutex->owner == currentThread) return OS_ERROR_MUTEX_ACQUIRED_BY_THREAD;
			mutex->Acquire();
			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_RELEASE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->ResolveHandle(argument0, type);
			if (!mutex) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(mutex, argument0));

			if (mutex->owner != currentThread) return OS_ERROR_MUTEX_NOT_ACQUIRED_BY_THREAD;
			mutex->Release();
			return OS_SUCCESS;
		} break;

		case OS_SYSCALL_CLOSE_HANDLE: {
			KernelObjectType type = (KernelObjectType) (KERNEL_OBJECT_MUTEX | KERNEL_OBJECT_PROCESS); 
			void *object = currentProcess->ResolveHandle(argument0, type, RESOLVE_HANDLE_TO_CLOSE);

			if (!object) {
				return OS_ERROR_INVALID_HANDLE;
			}

			currentProcess->CloseHandle(argument0);

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

				default: {
					KernelPanic("DoSyscall - Cannot close object of type %d.\n", type);
				} break;
			}
		} break;

		case OS_SYSCALL_TERMINATE_THREAD: {
			KernelObjectType type = KERNEL_OBJECT_THREAD;
			Thread *thread = (Thread *) currentProcess->ResolveHandle(argument0, type);
			if (!thread) return OS_ERROR_INVALID_HANDLE;
			Defer(currentProcess->CompleteHandle(thread, argument0));

			scheduler.RemoveThread(thread);
			return OS_SUCCESS;
		} break;
	}
	
	return OS_ERROR_UNKNOWN_SYSCALL;
}

#endif
