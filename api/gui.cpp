OSControl *OSCreateControl(OSWindow *window, OSPoint position) {
	OSControl *control = (OSControl *) OSHeapAllocate(sizeof(OSControl));
	if (!control) return nullptr;

	control->window = window;
	control->position = position;

	OSPoint drawPosition = OSPoint(5 + position.x, 29 + position.y);

	// Draw the control.
	OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, 
			OSRectangle(drawPosition.x, drawPosition.x + 80, drawPosition.y, drawPosition.y + 21),
			OSRectangle(51, 51 + 8, 88, 88 + 21),
			OSRectangle(51 + 3, 51 + 5, 88 + 10, 88 + 11),
			OS_DRAW_MODE_REPEAT_FIRST);
	OSUpdateWindow(window);

	return control;
}

OSError OSCreateWindow(OSWindow *window, size_t width, size_t height) {
	width += 8;
	height += 34;

	OSError result = OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) window, width, height, 0);

	if (result != OS_SUCCESS) {
		return result;
	}

	// Draw the window background and border.
	OSDrawSurface(window->surface, OS_SURFACE_UI_SHEET, OSRectangle(0, width, 0, height), 
			OSRectangle(96, 105, 42, 77), OSRectangle(96 + 3, 96 + 5, 42 + 29, 42 + 31), OS_DRAW_MODE_REPEAT_FIRST);
	OSUpdateWindow(window);

	return OS_SUCCESS;
}
