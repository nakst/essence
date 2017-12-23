// TODO Replace OS_ERROR_UNKNOWN_OPERATION_FAILURE with proper errors.
// TODO Clean up the return values for system calls; with FATAL_ERRORs there should need to be less error codes returned.

uintptr_t DoSyscall(uintptr_t index,
		uintptr_t argument0, uintptr_t argument1,
		uintptr_t argument2, uintptr_t argument3,
		bool fromKernel);

#ifdef IMPLEMENTATION

bool MessageQueue::SendMessage(OSMessage &_message) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (count == MESSAGE_QUEUE_MAX_LENGTH) {
		return false;
	}

	if (_message.type == OS_MESSAGE_MOUSE_MOVED) {
		if (mouseMovedMessage) {
			CopyMemory(messages + mouseMovedMessage - 1, &_message, sizeof(OSMessage));
			goto done;
		} else {
			mouseMovedMessage = count + 1;
		}
	}

	if (count + 1 >= allocated) {
		allocated = (allocated + 8) * 2;
		OSMessage *old = messages;
		messages = (OSMessage *) OSHeapAllocate(allocated * sizeof(OSMessage), false);
		CopyMemory(messages, old, count * sizeof(OSMessage));
		OSHeapFree(old);
	}

	CopyMemory(messages + count, &_message, sizeof(OSMessage));
	count++;

	done:;

	if (!notEmpty.Poll()) {
		notEmpty.Set();
	}

	return true;
}

bool MessageQueue::GetMessage(OSMessage &_message) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (!count) {
		return false;
	}

	CopyMemory(&_message, messages, sizeof(OSMessage));
	CopyMemory(messages, messages + 1, (count - 1) * sizeof(OSMessage));
	count--;

	if (mouseMovedMessage) {
		mouseMovedMessage--;
	}

	if (!count) {
		notEmpty.Reset();
	}

	return true;
}

uintptr_t DoSyscall(uintptr_t index,
		uintptr_t argument0, uintptr_t argument1,
		uintptr_t argument2, uintptr_t argument3,
		bool fromKernel) {
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

	if (!fromKernel) {
		if (currentThread->terminatableState != THREAD_TERMINATABLE) {
			KernelPanic("DoSyscall - Current thread %x was not terminatable (was %d).\n", 
					currentThread, currentThread->terminatableState);
		}

		currentThread->terminatableState = THREAD_IN_SYSCALL;
	} else {
		currentProcess = kernelProcess;
	}

	OSError returnValue = OS_FATAL_ERROR_UNKNOWN_SYSCALL;
	bool fatalError = true;

#define SYSCALL_RETURN(value, fatal) {returnValue = value; fatalError = fatal; goto end;}

#define SYSCALL_BUFFER(address, length, index) \
	VMMRegion *region ## index = currentVMM->FindAndLockRegion((address), (length)); \
	if (!region ## index && !fromKernel) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_BUFFER, true); \
	Defer(if (region ## index) currentVMM->UnlockRegion(region ## index));
#define SYSCALL_BUFFER_ALLOW_NULL(address, length, index) \
	VMMRegion *region ## index = currentVMM->FindAndLockRegion((address), (length)); \
	Defer(if (region ## index) currentVMM->UnlockRegion(region ## index));

	switch (index) {
		case OS_SYSCALL_PRINT: {
			SYSCALL_BUFFER(argument0, argument1, 1);
			Print("%s", argument1, argument0);
			SYSCALL_RETURN(OS_SUCCESS, false);
	     	} break;

		case OS_SYSCALL_ALLOCATE: {
			uintptr_t address = (uintptr_t) currentVMM->Allocate("UserReq", argument0);
			SYSCALL_RETURN(address, false);
		} break;

		case OS_SYSCALL_FREE: {
			OSError error = currentVMM->Free((void *) argument0);
			SYSCALL_RETURN(error, false);
		} break;

		case OS_SYSCALL_CREATE_PROCESS: {
			if (argument1 > MAX_PATH) SYSCALL_RETURN(OS_FATAL_ERROR_PATH_LENGTH_EXCEEDS_LIMIT, true);

			SYSCALL_BUFFER(argument0, argument1, 1);
			SYSCALL_BUFFER(argument2, sizeof(OSProcessInformation), 2);

			OSProcessInformation *process = (OSProcessInformation *) argument2;
			Process *processObject = scheduler.SpawnProcess((char *) argument0, argument1, false, (void *) argument3);

			if (!processObject) {
				SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE, false);
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

				SYSCALL_RETURN(OS_SUCCESS, false);
			}
		} break;

		case OS_SYSCALL_GET_CREATION_ARGUMENT: {
			KernelObjectType type = (KernelObjectType) (KERNEL_OBJECT_PROCESS | KERNEL_OBJECT_WINDOW);
			void *object = currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!object) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(object, argument0));

			uintptr_t creationArgument;

			switch (type) {
				case KERNEL_OBJECT_PROCESS: {
					creationArgument = (uintptr_t) ((Process *) object)->creationArgument;
				} break;

				case KERNEL_OBJECT_WINDOW: {
					creationArgument = (uintptr_t) ((Window *) object)->apiWindow;
				} break;

				default: {
					KernelPanic("DoSyscall - Invalid creation argument object type.\n");
				} break;
			}

			SYSCALL_RETURN(creationArgument, false);
		} break;

		case OS_SYSCALL_CREATE_SURFACE: {
			Surface *surface = (Surface *) graphics.surfacePool.Add();
			if (!surface->Initialise(argument0, argument1, false)) {
				SYSCALL_RETURN(OS_INVALID_HANDLE, true);
			}

			Handle _handle = {};
			_handle.type = KERNEL_OBJECT_SURFACE;
			_handle.object = surface;
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(_handle), true);
		} break;

		case OS_SYSCALL_GET_LINEAR_BUFFER: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

			OSLinearBuffer *linearBuffer = (OSLinearBuffer *) argument1;
			SYSCALL_BUFFER(argument1, sizeof(OSLinearBuffer), 1);

			surface->region->mutex.Acquire();
			surface->region->handles++;
			surface->region->mutex.Release();

			Handle handle;
			handle.type = KERNEL_OBJECT_SHMEM;
			handle.object = surface->region;

			linearBuffer->handle = currentProcess->handleTable.OpenHandle(handle);
			linearBuffer->width = surface->resX;
			linearBuffer->height = surface->resY;
			linearBuffer->stride = surface->stride;
			linearBuffer->colorFormat = OS_COLOR_FORMAT_32_XRGB;

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_INVALIDATE_RECTANGLE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

			OSRectangle *rectangle = (OSRectangle *) argument1;
			SYSCALL_BUFFER(argument1, sizeof(OSRectangle), 1);

			surface->InvalidateRectangle(*rectangle);

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_COPY_TO_SCREEN: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

			OSPoint *point = (OSPoint *) argument1;
			SYSCALL_BUFFER(argument1, sizeof(OSPoint), 1);

			graphics.frameBuffer.Copy(*surface, *point, OSRectangle(0, surface->resX, 0, surface->resY), true, (uint16_t) argument2);

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_FILL_RECTANGLE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *surface = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!surface) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(surface, argument0));

			_OSRectangleAndColor *arg = (_OSRectangleAndColor *) argument1;
			SYSCALL_BUFFER(argument1, sizeof(_OSRectangleAndColor), 1);

			surface->FillRectangle(arg->rectangle, arg->color);

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_FORCE_SCREEN_UPDATE: {
			graphics.UpdateScreen();
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_COPY_SURFACE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;

			Surface *destination = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->handleTable.ResolveHandle(argument1, type);
			if (!source) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(source, argument1));

			OSPoint *point = (OSPoint *) argument2;
			SYSCALL_BUFFER(argument2, sizeof(OSPoint), 1);

			destination->Copy(*source, *point, OSRectangle(0, source->resX, 0, source->resY), true, SURFACE_COPY_WITHOUT_DEPTH_CHECKING);

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_DRAW_SURFACE: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;

			Surface *destination = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(destination, argument0));

			Surface *source = (Surface *) currentProcess->handleTable.ResolveHandle(argument1, type);
			if (!source) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(source, argument1));

			_OSDrawSurfaceArguments *arguments = (_OSDrawSurfaceArguments *) argument2;
			SYSCALL_BUFFER(argument2, sizeof(_OSDrawSurfaceArguments), 1);

			destination->Draw(*source, arguments->destination, arguments->source, arguments->border, (OSDrawMode) argument3);

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_CLEAR_MODIFIED_REGION: {
			KernelObjectType type = KERNEL_OBJECT_SURFACE;
			Surface *destination = (Surface *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!destination) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(destination, argument0));

			destination->mutex.Acquire();
			destination->ClearModifiedRegion();
			destination->mutex.Release();

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_GET_MESSAGE: {
			OSMessage *returnMessage = (OSMessage *) argument0;
			SYSCALL_BUFFER(argument0, sizeof(OSMessage), 1);

			if (currentProcess->messageQueue.GetMessage(*returnMessage)) {
				SYSCALL_RETURN(OS_SUCCESS, false);
			} else {
				SYSCALL_RETURN(OS_ERROR_NO_MESSAGES_AVAILABLE, false);
			}
		} break;

		case OS_SYSCALL_WAIT_MESSAGE: {
			while (!currentProcess->messageQueue.count) {
				currentThread->terminatableState = THREAD_USER_BLOCK_REQUEST;
				// KernelLog(LOG_VERBOSE, "Thread %x in block request\n", currentThread);
				
				if (!currentProcess->messageQueue.notEmpty.Wait(argument0 /*Timeout*/)) {
					break;
				}
			}

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_CREATE_WINDOW: {
			OSHandle *returnData = (OSHandle *) argument0;
			SYSCALL_BUFFER(argument0, sizeof(OSHandle) * 2, 1);

			OSRectangle *bounds = (OSRectangle *) argument1;
			SYSCALL_BUFFER(argument1, sizeof(OSRectangle), 2);

			Window *window = windowManager.CreateWindow(currentProcess, *bounds, returnData);

			if (!window) {
				SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE, false);
			} else {
				Handle _handle = {};

				_handle.type = KERNEL_OBJECT_WINDOW;
				_handle.object = window;
				returnData[0] = currentProcess->handleTable.OpenHandle(_handle);

				_handle.type = KERNEL_OBJECT_SURFACE;
				_handle.object = window->surface;
				returnData[1] = currentProcess->handleTable.OpenHandle(_handle);
				
				OSMessage message = {};
				message.type = OS_MESSAGE_WINDOW_CREATED;
				message.targetWindow = window->apiWindow;
				window->owner->messageQueue.SendMessage(message);

				SYSCALL_RETURN(OS_SUCCESS, false);
			}
		} break;

		case OS_SYSCALL_UPDATE_WINDOW: {
			KernelObjectType type = KERNEL_OBJECT_WINDOW;
			Window *window = (Window *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!window) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(window, argument0));

			window->mutex.Acquire();
			window->Update();
			window->mutex.Release();

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_CREATE_EVENT: {
			Event *event = (Event *) OSHeapAllocate(sizeof(Event), true);
			if (!event) SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE, false);
			event->handles = 1;
			event->autoReset = argument0;
			Handle handle = {};
			handle.type = KERNEL_OBJECT_EVENT;
			handle.object = event;
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(handle), false);
		} break;

		case OS_SYSCALL_CREATE_MUTEX: {
			Mutex *mutex = (Mutex *) OSHeapAllocate(sizeof(Mutex), true);
			if (!mutex) SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE, false);
			mutex->handles = 1;
			Handle handle = {};
			handle.type = KERNEL_OBJECT_MUTEX;
			handle.object = mutex;
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(handle), false);
		} break;

		case OS_SYSCALL_ACQUIRE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!mutex) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(mutex, argument0));

			if (mutex->owner == currentThread) SYSCALL_RETURN(OS_FATAL_ERROR_MUTEX_ALREADY_ACQUIRED, true);
			currentThread->terminatableState = THREAD_USER_BLOCK_REQUEST;
			// KernelLog(LOG_VERBOSE, "Thread %x in block request\n", currentThread);
			mutex->Acquire();
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_RELEASE_MUTEX: {
			KernelObjectType type = KERNEL_OBJECT_MUTEX;
			Mutex *mutex = (Mutex *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!mutex) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(mutex, argument0));

			if (mutex->owner != currentThread) SYSCALL_RETURN(OS_FATAL_ERROR_MUTEX_NOT_ACQUIRED_BY_THREAD, true);
			mutex->Release();
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_CLOSE_HANDLE: {
			KernelObjectType type = CLOSABLE_OBJECT_TYPES; 
			void *object = currentProcess->handleTable.ResolveHandle(argument0, type, RESOLVE_HANDLE_TO_CLOSE);

			if (!object) {
				SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			}

			currentProcess->handleTable.CloseHandle(argument0);
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_TERMINATE_THREAD: {
			KernelObjectType type = KERNEL_OBJECT_THREAD;
			Thread *thread = (Thread *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!thread) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(thread, argument0));

			scheduler.TerminateThread(thread);
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_TERMINATE_PROCESS: {
			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!process) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(process, argument0));

			scheduler.TerminateProcess(process);
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_CREATE_THREAD: {
			SYSCALL_BUFFER(argument2, sizeof(OSThreadInformation), 1);

			OSThreadInformation *thread = (OSThreadInformation *) argument2;
			Thread *threadObject = scheduler.SpawnThread(argument0, argument3, currentProcess, true, true);

			if (!threadObject) {
				SYSCALL_RETURN(OS_ERROR_UNKNOWN_OPERATION_FAILURE, false);
			} else {
				Handle handle = {};
				handle.type = KERNEL_OBJECT_THREAD;
				handle.object = threadObject;

				// Register processObject as a handle.
				thread->handle = currentProcess->handleTable.OpenHandle(handle); 
				thread->tid = threadObject->id;

				SYSCALL_RETURN(OS_SUCCESS, false);
			}
		} break;

		case OS_SYSCALL_CREATE_SHARED_MEMORY: {
			if (argument0 > OS_SHARED_MEMORY_MAXIMUM_SIZE) {
				SYSCALL_RETURN(OS_FATAL_ERROR_SHARED_MEMORY_REGION_TOO_LARGE, true);
			}

			if (argument2 > OS_SHARED_MEMORY_NAME_MAX_LENGTH) {
				SYSCALL_RETURN(OS_FATAL_ERROR_PATH_LENGTH_EXCEEDS_LIMIT, true);
			}

			if (argument1 && !argument2) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_BUFFER, true);

			SYSCALL_BUFFER_ALLOW_NULL(argument1, argument2, 1);

			SharedMemoryRegion *region = sharedMemoryManager.CreateSharedMemory(argument0, (char *) argument1, argument2);
			if (!region) SYSCALL_RETURN(OS_INVALID_HANDLE, false);
			Handle handle = {};
			handle.type = KERNEL_OBJECT_SHMEM;
			handle.object = region;
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(handle), false);
		} break;

		case OS_SYSCALL_MAP_SHARED_MEMORY: {
			KernelObjectType type = KERNEL_OBJECT_SHMEM;
			SharedMemoryRegion *region = (SharedMemoryRegion *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!region) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(region, argument0));

			if (argument2 == OS_SHARED_MEMORY_MAP_ALL) {
				argument2 = region->sizeBytes;
			}

			uintptr_t address = (uintptr_t) currentVMM->Allocate("UserReq", argument2, vmmMapLazy, vmmRegionShared, argument1, VMM_REGION_FLAG_CACHABLE, region);

			if (!address) {
				CloseHandleToObject(region, KERNEL_OBJECT_SHMEM);
			}

			SYSCALL_RETURN(address, false);
		} break;

		case OS_SYSCALL_SHARE_MEMORY: {
			KernelObjectType type = KERNEL_OBJECT_SHMEM;
			SharedMemoryRegion *region = (SharedMemoryRegion *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!region) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(region, argument0));

			type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->handleTable.ResolveHandle(argument1, type);
			if (!process) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(process, argument1));

			region->mutex.Acquire();
			region->handles++;
			region->mutex.Release();

			Handle handle = {};
			handle.type = KERNEL_OBJECT_SHMEM;
			handle.object = region;
			handle.readOnly = argument2 ? true : false;
			SYSCALL_RETURN(process->handleTable.OpenHandle(handle), false);
		} break;

		case OS_SYSCALL_OPEN_NAMED_SHARED_MEMORY: {
			if (argument1 > OS_SHARED_MEMORY_NAME_MAX_LENGTH) {
				SYSCALL_RETURN(OS_FATAL_ERROR_PATH_LENGTH_EXCEEDS_LIMIT, true);
			}

			SYSCALL_BUFFER(argument0, argument1, 1);

			SharedMemoryRegion *region = sharedMemoryManager.LookupSharedMemory((char *) argument0, argument1);
			if (!region) SYSCALL_RETURN(OS_INVALID_HANDLE, false);

			Handle handle = {};
			handle.type = KERNEL_OBJECT_SHMEM;
			handle.object = region;
			SYSCALL_RETURN(currentProcess->handleTable.OpenHandle(handle), false);
		} break;

		case OS_SYSCALL_OPEN_NODE: {
			SYSCALL_BUFFER(argument0, argument1, 1);
			SYSCALL_BUFFER(argument3, sizeof(OSNodeInformation), 2);

			char *path = (char *) argument0;
			size_t pathLength = (size_t) argument1;
			uint64_t flags = (uint64_t) argument2;

			OSError error;
			Node *node = vfs.OpenNode(path, pathLength, flags, &error);

			if (!node) {
				SYSCALL_RETURN(error, false);
			}

			Handle handle = {};
			handle.type = KERNEL_OBJECT_NODE;
			handle.object = node;
			handle.flags = flags;

			OSNodeInformation *information = (OSNodeInformation *) argument3;
			node->CopyInformation(information);
			information->handle = currentProcess->handleTable.OpenHandle(handle);

			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_READ_FILE_SYNC: {
			KernelObjectType type = KERNEL_OBJECT_NODE;
			Handle *handleData;
			Node *file = (Node *) currentProcess->handleTable.ResolveHandle(argument0, type, RESOLVE_HANDLE_TO_USE, &handleData);
			if (!file) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(file, argument0));

			if (file->data.type != OS_NODE_FILE) SYSCALL_RETURN(OS_FATAL_ERROR_INCORRECT_NODE_TYPE, true);

			SYSCALL_BUFFER(argument3, argument2, 1);

			if (handleData->flags & OS_OPEN_NODE_ACCESS_READ) {
#if 0
				OSError error;
				size_t bytesRead = file->Read(argument1, argument2, (uint8_t *) argument3, &error);
#else
				IORequest *request = (IORequest *) OSHeapAllocate(sizeof(IORequest), true);
				request->type = IO_REQUEST_READ;
				request->node = file;
				request->offset = argument1;
				request->count = argument2;
				request->buffer = (void *) argument3;
				ioManager.AddRequest(request);
				request->complete.Wait(OS_WAIT_NO_TIMEOUT);
				size_t bytesRead = request->doneCount;
				OSError error = request->error;
				OSHeapFree(request);
#endif
				SYSCALL_RETURN(bytesRead ? bytesRead : error, false);
			} else {
				SYSCALL_RETURN(OS_FATAL_ERROR_INCORRECT_FILE_ACCESS, true);
			}
		} break;

		case OS_SYSCALL_WRITE_FILE_SYNC: {
			KernelObjectType type = KERNEL_OBJECT_NODE;
			Handle *handleData;
			Node *file = (Node *) currentProcess->handleTable.ResolveHandle(argument0, type, RESOLVE_HANDLE_TO_USE, &handleData);
			if (!file) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(file, argument0));

			if (file->data.type != OS_NODE_FILE) SYSCALL_RETURN(OS_FATAL_ERROR_INCORRECT_NODE_TYPE, true);

			SYSCALL_BUFFER(argument3, argument2, 1);

			if (handleData->flags & OS_OPEN_NODE_ACCESS_WRITE) {
				OSError error;
				size_t bytesWritten = file->Write(argument1, argument2, (uint8_t *) argument3, &error);
				SYSCALL_RETURN(bytesWritten ? bytesWritten : error, false);
			} else {
				SYSCALL_RETURN(OS_FATAL_ERROR_INCORRECT_FILE_ACCESS, true);
			}
		} break;

		case OS_SYSCALL_RESIZE_FILE: {
			KernelObjectType type = KERNEL_OBJECT_NODE;
			Handle *handleData;
			Node *file = (Node *) currentProcess->handleTable.ResolveHandle(argument0, type, RESOLVE_HANDLE_TO_USE, &handleData);
			if (!file) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(file, argument0));

			if (file->data.type != OS_NODE_FILE) SYSCALL_RETURN(OS_FATAL_ERROR_INCORRECT_NODE_TYPE, true);

			if (handleData->flags & OS_OPEN_NODE_ACCESS_RESIZE) {
				bool success = file->Resize(argument1);
				SYSCALL_RETURN(success ? OS_SUCCESS : OS_ERROR_UNKNOWN_OPERATION_FAILURE, false);
			} else {
				SYSCALL_RETURN(OS_FATAL_ERROR_INCORRECT_FILE_ACCESS, true);
			}
		} break;
					     
		case OS_SYSCALL_SET_EVENT: {
			KernelObjectType type = KERNEL_OBJECT_EVENT;
			Event *event = (Event *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!event) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(event, argument0));

			event->Set();
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_RESET_EVENT: {
			KernelObjectType type = KERNEL_OBJECT_EVENT;
			Event *event = (Event *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!event) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(event, argument0));

			event->Reset();
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_POLL_EVENT: {
			KernelObjectType type = KERNEL_OBJECT_EVENT;
			Event *event = (Event *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!event) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(event, argument0));

			bool eventWasSet = event->Poll();
			SYSCALL_RETURN(eventWasSet ? OS_SUCCESS : OS_ERROR_EVENT_NOT_SET, false);
		} break;

		case OS_SYSCALL_WAIT: {
			if (argument1 >= OS_MAX_WAIT_COUNT) {
				SYSCALL_RETURN(OS_FATAL_ERROR_TOO_MANY_WAIT_OBJECTS, true);
			}

			SYSCALL_BUFFER(argument0, argument1 * sizeof(OSHandle), 1);

			OSHandle *_handles = (OSHandle *) argument0;
			volatile OSHandle handles[OS_MAX_WAIT_COUNT];
			CopyMemory((void *) handles, _handles, argument1 * sizeof(OSHandle));

			Event *events[OS_MAX_WAIT_COUNT];
			void *objects[OS_MAX_WAIT_COUNT];

			KernelObjectType waitableObjectTypes = (KernelObjectType) (KERNEL_OBJECT_PROCESS
								| KERNEL_OBJECT_THREAD
								| KERNEL_OBJECT_EVENT);

			for (uintptr_t i = 0; i < argument1; i++) {
				KernelObjectType type = waitableObjectTypes;
				void *object = (void *) currentProcess->handleTable.ResolveHandle(handles[i], type);

				if (!object) {
					for (uintptr_t j = 0; j < i; j++) {
						currentProcess->handleTable.CompleteHandle(objects[j], handles[j]);
					}

					SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
				}

				objects[i] = object;

				switch (type) {
					case KERNEL_OBJECT_PROCESS: {
						events[i] = &((Process *) object)->killedEvent;
					} break;

					case KERNEL_OBJECT_THREAD: {
						events[i] = &((Thread *) object)->killedEvent;
					} break;

					case KERNEL_OBJECT_EVENT: {
						events[i] = (Event *) object;
					} break;

					default: {
						KernelPanic("DoSyscall - Unexpected wait object type %d.\n", type);
					} break;
				}
			}

			size_t waitObjectCount = argument1;
			Timer *timer = nullptr;

			if (argument2 != (uintptr_t) OS_WAIT_NO_TIMEOUT) {
				Timer _timer = {};
				_timer.Set(argument2, false);
				events[waitObjectCount++] = &_timer.event;
				timer = &_timer;
			}

			uintptr_t waitReturnValue;
			currentThread->terminatableState = THREAD_USER_BLOCK_REQUEST;
			waitReturnValue = scheduler.WaitEvents(events, waitObjectCount);

			if (waitReturnValue == argument1) {
				waitReturnValue = OS_ERROR_TIMEOUT_REACHED;
			}

			for (uintptr_t i = 0; i < argument1; i++) {
				currentProcess->handleTable.CompleteHandle(objects[i], handles[i]);
			}

			if (timer) {
				timer->Remove();
			}

			SYSCALL_RETURN(waitReturnValue, false);
		} break;

		case OS_SYSCALL_REFRESH_NODE_INFORMATION: {
			SYSCALL_BUFFER(argument0, sizeof(OSNodeInformation), 1);

			OSNodeInformation *information = (OSNodeInformation *) argument0;

			volatile OSHandle handle = information->handle;
			KernelObjectType type = KERNEL_OBJECT_NODE;
			Node *node = (Node *) currentProcess->handleTable.ResolveHandle(handle, type);
			if (!node) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(node, handle));

			node->CopyInformation((OSNodeInformation *) argument0);
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_SET_CURSOR_STYLE: {
			KernelObjectType type = KERNEL_OBJECT_WINDOW;
			Window *window = (Window *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!window) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(window, argument0));

			window->SetCursorStyle((OSCursorStyle) argument1);
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_MOVE_WINDOW: {
			KernelObjectType type = KERNEL_OBJECT_WINDOW;
			Window *window = (Window *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!window) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(window, argument0));

			OSRectangle *rectangle = (OSRectangle *) argument1;
			SYSCALL_BUFFER(argument1, sizeof(OSRectangle), 1);

			window->Move(*rectangle);
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_GET_WINDOW_BOUNDS: {
			KernelObjectType type = KERNEL_OBJECT_WINDOW;
			Window *window = (Window *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!window) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(window, argument0));

			OSRectangle *rectangle = (OSRectangle *) argument1;
			SYSCALL_BUFFER(argument1, sizeof(OSRectangle), 1);

			windowManager.mutex.Acquire();
			rectangle->left = window->position.x;
			rectangle->top = window->position.y;
			rectangle->right = window->position.x + window->width;
			rectangle->bottom = window->position.y + window->height;
			windowManager.mutex.Release();
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_REDRAW_ALL: {
			windowManager.mutex.Acquire();
			windowManager.Redraw(OSPoint(0, 0), graphics.frameBuffer.resX, graphics.frameBuffer.resY, nullptr);
			windowManager.mutex.Release();
			graphics.UpdateScreen();
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;
					    
		case OS_SYSCALL_PAUSE_PROCESS: {
			KernelObjectType type = KERNEL_OBJECT_PROCESS;
			Process *process = (Process *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!process) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(process, argument0));

			scheduler.PauseProcess(process, (bool) argument1);
			SYSCALL_RETURN(OS_SUCCESS, false);
		} break;

		case OS_SYSCALL_CRASH_PROCESS: {
			KernelLog(LOG_WARNING, "process crash request, reason %d\n", argument0);
			SYSCALL_RETURN(argument0, true);
		} break;

		case OS_SYSCALL_POST_MESSAGE: {
			OSMessage *message = (OSMessage *) argument0;
			SYSCALL_BUFFER(argument0, sizeof(OSMessage), 1);

			if (currentProcess->messageQueue.SendMessage(*message)) {
				SYSCALL_RETURN(OS_SUCCESS, false);
			} else {
				SYSCALL_RETURN(OS_ERROR_MESSAGE_QUEUE_FULL, false);
			}
		} break;

		case OS_SYSCALL_GET_THREAD_ID: {
			KernelObjectType type = KERNEL_OBJECT_THREAD;
			Thread *thread = (Thread *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!thread) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(thread, argument0));

			SYSCALL_RETURN(thread->id, false);
		} break;

		case OS_SYSCALL_ENUMERATE_DIRECTORY_CHILDREN: {
			KernelObjectType type = KERNEL_OBJECT_NODE;
			Node *node = (Node *) currentProcess->handleTable.ResolveHandle(argument0, type);
			if (!node) SYSCALL_RETURN(OS_FATAL_ERROR_INVALID_HANDLE, true);
			Defer(currentProcess->handleTable.CompleteHandle(node, argument0));
			
			if (node->data.type != OS_NODE_DIRECTORY) SYSCALL_RETURN(OS_FATAL_ERROR_INCORRECT_NODE_TYPE, true);

			SYSCALL_BUFFER(argument1, argument2 * sizeof(OSDirectoryChild), 1);

			bool success = node->EnumerateChildren((OSDirectoryChild *) argument1, argument2);

			if (success) {
				SYSCALL_RETURN(OS_SUCCESS, false);
			} else {
				SYSCALL_RETURN(OS_ERROR_BUFFER_TOO_SMALL, false);
			}
		} break;
	}

	end:;

	if (fatalError) {
		OSCrashReason reason;
		reason.errorCode = returnValue;
		KernelLog(LOG_WARNING, "Process crashed during system call\n");
		scheduler.CrashProcess(currentProcess, reason);
	}

	if (!fromKernel) {
		currentThread->terminatableState = THREAD_TERMINATABLE;
	}

	if (currentThread->terminating) {
		// The thread has been terminated.
		// Yield the scheduler so it can be removed.
		ProcessorFakeTimerInterrupt();
	}
	
	return returnValue;
}

#endif
