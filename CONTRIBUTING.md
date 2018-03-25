# Contributing Guidelines

## Code Style

Functions and structures names use `PascalCase`.
Variables use `camelCase`, while constants and macros use `SCREAMING_SNAKE_CASE`.

Tabs are `\t`, and are 8 characters in size.

Braces are placed at the end of the line: 

    if (a > b) {
        ...
    }
    
Blocks are always surrounded by newlines, and always have braces.

    int x = 5;
    
    if (x < 6) {
        x++; // Postfix operators are preferred.
    }
    
Exception: If there are lot of short, linked blocks, then they may be written like this-

    if (width == DIMENSION_PUSH) { bool a = grid->widths[i] == DIMENSION_PUSH; grid->widths[i] = DIMENSION_PUSH; if (!a) pushH++; }
    else if (grid->widths[i] < width && grid->widths[i] != DIMENSION_PUSH) grid->widths[i] = width;
    if (height == DIMENSION_PUSH) { bool a = grid->heights[j] == DIMENSION_PUSH; grid->heights[j] = DIMENSION_PUSH; if (!a) pushV++; }
    else if (grid->heights[j] < height && grid->heights[j] != DIMENSION_PUSH) grid->heights[j] = height;

Function names are always descriptive, and use prepositions and conjuctions if neccesary. 

    OSCopyToScreen // Symbols provided by the API are prefixed with OS-, or os-.
    OSDrawSurfaceClipped
    OSZeroMemory
    
Variable names are usually descriptive, but sometimes shortened names are used for short-lived variables.

    OSMessage m = {};
    m.type = OS_MESSAGE_MEASURE;
    OSCallbackResponse r = OSSendMessage(grid->objects[0], &m);
		if (r != OS_CALLBACK_HANDLED) OSCrashProcess(OS_FATAL_ERROR_MESSAGE_SHOULD_BE_HANDLED);

Operators are padded with spaces on either side.

    bounds.left = (grip->bounds.left + grip->bounds.right) / 2 - 4;

Although the operating system is written in C++, most C++ features are avoided.
However, the kernel uses a lot of member functions.

    struct Window {
        void Update(bool fromUser);
        void SetCursorStyle(OSCursorStyle style);
        void NeedWMTimer(int hz);
        void Destroy();
        bool Move(OSRectangle &newBounds);
        void ClearImage();

        Mutex mutex; // Mutex for drawing to the window. Also needed when moving the window.
        Surface *surface;
        OSPoint position;
        size_t width, height;
        ...
    }
    
Default arguments often provided as functions grow over time.

There is no limit on function size. However, you should avoid regularly exceeding 120 columns.

    static OSCallbackResponse ProcessControlMessage(OSObject _object, OSMessage *message) {
        // 300 lines later...
    }

Pointers are declared like this: `Type *name;`.
