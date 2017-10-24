uintptr_t DoSyscall(uintptr_t index,
		uintptr_t argument0, uintptr_t argument1,
		uintptr_t argument2, uintptr_t argument3);

#ifdef IMPLEMENTATION

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

	Thread *currentThread = GetCurrentThread();
	Process *currentProcess = currentThread->process;
	VMM *currentVMM = currentProcess->vmm;

	if (currentThread->terminating) {
		// The thread has been terminated.
		// Yield the scheduler so it can be removed.
		ProcessorFakeTimerInterrupt();
	}

	if (currentThread->isKernelThread) {
		KernelPanic("DoSyscall - Current thread %x was a kernel thread.\n", 
				currentThread);
	}

	if (currentThread->terminatableState != THREAD_TERMINATABLE) {
		KernelPanic("DoSyscall - Current thread %x was not terminatable (was %d).\n", 
				currentThread, currentThread->terminatableState);
	}

	currentThread->terminatableState = THREAD_IN_SYSCALL;
	// KernelLog(LOG_VERBOSE, "Thread %x now in syscall\n", currentThread);

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
			OSError error = currentVMM->Free((void *) argument0);
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
				process->handle = currentProcess->handleTable.OpenHandle(handle); 
				process->pid = processObject->id;
				process->mainThread.handle = currentProcess->handleTable.OpenHandle(handle2);
				process->mainThread.tid = processObject->executableMainThread->id;

				SYSCALL_RETURN(OS_SUCCESS);
			}
		} break;

		case OS_SYSCALL_GET_CREATION_ARGUMENT: {
			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!process) SYSCALL_RETURN(0);
			Defer(currentProcess->handleTable.CompleteHandle(process, argument0));

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
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(_handle));
		} break;

		case OS_SYSCALL_GET_LINEAR_BUFFER: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

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
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

			OSRectangle *rectangle = (OSRectangle *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) rectangle, sizeof(OSRectangle));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			surface->InvalidateRectangle(*rectangle);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_COPY_TO_SCREEN: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

			OSPoint *point = (OSPoint *) argument1;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) point, sizeof(OSPoint));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			graphics.frameBuffer.Copy(*surface, *point, OSRectangle(0, surface->resX, 0, surface->resY), true, (uint16_t) argument2);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_FILL_RECTANGLE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

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

			Surface *destination = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->handleTable.ResolveHandle(argument1, type);
			if (!source) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(source, argument1));

			OSPoint *point = (OSPoint *) argument2;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) point, sizeof(OSPoint));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			destination->Copy(*source, *point, OSRectangle(0, source->resX, 0, source->resY), true, SURFACE_COPY_WITHOUT_DEPTH_CHECKING);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_DRAW_SURFACE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;

			Surface *destination = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->handleTable.ResolveHandle(argument1, type);
			if (!source) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(source, argument1));

			_OSDrawSurfaceArguments *arguments = (_OSDrawSurfaceArguments *) argument2;
			VMMRegion *region1 = currentVMM->FindAndLockRegion((uintptr_t) arguments, sizeof(_OSDrawSurfaceArguments));
			if (!region1) SYSCALL_RETURN(OS_ERROR_INVALID_BUFFER);
			Defer(currentVMM->UnlockRegion(region1));

			destination->Draw(*source, arguments->destination, arguments->source, arguments->border, (OSDrawMode) argument3);

			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_CLEAR_MODIFIED_REGION: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *destination = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(destination, argument0));

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
				// KernelLog(LOG_VERBOSE, "Thread %x in block request\n", currentThread);
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
			Process *process = (Process *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!process) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(process, argument0));

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
				osWindow->handle = currentProcess->handleTable.OpenHandle(_handle);

				_handle.type = KERNEL_OBJECT_SURFACE;
				_handle.object = &window->surface;
				osWindow->surface = currentProcess->handleTable.OpenHandle(_handle);
				
				OSMessage message = {};
				message.type = OS_MESSAGE_WINDOW_CREATED;
				message.targetWindow = window->apiWindow;
				window->owner->SendMessage(message);

				SYSCALL_RETURN(OS_SUCCESS);
			}
		} break;

		case OS_SYSCALL_UPDATE_WINDOW: {
			KernelObjectType type = KERNEL_OBJECT_WINDOW;
			Window *window = (Window *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!window) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(window, argument0));

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
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(handle));
		} break;

		case OS_SYSCALL_ACQUIRE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!mutex) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(mutex, argument0));

			if (mutex->owner == currentThread) SYSCALL_RETURN(OS_ERROR_MUTEX_ALREADY_ACQUIRED);
			currentThread->terminatableState = THREAD_USER_BLOCK_REQUEST;
			// KernelLog(LOG_VERBOSE, "Thread %x in block request\n", currentThread);
			mutex->Acquire();
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_RELEASE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!mutex) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(mutex, argument0));

			if (mutex->owner != currentThread) SYSCALL_RETURN(OS_ERROR_MUTEX_NOT_ACQUIRED_BY_THREAD);
			mutex->Release();
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_CLOSE_HANDLE: {
			KernelObjectType type = CLOSABLE_OBJECT_TYPES; 
			void *object = currentProcess->handleTable.ResolveHandle(argument0, type, RESOLVE_HANDLE_TO_CLOSE);

			if (!object) {
				SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			}

			currentProcess->handleTable.CloseHandle(argument0);
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_TERMINATE_THREAD: {
			KernelObjectType type = KERNEL_OBJECT_THREAD;
			Thread *thread = (Thread *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!thread) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(thread, argument0));

			scheduler.TerminateThread(thread);
			SYSCALL_RETURN(OS_SUCCESS);
		} break;

		case OS_SYSCALL_TERMINATE_PROCESS: {
			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!process) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(process, argument0));

			scheduler.TerminateProcess(process);
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
				thread->handle = currentProcess->handleTable.OpenHandle(handle); 
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
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(handle));
		} break;

		case OS_SYSCALL_MAP_SHARED_MEMORY: {
			KernelObjectType type = KERNEL_OBJECT_SHMEM;
			SharedMemoryRegion *region = (SharedMemoryRegion *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!region) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(region, argument0));

			if (argument2 == OS_SHARED_MEMORY_MAP_ALL) {
				argument2 = region->sizeBytes;
			}

			uintptr_t address = (uintptr_t) currentVMM->Allocate(argument2, vmmMapLazy, vmmRegionShared, argument1, VMM_REGION_FLAG_CACHABLE, region);

			if (!address) {
				CloseHandleToObject(region, KERNEL_OBJECT_SHMEM);
			}

			SYSCALL_RETURN(address);
		} break;

		case OS_SYSCALL_SHARE_MEMORY: {
			KernelObjectType type = KERNEL_OBJECT_SHMEM;
			SharedMemoryRegion *region = (SharedMemoryRegion *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!region) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(region, argument0));

			type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->handleTable.ResolveHandle(argument1, type);
			if (!process) SYSCALL_RETURN(OS_ERROR_INVALID_HANDLE);
			Defer(currentProcess->handleTable.CompleteHandle(process, argument1));

			Handle handle = {};
			handle.type = KERNEL_OBJECT_SHMEM;
			handle.object = region;
			handle.readOnly = argument2 ? true : false;
			SYSCALL_RETURN(process->handleTable.OpenHandle(handle));
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
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(handle));
		} break;
	}

	end:;

	currentThread->terminatableState = THREAD_TERMINATABLE;
	// KernelLog(LOG_VERBOSE, "Thread %x finished syscall\n", currentThread);

	if (currentThread->terminating) {
		// The thread has been terminated.
		// Yield the scheduler so it can be removed.
		ProcessorFakeTimerInterrupt();
	}
	
	return returnValue;
}

#endif
