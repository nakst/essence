#ifdef KERNEL
uintptr_t _OSSyscall(uintptr_t argument0, uintptr_t argument1, uintptr_t argument2, 
		uintptr_t unused, uintptr_t argument3, uintptr_t argument4) {
	(void) unused;
	return DoSyscall((OSSyscallType) argument0, argument1, argument2, argument3, argument4, true, nullptr);
}
#endif

void *OSAllocate(size_t size) {
	intptr_t result = OSSyscall(OS_SYSCALL_ALLOCATE, size, 0, 0, 0);

	if (result >= 0) {
		return (void *) result;
	} else {
		return nullptr;
	}
}

OSError OSFree(void *address) {
	intptr_t result = OSSyscall(OS_SYSCALL_FREE, (uintptr_t) address, 0, 0, 0);
	return result;
}

OSError OSCreateProcess(const char *executablePath, size_t executablePathLength, OSProcessInformation *information, void *argument) {
	intptr_t result = OSSyscall(OS_SYSCALL_CREATE_PROCESS, (uintptr_t) executablePath, executablePathLength, (uintptr_t) information, (uintptr_t) argument);
	return result;
}

void *OSGetCreationArgument(OSHandle process) {
	return (void *) OSSyscall(OS_SYSCALL_GET_CREATION_ARGUMENT, process, 0, 0, 0);
}

OSHandle OSCreateSurface(size_t width, size_t height) {
	return OSSyscall(OS_SYSCALL_CREATE_SURFACE, width, height, 0, 0);
}

OSError OSGetLinearBuffer(OSHandle surface, OSLinearBuffer *linearBuffer) {
	return OSSyscall(OS_SYSCALL_GET_LINEAR_BUFFER, surface, (uintptr_t) linearBuffer, 0, 0);
}

OSError OSInvalidateRectangle(OSHandle surface, OSRectangle rectangle) {
	return OSSyscall(OS_SYSCALL_INVALIDATE_RECTANGLE, surface, (uintptr_t) &rectangle, 0, 0);
}

OSError OSCopyToScreen(OSHandle source, OSPoint point, uint16_t depth) {
	return OSSyscall(OS_SYSCALL_COPY_TO_SCREEN, source, (uintptr_t) &point, depth, 0);
}

OSError OSForceScreenUpdate() {
	return OSSyscall(OS_SYSCALL_FORCE_SCREEN_UPDATE, 0, 0, 0, 0);
}

OSError OSFillRectangle(OSHandle surface, OSRectangle rectangle, OSColor color) {
	_OSRectangleAndColor arg;
	arg.rectangle = rectangle;
	arg.color = color;

	return OSSyscall(OS_SYSCALL_FILL_RECTANGLE, surface, (uintptr_t) &arg, 0, 0);
}

OSError OSCopySurface(OSHandle destination, OSHandle source, OSPoint destinationPoint) {
	return OSSyscall(OS_SYSCALL_COPY_SURFACE, destination, source, (uintptr_t) &destinationPoint, 0);
}

OSError OSClearModifiedRegion(OSHandle surface) {
	return OSSyscall(OS_SYSCALL_CLEAR_MODIFIED_REGION, surface, 0, 0, 0);
}

OSError OSGetMessage(OSMessage *message) {
	return OSSyscall(OS_SYSCALL_GET_MESSAGE, (uintptr_t) message, 0, 0, 0);
}

OSError OSPostMessage(OSMessage *message) {
	return OSSyscall(OS_SYSCALL_POST_MESSAGE, (uintptr_t) message, 0, 0, 0);
}

OSError OSWaitMessage(uintptr_t timeoutMs) {
	return OSSyscall(OS_SYSCALL_WAIT_MESSAGE, timeoutMs, 0, 0, 0);
}

OSError OSDrawSurface(OSHandle destination, OSHandle source, OSRectangle destinationRegion, OSRectangle sourceRegion, OSRectangle borderRegion, OSDrawMode mode, uint8_t alpha) {
	_OSDrawSurfaceArguments arg;
	arg.destination = destinationRegion;
	arg.source = sourceRegion;
	arg.border = borderRegion;
	arg.alpha = alpha;

	return OSSyscall(OS_SYSCALL_DRAW_SURFACE, destination, source, (uintptr_t) &arg, mode);
}

OSError OSDrawSurfaceClipped(OSHandle destination, OSHandle source, OSRectangle destinationRegion, OSRectangle sourceRegion, OSRectangle borderRegion, OSDrawMode mode, uint8_t alpha, OSRectangle clipRegion) {
	if (clipRegion.left < destinationRegion.left) clipRegion.left = destinationRegion.left;
	if (clipRegion.right > destinationRegion.right) clipRegion.right = destinationRegion.right;
	if (clipRegion.top < destinationRegion.top) clipRegion.top = destinationRegion.top;
	if (clipRegion.bottom > destinationRegion.bottom) clipRegion.bottom = destinationRegion.bottom;

	sourceRegion.left += clipRegion.left - destinationRegion.left;
	sourceRegion.right += clipRegion.right - destinationRegion.right;
	sourceRegion.top += clipRegion.top - destinationRegion.top;
	sourceRegion.bottom += clipRegion.bottom - destinationRegion.bottom;

	if (sourceRegion.left > borderRegion.left) sourceRegion.left = borderRegion.left;
	if (sourceRegion.right < borderRegion.right) sourceRegion.right = borderRegion.right;
	if (sourceRegion.top > borderRegion.top) sourceRegion.top = borderRegion.top;
	if (sourceRegion.bottom < borderRegion.bottom) sourceRegion.bottom = borderRegion.bottom;

	return OSDrawSurface(destination, source, clipRegion, sourceRegion, borderRegion, mode, alpha);
}

OSHandle OSCreateMutex() {
	return OSSyscall(OS_SYSCALL_CREATE_MUTEX, 0, 0, 0, 0);
}

OSHandle OSCreateEvent(bool autoReset) {
	return OSSyscall(OS_SYSCALL_CREATE_EVENT, autoReset, 0, 0, 0);
}

OSError OSSetEvent(OSHandle handle) {
	return OSSyscall(OS_SYSCALL_SET_EVENT, handle, 0, 0, 0);
}

OSError OSResetEvent(OSHandle handle) {
	return OSSyscall(OS_SYSCALL_RESET_EVENT, handle, 0, 0, 0);
}

OSError OSPollEvent(OSHandle handle) {
	return OSSyscall(OS_SYSCALL_POLL_EVENT, handle, 0, 0, 0);
}

OSError OSAcquireMutex(OSHandle handle) {
	return OSSyscall(OS_SYSCALL_ACQUIRE_MUTEX, handle, 0, 0, 0);
}

OSError OSReleaseMutex(OSHandle handle) {
	return OSSyscall(OS_SYSCALL_RELEASE_MUTEX, handle, 0, 0, 0);
}

OSError OSCloseHandle(OSHandle handle) {
	return OSSyscall(OS_SYSCALL_CLOSE_HANDLE, handle, 0, 0, 0);
}

OSError OSTerminateThread(OSHandle thread) {
	return OSSyscall(OS_SYSCALL_TERMINATE_THREAD, thread, 0, 0, 0);
}

OSError OSTerminateProcess(OSHandle process) {
	return OSSyscall(OS_SYSCALL_TERMINATE_PROCESS, process, 0, 0, 0);
}

OSError OSTerminateThisProcess() {
	return OSSyscall(OS_SYSCALL_TERMINATE_PROCESS, OS_CURRENT_PROCESS, 0, 0, 0);
}

OSError OSCreateThread(OSThreadEntryFunction entryFunction, OSThreadInformation *information, void *argument) {
	return OSSyscall(OS_SYSCALL_CREATE_THREAD, (uintptr_t) entryFunction, 0, (uintptr_t) information, (uintptr_t) argument);
}

void *OSReadEntireFile(const char *filePath, size_t filePathLength, size_t *fileSize) {
	OSNodeInformation information;

	if (OS_SUCCESS != OSOpenNode((char *) filePath, filePathLength, OS_OPEN_NODE_READ_ACCESS, &information)) {
		return nullptr;
	}

	*fileSize = information.fileSize;
	void *buffer = OSHeapAllocate(information.fileSize, false);

	if (information.fileSize != OSReadFileSync(information.handle, 0, information.fileSize, buffer)) {
		OSHeapFree(buffer);
		buffer = nullptr;
	}

	OSCloseHandle(information.handle);
	return buffer;
}

OSHandle OSOpenSharedMemory(size_t size, char *name, size_t nameLength, unsigned flags) {
	return OSSyscall(OS_SYSCALL_OPEN_SHARED_MEMORY, size, (uintptr_t) name, nameLength, flags);
}

OSHandle OSShareMemory(OSHandle sharedMemoryRegion, OSHandle targetProcess, bool readOnly) {
	return OSSyscall(OS_SYSCALL_SHARE_MEMORY, sharedMemoryRegion, targetProcess, readOnly, 0);
}

void *OSMapObject(OSHandle sharedMemoryRegion, uintptr_t offset, size_t size, unsigned flags) {
	intptr_t result = OSSyscall(OS_SYSCALL_MAP_OBJECT, sharedMemoryRegion, offset, size, flags);

	if (result >= 0) {
		return (void *) result;
	} else {
		return nullptr;
	}
}

OSError OSOpenNode(char *path, size_t pathLength, uint64_t flags, OSNodeInformation *information) {
	intptr_t result = OSSyscall(OS_SYSCALL_OPEN_NODE, (uintptr_t) path, pathLength, flags, (uintptr_t) information);
	return result;
}

size_t OSReadFileSync(OSHandle handle, uint64_t offset, size_t size, void *buffer) {
	intptr_t result = OSSyscall(OS_SYSCALL_READ_FILE_SYNC, handle, offset, size, (uintptr_t) buffer);
	return result;
}

size_t OSWriteFileSync(OSHandle handle, uint64_t offset, size_t size, void *buffer) {
	intptr_t result = OSSyscall(OS_SYSCALL_WRITE_FILE_SYNC, handle, offset, size, (uintptr_t) buffer);
	return result;
}

OSHandle OSReadFileAsync(OSHandle handle, uint64_t offset, size_t size, void *buffer) {
	intptr_t result = OSSyscall(OS_SYSCALL_READ_FILE_ASYNC, handle, offset, size, (uintptr_t) buffer);
	return result;
}

OSHandle OSWriteFileAsync(OSHandle handle, uint64_t offset, size_t size, void *buffer) {
	intptr_t result = OSSyscall(OS_SYSCALL_WRITE_FILE_ASYNC, handle, offset, size, (uintptr_t) buffer);
	return result;
}

OSError OSResizeFile(OSHandle handle, uint64_t newSize) {
	return OSSyscall(OS_SYSCALL_RESIZE_FILE, handle, newSize, 0, 0);
}

uintptr_t OSWait(OSHandle *handles, size_t count, uintptr_t timeoutMs) {
	return OSSyscall(OS_SYSCALL_WAIT, (uintptr_t) handles, count, timeoutMs, 0);
}

void OSRefreshNodeInformation(OSNodeInformation *information) {
	OSSyscall(OS_SYSCALL_REFRESH_NODE_INFORMATION, (uintptr_t) information, 0, 0, 0);
}

void OSRedrawAll() {
	OSSyscall(OS_SYSCALL_REDRAW_ALL, 0, 0, 0, 0);
}

void OSPauseProcess(OSHandle process, bool resume) {
	OSSyscall(OS_SYSCALL_PAUSE_PROCESS, process, resume, 0, 0);
}

void OSCrashProcess(OSError error) {
	OSSyscall(OS_SYSCALL_CRASH_PROCESS, error, 0, 0, 0);
}

uintptr_t OSGetThreadID(OSHandle thread) {
	return OSSyscall(OS_SYSCALL_GET_THREAD_ID, thread, 0, 0, 0);
}

OSError OSEnumerateDirectoryChildren(OSHandle directory, OSDirectoryChild *buffer, size_t size) {
	return OSSyscall(OS_SYSCALL_ENUMERATE_DIRECTORY_CHILDREN, directory, (uintptr_t) buffer, size, 0);
}

void OSGetIORequestProgress(OSHandle ioRequest, OSIORequestProgress *buffer) {
	OSSyscall(OS_SYSCALL_GET_IO_REQUEST_PROGRESS, ioRequest, (uintptr_t) buffer, 0, 0);
}

void OSCancelIORequest(OSHandle ioRequest) {
	OSSyscall(OS_SYSCALL_CANCEL_IO_REQUEST, ioRequest, 0, 0, 0);
}

void OSBatch(OSBatchCall *calls, size_t count) {
#if 0
	for (uintptr_t i = 0; i < count; i++) {
		OSBatchCall *call = calls + i;
		// ... modify system call for version changes ... 
	}
#endif

	OSSyscall(OS_SYSCALL_BATCH, (uintptr_t) calls, count, 0, 0);
}

void OSRemoveNodeFromParent(OSHandle node) {
	OSSyscall(OS_SYSCALL_REMOVE_NODE_FROM_PARENT, node, 0, 0, 0);
}
