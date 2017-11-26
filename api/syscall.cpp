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

OSError OSSendMessage(OSHandle process, OSMessage *message) {
	return OSSyscall(OS_SYSCALL_SEND_MESSAGE, process, (uintptr_t) message, 0, 0);
}

OSError OSWaitMessage(uintptr_t timeoutMs) {
	return OSSyscall(OS_SYSCALL_WAIT_MESSAGE, timeoutMs, 0, 0, 0);
}

OSError OSUpdateWindow(OSWindow *window) {
	return OSSyscall(OS_SYSCALL_UPDATE_WINDOW, window->handle, 0, 0, 0);
}

OSError OSDrawSurface(OSHandle destination, OSHandle source, OSRectangle destinationRegion, OSRectangle sourceRegion, OSRectangle borderRegion, OSDrawMode mode) {
	_OSDrawSurfaceArguments arg;
	arg.destination = destinationRegion;
	arg.source = sourceRegion;
	arg.border = borderRegion;

	return OSSyscall(OS_SYSCALL_DRAW_SURFACE, destination, source, (uintptr_t) &arg, mode);
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

	if (OS_SUCCESS != OSOpenNode((char *) filePath, filePathLength, OS_OPEN_NODE_ACCESS_READ, &information)) {
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

OSHandle OSOpenNamedSharedMemory(char *name, size_t nameLength) {
	return OSSyscall(OS_SYSCALL_OPEN_NAMED_SHARED_MEMORY, (uintptr_t) name, nameLength, 0, 0);
}

OSHandle OSCreateSharedMemory(size_t size, char *name, size_t nameLength) {
	return OSSyscall(OS_SYSCALL_CREATE_SHARED_MEMORY, size, (uintptr_t) name, nameLength, 0);
}

OSHandle OSShareMemory(OSHandle sharedMemoryRegion, OSHandle targetProcess, bool readOnly) {
	return OSSyscall(OS_SYSCALL_SHARE_MEMORY, sharedMemoryRegion, targetProcess, readOnly, 0);
}

void *OSMapSharedMemory(OSHandle sharedMemoryRegion, uintptr_t offset, size_t size) {
	intptr_t result = OSSyscall(OS_SYSCALL_MAP_SHARED_MEMORY, sharedMemoryRegion, offset, size, 0);

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

OSError OSResizeFile(OSHandle handle, uint64_t newSize) {
	return OSSyscall(OS_SYSCALL_RESIZE_FILE, handle, newSize, 0, 0);
}

uintptr_t OSWait(OSHandle *handles, size_t count, uintptr_t timeoutMs) {
	return OSSyscall(OS_SYSCALL_WAIT, (uintptr_t) handles, count, timeoutMs, 0);
}

OSError OSRefreshNodeInformation(OSNodeInformation *information) {
	return OSSyscall(OS_SYSCALL_REFRESH_NODE_INFORMATION, (uintptr_t) information, 0, 0, 0);
}

OSError OSSetCursorStyle(OSHandle window, OSCursorStyle style) {
	return OSSyscall(OS_SYSCALL_SET_CURSOR_STYLE, (uintptr_t) window, (uintptr_t) style, 0, 0);
}

OSError OSMoveWindow(OSHandle window, OSRectangle newBounds) {
	return OSSyscall(OS_SYSCALL_MOVE_WINDOW, (uintptr_t) window, (uintptr_t) &newBounds, 0, 0);
}

OSError OSGetWindowBounds(OSHandle window, OSRectangle *rectangle) {
	return OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, (uintptr_t) window, (uintptr_t) rectangle, 0, 0);
}
