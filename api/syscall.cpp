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
