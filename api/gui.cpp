#include "../bin/os/standard.manifest.h"

#define DIMENSION_PUSH (65535)

#define CARET_BLINK_HZ (1)
#define CARET_BLINK_PAUSE (2)

#define RESIZE_LEFT 		(1)
#define RESIZE_RIGHT 		(2)
#define RESIZE_TOP 		(4)
#define RESIZE_BOTTOM 		(8)
#define RESIZE_TOP_LEFT 	(5)
#define RESIZE_TOP_RIGHT 	(6)
#define RESIZE_BOTTOM_LEFT 	(9)
#define RESIZE_BOTTOM_RIGHT 	(10)
#define RESIZE_MOVE		(0)

#define SCROLLBAR_SIZE (17)
#define SCROLLBAR_BUTTON_AMOUNT (64)

#define STANDARD_BACKGROUND_COLOR (0xF5F6F9)
#define STANDARD_BORDER_SIZE (2)

#define LIST_VIEW_MARGIN (10)
#define LIST_VIEW_TEXT_MARGIN (6)
#define LIST_VIEW_WITH_BORDER_MARGIN (LIST_VIEW_MARGIN + STANDARD_BORDER_SIZE)
#define LIST_VIEW_ROW_HEIGHT (20)

// TODO Calculator textbox - selection extends out of top of textbox
// TODO Minor menu[bar] border adjustments; menu icons.
// TODO Keyboard controls.
// TODO Send repeat messages for held left press? Scrollbar buttons, scrollbar nudges, scroll-selections, etc.
// TODO Minimum scrollbar grip size.
// TODO Minimum size is smaller than expected?
// TODO Timer messages seem to be buggy?
// TODO Memory "arenas".

struct UIImage {
	OSRectangle region;
	OSRectangle border;
};

#if 0
#define SUPER_COOL_BUTTONS
#endif

static UIImage activeWindowBorder11	= {{1, 1 + 6, 144, 144 + 6}, 	{1, 1, 144, 144}};
static UIImage activeWindowBorder12	= {{8, 8 + 1, 144, 144 + 6}, 	{8, 9, 144, 144}};
static UIImage activeWindowBorder13	= {{10, 10 + 6, 144, 144 + 6}, 	{10, 10, 144, 144}};
static UIImage activeWindowBorder21	= {{1, 1 + 6, 151, 151 + 24}, 	{1, 1, 151, 151}};
static UIImage activeWindowBorder22	= {{8, 8 + 1, 151, 151 + 24}, 	{8, 9, 151, 151}};
static UIImage activeWindowBorder23	= {{10, 10 + 6, 151, 151 + 24},	{10, 10, 151, 151}};
static UIImage activeWindowBorder31	= {{1, 1 + 6, 176, 176 + 1}, 	{1, 1, 176, 177}};
static UIImage activeWindowBorder33	= {{10, 10 + 6, 176, 176 + 1}, 	{10, 10, 176, 177}};
static UIImage activeWindowBorder41	= {{1, 1 + 6, 178, 178 + 6}, 	{1, 1, 178, 178}};
static UIImage activeWindowBorder42	= {{8, 8 + 1, 178, 178 + 6}, 	{8, 9, 178, 178}};
static UIImage activeWindowBorder43	= {{10, 10 + 6, 178, 178 + 6}, 	{10, 10, 178, 178}};

static UIImage inactiveWindowBorder11	= {{16 + 1, 16 + 1 + 6, 144, 144 + 6}, 	{16 + 1, 16 + 1, 144, 144}};
static UIImage inactiveWindowBorder12	= {{16 + 8, 16 + 8 + 1, 144, 144 + 6}, 	{16 + 8, 16 + 9, 144, 144}};
static UIImage inactiveWindowBorder13	= {{16 + 10, 16 + 10 + 6, 144, 144 + 6},{16 + 10, 16 + 10, 144, 144}};
static UIImage inactiveWindowBorder21	= {{16 + 1, 16 + 1 + 6, 151, 151 + 24}, {16 + 1, 16 + 1, 151, 151}};
static UIImage inactiveWindowBorder22	= {{16 + 8, 16 + 8 + 1, 151, 151 + 24}, {16 + 8, 16 + 9, 151, 151}};
static UIImage inactiveWindowBorder23	= {{16 + 10, 16 + 10 + 6, 151, 151 + 24},{16 + 10, 16 + 10, 151, 151}};
static UIImage inactiveWindowBorder31	= {{16 + 1, 16 + 1 + 6, 176, 176 + 1}, 	{16 + 1, 16 + 1, 176, 177}};
static UIImage inactiveWindowBorder33	= {{16 + 10, 16 + 10 + 6, 176, 176 + 1}, {16 + 10, 16 + 10, 176, 177}};
static UIImage inactiveWindowBorder41	= {{16 + 1, 16 + 1 + 6, 178, 178 + 6}, 	{16 + 1, 16 + 1, 178, 178}};
static UIImage inactiveWindowBorder42	= {{16 + 8, 16 + 8 + 1, 178, 178 + 6}, 	{16 + 8, 16 + 9, 178, 178}};
static UIImage inactiveWindowBorder43	= {{16 + 10, 16 + 10 + 6, 178, 178 + 6}, {16 + 10, 16 + 10, 178, 178}};

static UIImage progressBarBackground 	= {{1, 8, 122, 143}, {3, 6, 125, 139}};
static UIImage progressBarDisabled   	= {{9, 16, 122, 143}, {11, 14, 125, 139}};
static UIImage progressBarPellet     	= {{18, 26, 128, 143}, {18, 18, 128, 128}};

#ifndef SUPER_COOL_BUTTONS
static UIImage buttonNormal		= {{51, 59, 88, 109}, {51 + 3, 51 + 5, 88 + 10, 88 + 11}};
static UIImage buttonDragged		= {{9 + 51, 9 + 59, 88, 109}, {9 + 54, 9 + 56, 98, 99}};
static UIImage buttonHover		= {{-9 + 51, -9 + 59, 88, 109}, {-9 + 54, -9 + 56, 98, 99}};
static UIImage buttonDisabled		= {{18 + 51, 18 + 59, 88, 109}, {18 + 54, 18 + 56, 98, 99}};
static UIImage buttonDangerousDragged	= {{24, 32, 105, 126}, {24 + 3, 24 + 5, 105 + 10, 105 + 11}};
static UIImage buttonDangerousHover	= {{-18 + 51, -18 + 59, 88, 109}, {-18 + 54, -18 + 56, 98, 99}};
#else
static UIImage buttonNormal		= {{86 + 76, 86 + 76 + 86, 194, 194 + 29}, {86 + 76 + 43, 86 + 76 + 44, 194 + 14, 194 + 15}};
static UIImage buttonDragged		= {{76, 76 + 86, 29 + 194, 29 + 194 + 29}, {76 + 43, 76 + 44, 29 + 194 + 14, 29 + 194 + 15}};
static UIImage buttonHover		= {{76, 76 + 86, 194, 194 + 29}, {76 + 43, 76 + 44, 194 + 14, 194 + 15}};
static UIImage buttonDisabled		= {{86 + 76, 86 + 76 + 86, 29 + 194, 29 + 194 + 29}, {86 + 76 + 43, 86 + 76 + 44, 29 + 194 + 14, 29 + 194 + 15}};
#endif

static UIImage checkboxNormal		= {{95, 108, 120, 133}, {95, 95, 120, 120}};
static UIImage checkboxDragged		= {{14 + 95, 14 + 108, 120, 133}, {14 + 95, 14 + 95, 120, 120}};
static UIImage checkboxHover		= {{-14 + 95, -14 + 108, 120, 133}, {-14 + 95, -14 + 95, 120, 120}};
static UIImage checkboxDisabled		= {{28 + 95, 28 + 108, 120, 133}, {28 + 95, 28 + 95, 120, 120}};
static UIImage checkboxNormalChecked	= {{95, 108, 14 + 120, 14 + 133}, {95, 95, 14 + 120, 14 + 120}};
static UIImage checkboxDraggedChecked	= {{14 + 95, 14 + 108, 14 + 120, 14 + 133}, {14 + 95, 14 + 95, 14 + 120, 14 + 120}};
static UIImage checkboxHoverChecked	= {{-14 + 95, -14 + 108, 14 + 120, 14 + 133}, {-14 + 95, -14 + 95, 14 + 120, 14 + 120}};
static UIImage checkboxDisabledChecked	= {{28 + 95, 28 + 108, 14 + 120, 14 + 133}, {28 + 95, 28 + 95, 14 + 120, 14 + 120}};

static UIImage textboxNormal		= {{52, 61, 166, 189}, {55, 58, 169, 186}};
static UIImage textboxFocus		= {{11 + 52, 11 + 61, 166, 189}, {11 + 55, 11 + 58, 169, 186}};
static UIImage textboxHover		= {{-11 + 52, -11 + 61, 166, 189}, {-11 + 55, -11 + 58, 169, 186}};
static UIImage textboxDisabled		= {{22 + 52, 22 + 61, 166, 189}, {22 + 55, 22 + 58, 169, 186}};

static UIImage gridBox 			= {{1, 7, 17, 23}, {3, 4, 19, 20}};
static UIImage menuBox 			= {{1, 32, 1, 15}, {28, 30, 4, 6}};
static UIImage menubarBackground	= {{34, 40, 124, 145}, {35, 38, 124, 124}};

static UIImage menuItemHover		= {{42, 50, 142, 159}, {45, 46, 151, 152}};
static UIImage menuItemDragged		= {{18 + 42, 18 + 50, 142, 159}, {18 + 45, 18 + 46, 151, 152}};

static UIImage scrollbarTrackHorizontalEnabled  = {{121, 122, 62, 79}, {121, 122, 62, 62}};
static UIImage scrollbarTrackHorizontalPressed  = {{117, 118, 62, 79}, {117, 118, 62, 62}};
static UIImage scrollbarTrackHorizontalDisabled = {{119, 120, 62, 79}, {119, 120, 62, 62}};
static UIImage scrollbarTrackVerticalEnabled    = {{174, 191, 82, 83}, {174, 174, 82, 83}};
static UIImage scrollbarTrackVerticalPressed    = {{174, 191, 84, 85}, {174, 174, 84, 85}};
static UIImage scrollbarTrackVerticalDisabled   = {{174, 191, 80, 81}, {174, 174, 80, 81}};
static UIImage scrollbarButtonHorizontalNormal  = {{159, 166, 62, 79}, {162, 163, 62, 62}};
static UIImage scrollbarButtonHorizontalHover   = {{167, 174, 62, 79}, {170, 171, 62, 62}};
static UIImage scrollbarButtonHorizontalPressed = {{175, 182, 62, 79}, {178, 179, 62, 62}};
static UIImage scrollbarButtonVerticalNormal    = {{141, 158, 62, 69}, {141, 141, 65, 66}};
static UIImage scrollbarButtonVerticalHover     = {{141, 158, 70, 77}, {141, 141, 73, 74}};
static UIImage scrollbarButtonVerticalPressed   = {{141, 158, 78, 85}, {141, 141, 81, 82}};
static UIImage scrollbarButtonDisabled          = {{183, 190, 62, 79}, {186, 187, 62, 62}};
static UIImage scrollbarNotchesHorizontal       = {{159, 164, 80, 88}, {159, 159, 80, 80}};
static UIImage scrollbarNotchesVertical         = {{165, 173, 80, 85}, {165, 165, 80, 80}};

#if 0
static UIImage resizePad               = {{123, 140, 62, 79}, {123, 123, 62, 62}};
#endif

static UIImage smallArrowUpNormal      = {{206, 217, 21, 30}, {206, 206, 21, 21}};
static UIImage smallArrowUpHover       = {{218, 229, 21, 30}, {218, 218, 21, 21}};
static UIImage smallArrowUpPressed     = {{230, 241, 21, 30}, {230, 230, 21, 21}};
static UIImage smallArrowUpDisabled    = {{242, 253, 21, 30}, {242, 242, 21, 21}};
static UIImage smallArrowDownNormal    = {{206, 217, 31, 40}, {206, 206, 31, 31}};
static UIImage smallArrowDownHover     = {{218, 229, 31, 40}, {218, 218, 31, 31}};
static UIImage smallArrowDownPressed   = {{230, 241, 31, 40}, {230, 230, 31, 31}};
static UIImage smallArrowDownDisabled  = {{242, 253, 31, 40}, {242, 242, 31, 31}};
static UIImage smallArrowLeftNormal    = {{208, 217, 39, 50}, {208, 208, 39, 39}};
static UIImage smallArrowLeftHover     = {{208, 217, 51, 62}, {208, 208, 51, 51}};
static UIImage smallArrowLeftPressed   = {{208, 217, 63, 74}, {208, 208, 63, 63}};
static UIImage smallArrowLeftDisabled  = {{208, 217, 75, 86}, {208, 208, 75, 75}};
static UIImage smallArrowRightNormal   = {{218, 227, 39, 50}, {218, 218, 39, 39}};
static UIImage smallArrowRightHover    = {{218, 227, 51, 62}, {218, 218, 51, 51}};
static UIImage smallArrowRightPressed  = {{218, 227, 63, 74}, {218, 218, 63, 63}};
static UIImage smallArrowRightDisabled = {{218, 227, 75, 86}, {218, 218, 75, 75}};

static UIImage listViewHighlight    = {{228, 241, 59, 72}, {228 + 6, 228 + 7, 59 + 6, 59 + 7}};
static UIImage listViewSelected     = {{14 + 228, 14 + 241, 59, 72}, {14 + 228 + 6, 14 + 228 + 7, 59 + 6, 59 + 7}};
static UIImage listViewSelected2    = {{14 + 228, 14 + 241, 28 + 59, 28 + 72}, {14 + 228 + 6, 14 + 228 + 7, 28 + 59 + 6, 28 + 59 + 7}};
static UIImage listViewLastClicked  = {{14 + 228, 14 + 241, 59 - 14, 72 - 14}, {14 + 228 + 6, 14 + 228 + 7, 59 + 6 - 14, 59 + 7 - 14}};
static UIImage listViewSelectionBox = {{14 + 228 - 14, 14 + 231 - 14, 42 + 59 - 14, 42 + 62 - 14}, {14 + 228 + 1 - 14, 14 + 228 + 2 - 14, 42 + 59 + 1 - 14, 42 + 59 + 2 - 14}};

static UIImage lineHorizontal		= {{40, 52, 114, 118}, {41, 42, 114, 114}};
static UIImage *lineHorizontalBackgrounds[] = { &lineHorizontal, &lineHorizontal, &lineHorizontal, &lineHorizontal, };
static UIImage lineVertical		= {{34, 38, 110, 122}, {34, 34, 111, 112}};
static UIImage *lineVerticalBackgrounds[] = { &lineVertical, &lineVertical, &lineVertical, &lineVertical, };

struct UIImage *smallArrowUpIcons[] = { &smallArrowUpNormal, &smallArrowUpDisabled, &smallArrowUpHover, &smallArrowUpPressed, };
struct UIImage *smallArrowDownIcons[] = { &smallArrowDownNormal, &smallArrowDownDisabled, &smallArrowDownHover, &smallArrowDownPressed, };
struct UIImage *smallArrowLeftIcons[] = { &smallArrowLeftNormal, &smallArrowLeftDisabled, &smallArrowLeftHover, &smallArrowLeftPressed, };
struct UIImage *smallArrowRightIcons[] = { &smallArrowRightNormal, &smallArrowRightDisabled, &smallArrowRightHover, &smallArrowRightPressed, };

struct UIImage *scrollbarTrackVerticalBackgrounds[] = { &scrollbarTrackVerticalEnabled, &scrollbarTrackVerticalDisabled, &scrollbarTrackVerticalEnabled, &scrollbarTrackVerticalPressed, };
struct UIImage *scrollbarTrackHorizontalBackgrounds[] = { &scrollbarTrackHorizontalEnabled, &scrollbarTrackHorizontalDisabled, &scrollbarTrackHorizontalEnabled, &scrollbarTrackHorizontalPressed, };

static UIImage *scrollbarButtonHorizontalBackgrounds[] = {
	&scrollbarButtonHorizontalNormal,
	&scrollbarButtonDisabled,
	&scrollbarButtonHorizontalHover,
	&scrollbarButtonHorizontalPressed,
};

static UIImage *scrollbarButtonVerticalBackgrounds[] = {
	&scrollbarButtonVerticalNormal,
	&scrollbarButtonDisabled,
	&scrollbarButtonVerticalHover,
	&scrollbarButtonVerticalPressed,
};

#if 0
static UIImage *scrollbarResizePadBackgrounds[] = {
	&scrollbarResizePad,
	&scrollbarResizePad,
	&scrollbarResizePad,
	&scrollbarResizePad,
};
#endif

static UIImage *menuItemBackgrounds[] = {
	nullptr,
	nullptr,
	&menuItemHover,
	&menuItemDragged,
};

static UIImage *checkboxIcons[] = {
	&checkboxNormal, 
	&checkboxDisabled, 
	&checkboxHover, 
	&checkboxDragged,
};

static UIImage *checkboxIconsChecked[] = {
	&checkboxNormalChecked, 
	&checkboxDisabledChecked, 
	&checkboxHoverChecked, 
	&checkboxDraggedChecked,
};

static UIImage *textboxBackgrounds[] = {
	&textboxNormal,
	&textboxDisabled,
	&textboxHover,
	&textboxFocus,
};

static UIImage *buttonBackgrounds[] = {
	&buttonNormal,
	&buttonDisabled,
	&buttonHover,
	&buttonDragged,
};

static UIImage *buttonDangerousBackgrounds[] = {
	&buttonNormal,
	&buttonDisabled,
	&buttonDangerousHover,
	&buttonDangerousDragged,
};

static UIImage *progressBarBackgrounds[] = {
	&progressBarBackground,
	&progressBarDisabled,
	&progressBarBackground,
	&progressBarBackground,
};

static UIImage *windowBorder11[] = {&activeWindowBorder11, &inactiveWindowBorder11, &activeWindowBorder11, &activeWindowBorder11};
static UIImage *windowBorder12[] = {&activeWindowBorder12, &inactiveWindowBorder12, &activeWindowBorder12, &activeWindowBorder12};
static UIImage *windowBorder13[] = {&activeWindowBorder13, &inactiveWindowBorder13, &activeWindowBorder13, &activeWindowBorder13};
static UIImage *windowBorder21[] = {&activeWindowBorder21, &inactiveWindowBorder21, &activeWindowBorder21, &activeWindowBorder21};
static UIImage *windowBorder22[] = {&activeWindowBorder22, &inactiveWindowBorder22, &activeWindowBorder22, &activeWindowBorder22};
static UIImage *windowBorder23[] = {&activeWindowBorder23, &inactiveWindowBorder23, &activeWindowBorder23, &activeWindowBorder23};
static UIImage *windowBorder31[] = {&activeWindowBorder31, &inactiveWindowBorder31, &activeWindowBorder31, &activeWindowBorder31};
static UIImage *windowBorder33[] = {&activeWindowBorder33, &inactiveWindowBorder33, &activeWindowBorder33, &activeWindowBorder33};
static UIImage *windowBorder41[] = {&activeWindowBorder41, &inactiveWindowBorder41, &activeWindowBorder41, &activeWindowBorder41};
static UIImage *windowBorder42[] = {&activeWindowBorder42, &inactiveWindowBorder42, &activeWindowBorder42, &activeWindowBorder42};
static UIImage *windowBorder43[] = {&activeWindowBorder43, &inactiveWindowBorder43, &activeWindowBorder43, &activeWindowBorder43};

static const int totalBorderWidth = 6 + 6;
static const int totalBorderHeight = 6 + 24 + 6;

struct GUIObject : APIObject {
	OSRectangle bounds, cellBounds, inputBounds;
	uint16_t descendentInvalidationFlags;
	uint16_t layout;
	uint16_t preferredWidth, preferredHeight;
	uint16_t minimumWidth, minimumHeight;
	bool verbose;
};

static inline void SetParentDescendentInvalidationFlags(GUIObject *object, uint16_t mask) {
	do {
		object->descendentInvalidationFlags |= mask;
		object = (GUIObject *) object->parent;
	} while (object);
}

#define DESCENDENT_REPAINT  (1)
#define DESCENDENT_RELAYOUT (2)

struct Control : GUIObject {
	// Current size: ~260 bytes.
	// Is this too big?
	
	struct Window *window;
		
#define UI_IMAGE_NORMAL (0)
#define UI_IMAGE_DISABLED (1)
#define UI_IMAGE_HOVER (2)
#define UI_IMAGE_DRAG (3)
	UIImage **backgrounds, **icons;

	void *context;

	OSCommand *command;
	LinkedItem<Control> commandItem;

	OSCallback notificationCallback;
	OSMenuSpecification *rightClickMenu;

	OSString text;
	OSRectangle textBounds;
	uint32_t textColor;
	uint8_t textSize, textAlign;

	uint32_t textShadow : 1, 
		textBold : 1,
		repaint : 1, 
		relayout : 1,
		noAnimations : 1,
		focusable : 1,
		customTextRendering : 1,
		drawParentBackground : 1,
		noDisabledTextColorChange : 1,
		keepCustomCursorWhenDisabled : 1,
		disabled : 1, 
		isChecked : 1, 
		checkable : 1,
		ignoreActivationClicks : 1,
		checkboxIcons : 1,
		centerIcons : 1,
		firstPaint : 1,
		cursor : 5;

	LinkedItem<Control> timerControlItem;

	uint8_t timerHz, timerStep;
	uint8_t animationStep, finalAnimationStep;
	uint8_t from1, from2, from3, from4;
	uint8_t current1, current2, current3, current4;
};

struct ListView : Control {
	unsigned flags;
	size_t itemCount;
	OSObject scrollbar;
	int scrollY;
	uintptr_t highlightRow, lastClickedRow;

	OSPoint selectionBoxAnchor;
	OSPoint selectionBoxPosition;
	OSRectangle selectionBox;
	int selectionBoxFirstRow, selectionBoxLastRow;
	
	enum {
		DRAGGING_NONE,
		DRAGGING_ITEMS,
		DRAGGING_SELECTION,
	} dragging;
};

struct MenuItem : Control {
	OSMenuItem item;
	Window *child;
	bool menubar;
};

struct Textbox : Control {
	OSCaret caret, caret2;
	OSCaret wordSelectionAnchor, wordSelectionAnchor2;
	uint8_t caretBlink : 1;
};

struct ProgressBar : Control {
	int minimum, maximum, value;
};

struct WindowResizeControl : Control {
	unsigned direction;
};

struct Grid : GUIObject {
	struct Window *window;
	unsigned columns, rows;
	GUIObject **objects;
	int *widths, *heights;
	int *minimumWidths, *minimumHeights;
	uint8_t relayout : 1, repaint : 1;
	unsigned flags;
	int borderSize, gapSize;
	UIImage *background;
	uint32_t backgroundColor;
	OSCallback notificationCallback;
	int xOffset, yOffset;
};

struct Scrollbar : Grid {
	bool enabled;
	bool orientation;

	int contentSize;
	int viewportSize;
	int height;

	int anchor;
	int position;
	int maxPosition;
	int size;
};

struct CommandWindow {
	// Data each window stores for each command specified in the program's manifest.
	LinkedList<Control> controls; // Controls bound to this command.
	OSCallback notificationCallback;
	uintptr_t disabled : 1, checked : 1;
	uintptr_t state;
};

struct Window : GUIObject {
	OSHandle window, surface;

	// For OS_CREATE_WINDOW_MENU: 1 x 1.
	// For OS_CREATE_WINDOW_NORMAL: 3 x 4.
	// 	- Content pane at 1, 2.
	// 	- Or if OS_CREATE_WINDOW_WITH_MENUBAR,
	// 		- This is a subgrid, 1 x 2
	// 		- Menubar at 0, 0.
	// 		- Content pane at 0, 1.
	Grid *root;

	unsigned flags;
	OSCursorStyle cursor, cursorOld;

	struct Control *pressed,   // Mouse is pressing the control.
		       *hover,     // Mouse is hovering over the control.
		       *focus, 	   // Control has strong focus.
		       *lastFocus; // Control has weak focus.

	bool destroyed, created;

	int width, height;
	int minimumWidth, minimumHeight;

	LinkedList<Control> timerControls;
	int timerHz, caretBlinkStep, caretBlinkPause;

	CommandWindow *commands;
	void *instance;
	bool hasParent;
};

struct OpenMenu {
	Window *window;
	Control *source;
};

static OpenMenu openMenus[8];
static unsigned openMenuCount;

#define CLIP_STACK_LIMIT (256)
#define CLIP_RECTANGLE (clipStack[clipStackPosition])
static OSRectangle clipStack[CLIP_STACK_LIMIT];
static uintptr_t clipStackPosition;

static bool PushClipRectangle(OSRectangle rectangle) {
	if (clipStackPosition == CLIP_STACK_LIMIT) {
		OSCrashProcess(OS_FATAL_ERROR_CLIP_STACK_OVERFLOW);
	}

	OSRectangle current = CLIP_RECTANGLE;
	OSRectangle intersection = OS_MAKE_RECTANGLE(0, 0, 0, 0);

	if (!((current.left > rectangle.right && current.right > rectangle.left)
			|| (current.top > rectangle.bottom && current.bottom > rectangle.top))) {
		intersection.left = current.left > rectangle.left ? current.left : rectangle.left;
		intersection.top = current.top > rectangle.top ? current.top : rectangle.top;
		intersection.right = current.right < rectangle.right ? current.right : rectangle.right;
		intersection.bottom = current.bottom < rectangle.bottom ? current.bottom : rectangle.bottom;
	}

	clipStack[++clipStackPosition] = intersection;

	return intersection.left != intersection.right && intersection.top != intersection.bottom;
}

static void PopClipRectangle() {
	clipStackPosition--;
}

void OSDebugGUIObject(OSObject _guiObject) {
	GUIObject *guiObject = (GUIObject *) _guiObject;
	guiObject->verbose = true;
}

static void CopyText(OSString buffer) {
	OSClipboardHeader header = {};
	header.format = OS_CLIPBOARD_FORMAT_TEXT;
	header.textBytes = buffer.bytes;
	OSSyscall(OS_SYSCALL_COPY, (uintptr_t) buffer.buffer, (uintptr_t) &header, 0, 0);
}

static size_t ClipboardTextBytes() {
	OSClipboardHeader clipboard;
	OSSyscall(OS_SYSCALL_GET_CLIPBOARD_HEADER, 0, (uintptr_t) &clipboard, 0, 0);
	return clipboard.textBytes;
}

static inline void OSRepaintControl(OSObject _control) {
	Control *control = (Control *) _control;

	if (!control->repaint) {
		control->repaint = true;
		SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);
	}
}

static inline bool IsPointInRectangle(OSRectangle rectangle, int x, int y) {
	if (rectangle.left > x || rectangle.right <= x || rectangle.top > y || rectangle.bottom <= y) {
		return false;
	}
	
	return true;
}

static inline void UpdateCheckboxIcons(Control *control) {
	control->icons = control->isChecked ? checkboxIconsChecked : checkboxIcons;
}

static void SetControlCommand(Control *control, OSCommand *_command) {
	if (_command->identifier == OS_COMMAND_DYNAMIC) {
		control->isChecked = _command->defaultCheck;
		control->disabled = _command->defaultDisabled;
		control->notificationCallback = _command->callback;
		return;
	}

	control->command = _command;
	control->commandItem.thisItem = control;
	control->checkable = _command->checkable;

	if (control->window) {
		CommandWindow *command = control->window->commands + _command->identifier;
		command->controls.InsertEnd(&control->commandItem);
		control->isChecked = command->checked;
		control->disabled = command->disabled;
		control->notificationCallback = command->notificationCallback;
	}
}

void OSAnimateControl(OSObject _control, bool fast) {
	Control *control = (Control *) _control;

	// OSPrint("Animate control %x\n", control);
	
	if (control->firstPaint && !control->noAnimations) {
		control->from1 = control->current1;
		control->from2 = control->current2;
		control->from3 = control->current3;
		control->from4 = control->current4;
		control->animationStep = 0;
		control->finalAnimationStep = fast ? 4 : 16;

		if (!control->timerControlItem.list) {
			control->timerHz = 30; // TODO Make this 60?
			control->window->timerControls.InsertStart(&control->timerControlItem);
			control->timerControlItem.thisItem = control;
		}
	} else {
		control->animationStep = 16;
		control->finalAnimationStep = 16;
	}

	OSRepaintControl(control);
}

static void StandardCellLayout(GUIObject *object) {
	object->cellBounds = object->bounds;

	int width = object->bounds.right - object->bounds.left;
	int height = object->bounds.bottom - object->bounds.top;

	int preferredWidth = object->preferredWidth == DIMENSION_PUSH ? width : object->preferredWidth;
	int preferredHeight = object->preferredHeight == DIMENSION_PUSH ? height : object->preferredHeight;

	if (width > preferredWidth) {
		if (object->layout & OS_CELL_H_EXPAND) {
		} else if (object->layout & OS_CELL_H_LEFT) {
			object->bounds.right = object->bounds.left + preferredWidth;
		} else if (object->layout & OS_CELL_H_RIGHT) {
			object->bounds.left = object->bounds.right - preferredWidth;
		} else {
			object->bounds.left = object->bounds.left + width / 2 - preferredWidth / 2;
			object->bounds.right = object->bounds.left + preferredWidth;
		}
	}

	if (height > preferredHeight) {
		if (object->layout & OS_CELL_V_EXPAND) {
		} else if (object->layout & OS_CELL_V_TOP) {
			object->bounds.bottom = object->bounds.top + preferredHeight;
		} else if (object->layout & OS_CELL_V_BOTTOM) {
			object->bounds.top = object->bounds.bottom - preferredHeight;
		} else {
			object->bounds.top = object->bounds.top + height / 2 - preferredHeight / 2;
			object->bounds.bottom = object->bounds.top + preferredHeight;
		}
	}
}

static OSCallbackResponse ProcessControlMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Control *control = (Control *) _object;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			if (control->cellBounds.left == message->layout.left
					&& control->cellBounds.right == message->layout.right
					&& control->cellBounds.top == message->layout.top
					&& control->cellBounds.bottom == message->layout.bottom
					&& !control->relayout) {
				break;
			}

			control->bounds = OS_MAKE_RECTANGLE(
					message->layout.left, message->layout.right,
					message->layout.top, message->layout.bottom);
			StandardCellLayout(control);

			PushClipRectangle(control->bounds);
			control->inputBounds = CLIP_RECTANGLE;
			if (control->verbose) {
				OSPrint("Layout control %x: %d->%d, %d->%d; input %d->%d, %d->%d\n", control, control->bounds.left, control->bounds.right, control->bounds.top, control->bounds.bottom,
						control->inputBounds.left, control->inputBounds.right, control->inputBounds.top, control->inputBounds.bottom);
			}
			PopClipRectangle();

			control->relayout = false;
			OSRepaintControl(control);
			SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);

			{
				OSMessage m = *message;
				m.type = OS_MESSAGE_LAYOUT_TEXT;
				OSSendMessage(control, &m);
			}
		} break;

		case OS_MESSAGE_LAYOUT_TEXT: {
			control->textBounds = control->bounds;

			if (control->icons) {
				control->textBounds.left += control->icons[0]->region.right - control->icons[0]->region.left + 4;
			}
		} break;

		case OS_MESSAGE_MEASURE: {
			if (control->layout & OS_CELL_H_PUSH) message->measure.preferredWidth = DIMENSION_PUSH;
			else message->measure.preferredWidth = control->preferredWidth;
			if (control->layout & OS_CELL_V_PUSH) message->measure.preferredHeight = DIMENSION_PUSH;
			else message->measure.preferredHeight = control->preferredHeight;
			
			message->measure.minimumWidth = control->minimumWidth;
			message->measure.minimumHeight = control->minimumHeight;
		} break;

		case OS_MESSAGE_PAINT: {
			if (control->repaint || message->paint.force) {
				// if (!control->firstPaint) OSPrint("first paint %x\n", control);
				control->firstPaint = true;
				
				if (control->drawParentBackground) {
					OSMessage m = *message;
					m.type = OS_MESSAGE_PAINT_BACKGROUND;
					m.paintBackground.surface = message->paint.surface;
					m.paintBackground.left = control->bounds.left;
					m.paintBackground.right = control->bounds.right;
					m.paintBackground.top = control->bounds.top;
					m.paintBackground.bottom = control->bounds.bottom;
					OSSendMessage(control->parent, &m);
				}

				bool menuSource = false;

				for (uintptr_t i = 0; i < openMenuCount; i++) {
					if (openMenus[i].source == control) {
						menuSource = true;
						break;
					}
				}

				bool disabled = control->disabled,
				     pressed = ((control->window->pressed == control && control->window->hover == control) || control->window->focus == control || menuSource) && !disabled,
				     hover = (control->window->hover == control || control->window->pressed == control) && !pressed && !disabled,
				     normal = !hover && !pressed && !disabled;

				control->current1 = ((normal   ? 15 : 0) - control->from1) * control->animationStep / control->finalAnimationStep + control->from1;
				control->current2 = ((hover    ? 15 : 0) - control->from2) * control->animationStep / control->finalAnimationStep + control->from2;
				control->current3 = ((pressed  ? 15 : 0) - control->from3) * control->animationStep / control->finalAnimationStep + control->from3;
				control->current4 = ((disabled ? 15 : 0) - control->from4) * control->animationStep / control->finalAnimationStep + control->from4;

				if (control->backgrounds) {
					if (control->backgrounds[0]) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[0]->region, 
								control->backgrounds[0]->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
					}

					if (control->backgrounds[2] && control->current2) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[2]->region, 
								control->backgrounds[2]->border, OS_DRAW_MODE_REPEAT_FIRST, control->current2 == 15 ? 0xFF : 0xF * control->current2, CLIP_RECTANGLE);
					}

					if (control->backgrounds[3] && control->current3) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[3]->region, 
								control->backgrounds[3]->border, OS_DRAW_MODE_REPEAT_FIRST, control->current3 == 15 ? 0xFF : 0xF * control->current3, CLIP_RECTANGLE);
					}

					if (control->backgrounds[1] && control->current4) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[1]->region, 
								control->backgrounds[1]->border, OS_DRAW_MODE_REPEAT_FIRST, control->current4 == 15 ? 0xFF : 0xF * control->current4, CLIP_RECTANGLE);
					}
				}

				if (control->icons) {
					OSRectangle bounds = control->bounds;
					
					if (control->centerIcons) {
						bounds.left += (bounds.right - bounds.left) / 2 - (control->icons[0]->region.right - control->icons[0]->region.left) / 2;
						bounds.right = bounds.left + control->icons[0]->region.right - control->icons[0]->region.left;
						bounds.top += (bounds.bottom - bounds.top) / 2 - (control->icons[0]->region.bottom - control->icons[0]->region.top) / 2;
						bounds.bottom = bounds.top + control->icons[0]->region.bottom - control->icons[0]->region.top;
					} else {
						bounds.right = bounds.left + control->icons[0]->region.right - control->icons[0]->region.left;
						bounds.top += (bounds.bottom - bounds.top) / 2 - (control->icons[0]->region.bottom - control->icons[0]->region.top) / 2;
						bounds.bottom = bounds.top + control->icons[0]->region.bottom - control->icons[0]->region.top;
					}

					if (control->current1) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->icons[0]->region, 
								control->icons[0]->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
					}

					if (control->current2) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->icons[2]->region, 
								control->icons[2]->border, OS_DRAW_MODE_REPEAT_FIRST, control->current2 == 15 ? 0xFF : 0xF * control->current2, CLIP_RECTANGLE);
					}

					if (control->current3) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->icons[3]->region, 
								control->icons[3]->border, OS_DRAW_MODE_REPEAT_FIRST, control->current3 == 15 ? 0xFF : 0xF * control->current3, CLIP_RECTANGLE);
					}

					if (control->current4) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, control->icons[1]->region, 
								control->icons[1]->border, OS_DRAW_MODE_REPEAT_FIRST, control->current4 == 15 ? 0xFF : 0xF * control->current4, CLIP_RECTANGLE);
					}
				}

				uint32_t textColor = control->textColor;
				uint32_t textShadowColor = 0xFFFFFF - textColor;

				if (control->disabled && !control->noDisabledTextColorChange) {
					textColor = 0x777777;
					textShadowColor = 0xEEEEEE;
				}

				if (!control->customTextRendering) {
					if (control->textShadow || control->disabled) {
						OSRectangle bounds = control->textBounds;
						bounds.top++; bounds.bottom++; bounds.left++; bounds.right++;

						OSDrawString(message->paint.surface, bounds, &control->text, control->textSize,
								control->textAlign, textShadowColor, -1, control->textBold, CLIP_RECTANGLE);
					}

					OSDrawString(message->paint.surface, control->textBounds, &control->text, control->textSize,
							control->textAlign, textColor, -1, control->textBold, CLIP_RECTANGLE);
				}

				{
					OSMessage m = *message;
					m.type = OS_MESSAGE_CUSTOM_PAINT;
					OSSendMessage(control, &m);
				}

				control->repaint = false;
			}
		} break;

		case OS_MESSAGE_PARENT_UPDATED: {
			control->window = (Window *) message->parentUpdated.window;
			control->animationStep = 16;
			control->finalAnimationStep = 16;
			OSRepaintControl(control);

			if (control->command) {
				SetControlCommand(control, control->command);

				if (control->checkboxIcons) {
					UpdateCheckboxIcons(control);
				}
			}
		} break;

		case OS_MESSAGE_DESTROY: {
			if (control->timerControlItem.list) {
				control->window->timerControls.Remove(&control->timerControlItem);
			}

			if (control->commandItem.list) {
				control->commandItem.RemoveFromList();
			}

			if (control->window->hover == control) control->window->hover = nullptr;
			if (control->window->pressed == control) control->window->pressed = nullptr;
			if (control->window->focus == control) control->window->focus = nullptr;
			if (control->window->lastFocus == control) control->window->lastFocus = nullptr;

			OSHeapFree(control->text.buffer);
			OSHeapFree(control);
		} break;

		case OS_MESSAGE_HIT_TEST: {
			message->hitTest.result = IsPointInRectangle(control->inputBounds, message->hitTest.positionX, message->hitTest.positionY);
		} break;

		case OS_MESSAGE_MOUSE_RIGHT_PRESSED: {
			if (control->rightClickMenu) {
				OSCreateMenu(control->rightClickMenu, control, OS_CREATE_MENU_AT_CURSOR, OS_FLAGS_DEFAULT);
			}
		} break;

		case OS_MESSAGE_START_HOVER:
		case OS_MESSAGE_END_HOVER:
		case OS_MESSAGE_START_FOCUS:
		case OS_MESSAGE_END_FOCUS:
		case OS_MESSAGE_END_LAST_FOCUS:
		case OS_MESSAGE_START_PRESS:
		case OS_MESSAGE_END_PRESS: {
			OSAnimateControl(control, message->type == OS_MESSAGE_START_PRESS || message->type == OS_MESSAGE_START_FOCUS);
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (!IsPointInRectangle(control->inputBounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				break;
			}

			if (!control->disabled || control->keepCustomCursorWhenDisabled) {
				control->window->cursor = (OSCursorStyle) control->cursor;
			}

			if (control->window->hover != control) {
				control->window->hover = control;

				{
					OSMessage message;
					message.type = OS_MESSAGE_START_HOVER;
					OSSendMessage(control, &message);
				}
			}
		} break;

		case OS_MESSAGE_WM_TIMER: {
			if (control->animationStep == control->finalAnimationStep) {
				control->window->timerControls.Remove(&control->timerControlItem);
			} else {
				control->animationStep++;
				OSRepaintControl(control);
			}
		} break;

		case OS_MESSAGE_CLICKED: {
			if (control->checkable) {
				// Update the checked state.
				control->isChecked = !control->isChecked;

				if (control->command) {
					// Update the command.
					CommandWindow *command = control->window->commands + control->command->identifier;
					command->checked = control->isChecked;
					LinkedItem<Control> *item = command->controls.firstItem;

					while (item) {
						item->thisItem->isChecked = control->isChecked;
						UpdateCheckboxIcons(item->thisItem);
						OSRepaintControl(item->thisItem);
						item = item->nextItem;
					}
				} else {
					UpdateCheckboxIcons(control);
				}
			}

			OSMessage message;
			message.type = OS_NOTIFICATION_COMMAND;
			message.command.checked = control->isChecked;
			message.command.window = control->window;
			message.command.command = control->command;
			OSForwardMessage(control, control->notificationCallback, &message);

			if (control->window->flags & OS_CREATE_WINDOW_MENU) {
				message.type = OS_MESSAGE_DESTROY;
				OSSendMessage(openMenus[0].window, &message);
			}
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

static void CreateString(char *text, size_t textBytes, OSString *string) {
	OSHeapFree(string->buffer);
	string->buffer = (char *) OSHeapAllocate(textBytes, false);
	string->bytes = textBytes;
	string->characters = 0;
	OSCopyMemory(string->buffer, text, textBytes);

	char *m = string->buffer;

	while (m < string->buffer + textBytes) {
		m = utf8_advance(m);
		string->characters++;
	}
}

static OSCallbackResponse ProcessWindowResizeHandleMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	WindowResizeControl *control = (WindowResizeControl *) _object;

	if (message->type == OS_MESSAGE_MOUSE_DRAGGED) {
		Window *window = control->window;
		OSRectangle bounds, bounds2;
		OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, window->window, (uintptr_t) &bounds, 0, 0);

		bounds2 = bounds;

		int oldWidth = bounds.right - bounds.left;
		int oldHeight = bounds.bottom - bounds.top;

		if (control->direction & RESIZE_LEFT) bounds.left = message->mouseMoved.newPositionXScreen;
		if (control->direction & RESIZE_RIGHT) bounds.right = message->mouseMoved.newPositionXScreen;
		if (control->direction & RESIZE_TOP) bounds.top = message->mouseMoved.newPositionYScreen;
		if (control->direction & RESIZE_BOTTOM) bounds.bottom = message->mouseMoved.newPositionYScreen;

		int newWidth = bounds.right - bounds.left;
		int newHeight = bounds.bottom - bounds.top;

		if (newWidth < window->minimumWidth && control->direction & RESIZE_LEFT) bounds.left = bounds.right - window->minimumWidth;
		if (newWidth < window->minimumWidth && control->direction & RESIZE_RIGHT) bounds.right = bounds.left + window->minimumWidth;
		if (newHeight < window->minimumHeight && control->direction & RESIZE_TOP) bounds.top = bounds.bottom - window->minimumHeight;
		if (newHeight < window->minimumHeight && control->direction & RESIZE_BOTTOM) bounds.bottom = bounds.top + window->minimumHeight;

		if (control->direction == RESIZE_MOVE) {
			bounds.left = message->mouseDragged.newPositionXScreen - message->mouseDragged.originalPositionX;
			bounds.top = message->mouseDragged.newPositionYScreen - message->mouseDragged.originalPositionY;
			bounds.right = bounds.left + oldWidth;
			bounds.bottom = bounds.top + oldHeight;
		}
		
		OSSyscall(OS_SYSCALL_MOVE_WINDOW, window->window, (uintptr_t) &bounds, 0, 0);

		window->width = bounds.right - bounds.left;
		window->height = bounds.bottom - bounds.top;

		OSMessage layout;
		layout.type = OS_MESSAGE_LAYOUT;
		layout.layout.left = 0;
		layout.layout.top = 0;
		layout.layout.right = window->width;
		layout.layout.bottom = window->height;
		layout.layout.force = true;
		OSSendMessage(window->root, &layout);
	} else if (message->type == OS_MESSAGE_LAYOUT_TEXT) {
		control->textBounds = control->bounds;
		control->textBounds.top -= 3;
		control->textBounds.bottom -= 3;
	} else {
		response = OSForwardMessage(_object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
	}

	return response;
}

static OSObject CreateWindowResizeHandle(UIImage **images, unsigned direction) {
	WindowResizeControl *control = (WindowResizeControl *) OSHeapAllocate(sizeof(WindowResizeControl), true);
	control->type = API_OBJECT_CONTROL;
	control->backgrounds = images;
	control->preferredWidth = images[0]->region.right - images[0]->region.left;
	control->preferredHeight = images[0]->region.bottom - images[0]->region.top;
	control->minimumWidth = images[0]->region.right - images[0]->region.left;
	control->minimumHeight = images[0]->region.bottom - images[0]->region.top;
	control->direction = direction;
	control->noAnimations = true;
	control->noDisabledTextColorChange = true;
	control->keepCustomCursorWhenDisabled = true;
	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessWindowResizeHandleMessage, nullptr));

	switch (direction) {
		case RESIZE_LEFT:
		case RESIZE_RIGHT:
			control->cursor = OS_CURSOR_RESIZE_HORIZONTAL;
			break;

		case RESIZE_TOP:
		case RESIZE_BOTTOM:
			control->cursor = OS_CURSOR_RESIZE_VERTICAL;
			break;

		case RESIZE_TOP_RIGHT:
		case RESIZE_BOTTOM_LEFT:
			control->cursor = OS_CURSOR_RESIZE_DIAGONAL_1;
			break;

		case RESIZE_TOP_LEFT:
		case RESIZE_BOTTOM_RIGHT:
			control->cursor = OS_CURSOR_RESIZE_DIAGONAL_2;
			break;

		case RESIZE_MOVE: {
			control->textColor = 0xFFFFFF;
			control->textShadow = true;
			control->textBold = true;
			control->textSize = 11;
		} break;
	}

	return control;
}

enum CharacterType {
	CHARACTER_INVALID,
	CHARACTER_IDENTIFIER, // A-Z, a-z, 0-9, _, >= 0x7F
	CHARACTER_WHITESPACE, // space, tab, newline
	CHARACTER_OTHER,
};

static CharacterType GetCharacterType(int character) {
	if ((character >= '0' && character <= '9') 
			|| (character >= 'a' && character <= 'z')
			|| (character >= 'A' && character <= 'Z')
			|| (character == '_')
			|| (character >= 0x80)) {
		return CHARACTER_IDENTIFIER;
	}

	if (character == '\n' || character == '\t' || character == ' ') {
		return CHARACTER_WHITESPACE;
	}

	// TODO Treat escape-sequence-likes in the textbox as words?
	return CHARACTER_OTHER;
}

static void MoveCaret(OSString *string, OSCaret *caret, bool right, bool word, bool strongWhitespace = false) {
	if (!string->bytes) return;

	CharacterType type = CHARACTER_INVALID;

	if (word && right) goto checkCharacterType;

	while (true) {
		if (!right) {
			if (caret->character) {
				caret->character--;
				caret->byte = utf8_retreat(string->buffer + caret->byte) - string->buffer;
			} else {
				return; // We cannot move any further left.
			}
		} else {
			if (caret->character != string->characters) {
				caret->character++;
				caret->byte = utf8_advance(string->buffer + caret->byte) - string->buffer;
			} else {
				return; // We cannot move any further right.
			}
		}

		if (!word) {
			return;
		}

		checkCharacterType:;
		CharacterType newType = GetCharacterType(utf8_value(string->buffer + caret->byte));

		if (type == CHARACTER_INVALID) {
			if (newType != CHARACTER_WHITESPACE || strongWhitespace) {
				type = newType;
			}
		} else {
			if (newType != type) {
				if (!right) {
					// We've gone too far.
					MoveCaret(string, caret, true, false);
				}

				break;
			}
		}
	}
}

static void FindCaret(Textbox *control, int positionX, int positionY, bool secondCaret, unsigned clickChainCount) {
	if (clickChainCount >= 3) {
		control->caret.byte = 0;
		control->caret.character = 0;
		control->caret2.byte = control->text.bytes;
		control->caret2.character = control->text.characters;
	} else {
		OSFindCharacterAtCoordinate(control->textBounds, OS_MAKE_POINT(positionX, positionY), 
				&control->text, control->textAlign, &control->caret2, control->textSize);

		if (!secondCaret) {
			control->caret = control->caret2;

			if (clickChainCount == 2) {
				MoveCaret(&control->text, &control->caret, false, true, true);
				MoveCaret(&control->text, &control->caret2, true, true, true);
				control->wordSelectionAnchor  = control->caret;
				control->wordSelectionAnchor2 = control->caret2;
			}
		} else {
			if (clickChainCount == 2) {
				if (control->caret2.byte < control->caret.byte) {
					MoveCaret(&control->text, &control->caret2, false, true);
					control->caret = control->wordSelectionAnchor2;
				} else {
					MoveCaret(&control->text, &control->caret2, true, true);
					control->caret = control->wordSelectionAnchor;
				}
			}
		}
	}

	control->window->caretBlinkPause = CARET_BLINK_PAUSE;
}

static void RemoveSelectedText(Textbox *control) {
	if (control->caret.byte == control->caret2.byte) return;
	if (control->caret.byte < control->caret2.byte) {
		OSCaret temp = control->caret2;
		control->caret2 = control->caret;
		control->caret = temp;
	}
	int bytes = control->caret.byte - control->caret2.byte;
	OSCopyMemory(control->text.buffer + control->caret2.byte, control->text.buffer + control->caret.byte, control->text.bytes - control->caret.byte);
	control->text.characters -= control->caret.character - control->caret2.character;
	control->text.bytes -= bytes;
	control->caret = control->caret2;
}

OSCallbackResponse ProcessTextboxMessage(OSObject object, OSMessage *message) {
	Textbox *control = (Textbox *) message->context;

	OSCallbackResponse result = OS_CALLBACK_NOT_HANDLED;
	static int lastClickChainCount = 1;

	if (message->type == OS_MESSAGE_CUSTOM_PAINT) {
		DrawString(message->paint.surface, control->textBounds, 
				&control->text, control->textAlign, control->textColor, 0xFFFFFF, control->window->focus == control ? 0xFFC4D9F9 : 0xFFDDDDDD,
				{0, 0}, nullptr, control->caret.character, control->window->lastFocus == control 
				&& !control->disabled ? control->caret2.character : control->caret.character, 
				control->window->lastFocus != control || control->caretBlink,
				control->textSize, fontRegular, CLIP_RECTANGLE);

		result = OS_CALLBACK_HANDLED;
	} else if (message->type == OS_MESSAGE_LAYOUT_TEXT) {
		control->textBounds = control->bounds;
		control->textBounds.left += 6;
		control->textBounds.right -= 6;
		result = OS_CALLBACK_HANDLED;
	} else if (message->type == OS_MESSAGE_CARET_BLINK) {
		control->caretBlink = !control->caretBlink;
		result = OS_CALLBACK_HANDLED;
		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_START_FOCUS) {
		control->caretBlink = false;
		control->window->caretBlinkPause = CARET_BLINK_PAUSE;

		control->caret.byte = 0;
		control->caret.character = 0;
		control->caret2.byte = control->text.bytes;
		control->caret2.character = control->text.characters;

		OSEnableCommand(control->window, osCommandPaste, ClipboardTextBytes());
		OSEnableCommand(control->window, osCommandSelectAll, true);

		OSSetCommandNotificationCallback(control->window, osCommandPaste, OS_MAKE_CALLBACK(ProcessTextboxMessage, control));
		OSSetCommandNotificationCallback(control->window, osCommandSelectAll, OS_MAKE_CALLBACK(ProcessTextboxMessage, control));
		OSSetCommandNotificationCallback(control->window, osCommandCopy, OS_MAKE_CALLBACK(ProcessTextboxMessage, control));
		OSSetCommandNotificationCallback(control->window, osCommandCut, OS_MAKE_CALLBACK(ProcessTextboxMessage, control));
		OSSetCommandNotificationCallback(control->window, osCommandDelete, OS_MAKE_CALLBACK(ProcessTextboxMessage, control));
	} else if (message->type == OS_MESSAGE_CLIPBOARD_UPDATED) {
		OSEnableCommand(control->window, osCommandPaste, ClipboardTextBytes());
	} else if (message->type == OS_MESSAGE_END_LAST_FOCUS) {
		OSDisableCommand(control->window, osCommandPaste, true);
		OSDisableCommand(control->window, osCommandSelectAll, true);
		OSDisableCommand(control->window, osCommandCopy, true);
		OSDisableCommand(control->window, osCommandCut, true);
		OSDisableCommand(control->window, osCommandDelete, true);
	} else if (message->type == OS_NOTIFICATION_COMMAND) {
		// TODO Code copy-and-pasted from KEY_TYPED below.

		if (message->command.command == osCommandSelectAll) {
			control->caret.byte = 0;
			control->caret.character = 0;

			control->caret2.byte = control->text.bytes;
			control->caret2.character = control->text.characters;
		}

		if (message->command.command == osCommandCopy || message->command.command == osCommandCut) {
			OSString string;
			int length = control->caret.byte - control->caret2.byte;
			if (length < 0) length = -length;
			string.bytes = length;
			if (control->caret.byte > control->caret2.byte) string.buffer = control->text.buffer + control->caret2.byte;
			else string.buffer = control->text.buffer + control->caret.byte;
			CopyText(string);
		}

		if (message->command.command == osCommandDelete || message->command.command == osCommandCut || message->command.command == osCommandPaste) {
			RemoveSelectedText(control);
		}

		if (message->command.command == osCommandPaste) {
			int bytes = ClipboardTextBytes();
			char *old = control->text.buffer;
			control->text.buffer = (char *) OSHeapAllocate(control->text.bytes + bytes, false);
			OSCopyMemory(control->text.buffer, old, control->caret.byte);
			OSCopyMemory(control->text.buffer + control->caret.byte + bytes, old + control->caret.byte, control->text.bytes - control->caret.byte);
			OSHeapFree(old);
			char *c = control->text.buffer + control->caret.byte;
			char *d = c;
			OSSyscall(OS_SYSCALL_PASTE_TEXT, bytes, (uintptr_t) c, 0, 0);
			size_t characters = 0;
			while (c < d + bytes) { characters++; c = utf8_advance(c); }  
			control->text.characters += characters;
			control->caret.character += characters;
			control->caret.byte += bytes;
			control->text.bytes += bytes;
			control->caret2 = control->caret;
		}

		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_START_PRESS) {
		FindCaret(control, message->mousePressed.positionX, message->mousePressed.positionY, false, message->mousePressed.clickChainCount);
		lastClickChainCount = message->mousePressed.clickChainCount;
		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_START_DRAG 
			|| (message->type == OS_MESSAGE_MOUSE_RIGHT_PRESSED && control->caret.byte == control->caret2.byte)) {
		FindCaret(control, message->mouseDragged.originalPositionX, message->mouseDragged.originalPositionY, true, lastClickChainCount);
		control->caret = control->caret2;
		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_MOUSE_DRAGGED) {
		FindCaret(control, message->mouseDragged.newPositionX, message->mouseDragged.newPositionY, true, lastClickChainCount);
		OSRepaintControl(control);
	} else if (message->type == OS_MESSAGE_TEXT_UPDATED) {
		control->caret = control->caret2;
	} else if (message->type == OS_MESSAGE_KEY_TYPED) {
		control->caretBlink = false;
		control->window->caretBlinkPause = CARET_BLINK_PAUSE;

		int ic = -1, isc = -1;

		switch (message->keyboard.scancode) {
			case OS_SCANCODE_A: ic = 'a'; isc = 'A'; break;
			case OS_SCANCODE_B: ic = 'b'; isc = 'B'; break;
			case OS_SCANCODE_C: ic = 'c'; isc = 'C'; break;
			case OS_SCANCODE_D: ic = 'd'; isc = 'D'; break;
			case OS_SCANCODE_E: ic = 'e'; isc = 'E'; break;
			case OS_SCANCODE_F: ic = 'f'; isc = 'F'; break;
			case OS_SCANCODE_G: ic = 'g'; isc = 'G'; break;
			case OS_SCANCODE_H: ic = 'h'; isc = 'H'; break;
			case OS_SCANCODE_I: ic = 'i'; isc = 'I'; break;
			case OS_SCANCODE_J: ic = 'j'; isc = 'J'; break;
			case OS_SCANCODE_K: ic = 'k'; isc = 'K'; break;
			case OS_SCANCODE_L: ic = 'l'; isc = 'L'; break;
			case OS_SCANCODE_M: ic = 'm'; isc = 'M'; break;
			case OS_SCANCODE_N: ic = 'n'; isc = 'N'; break;
			case OS_SCANCODE_O: ic = 'o'; isc = 'O'; break;
			case OS_SCANCODE_P: ic = 'p'; isc = 'P'; break;
			case OS_SCANCODE_Q: ic = 'q'; isc = 'Q'; break;
			case OS_SCANCODE_R: ic = 'r'; isc = 'R'; break;
			case OS_SCANCODE_S: ic = 's'; isc = 'S'; break;
			case OS_SCANCODE_T: ic = 't'; isc = 'T'; break;
			case OS_SCANCODE_U: ic = 'u'; isc = 'U'; break;
			case OS_SCANCODE_V: ic = 'v'; isc = 'V'; break;
			case OS_SCANCODE_W: ic = 'w'; isc = 'W'; break;
			case OS_SCANCODE_X: ic = 'x'; isc = 'X'; break;
			case OS_SCANCODE_Y: ic = 'y'; isc = 'Y'; break;
			case OS_SCANCODE_Z: ic = 'z'; isc = 'Z'; break;
			case OS_SCANCODE_0: ic = '0'; isc = ')'; break;
			case OS_SCANCODE_1: ic = '1'; isc = '!'; break;
			case OS_SCANCODE_2: ic = '2'; isc = '@'; break;
			case OS_SCANCODE_3: ic = '3'; isc = '#'; break;
			case OS_SCANCODE_4: ic = '4'; isc = '$'; break;
			case OS_SCANCODE_5: ic = '5'; isc = '%'; break;
			case OS_SCANCODE_6: ic = '6'; isc = '^'; break;
			case OS_SCANCODE_7: ic = '7'; isc = '&'; break;
			case OS_SCANCODE_8: ic = '8'; isc = '*'; break;
			case OS_SCANCODE_9: ic = '9'; isc = '('; break;
			case OS_SCANCODE_SLASH: 	ic = '/';  isc = '?'; break;
			case OS_SCANCODE_BACKSLASH: 	ic = '\\'; isc = '|'; break;
			case OS_SCANCODE_LEFT_BRACE: 	ic = '[';  isc = '{'; break;
			case OS_SCANCODE_RIGHT_BRACE: 	ic = ']';  isc = '}'; break;
			case OS_SCANCODE_EQUALS: 	ic = '=';  isc = '+'; break;
			case OS_SCANCODE_BACKTICK: 	ic = '`';  isc = '~'; break;
			case OS_SCANCODE_HYPHEN: 	ic = '-';  isc = '_'; break;
			case OS_SCANCODE_SEMICOLON: 	ic = ';';  isc = ':'; break;
			case OS_SCANCODE_QUOTE: 	ic = '\''; isc = '"'; break;
			case OS_SCANCODE_COMMA: 	ic = ',';  isc = '<'; break;
			case OS_SCANCODE_PERIOD: 	ic = '.';  isc = '>'; break;
			case OS_SCANCODE_SPACE: 	ic = ' ';  isc = ' '; break;

			case OS_SCANCODE_BACKSPACE: {
				if (control->caret.byte == control->caret2.byte && control->caret.byte) {
					MoveCaret(&control->text, &control->caret2, false, message->keyboard.ctrl);
					int bytes = control->caret.byte - control->caret2.byte;
					OSCopyMemory(control->text.buffer + control->caret2.byte, control->text.buffer + control->caret.byte, control->text.bytes - control->caret.byte);
					control->text.characters -= 1;
					control->text.bytes -= bytes;
					control->caret = control->caret2;
				} else {
					RemoveSelectedText(control);
				}
			} break;

			case OS_SCANCODE_DELETE: {
				if (control->caret.byte == control->caret2.byte && control->caret.byte != control->text.bytes) {
					MoveCaret(&control->text, &control->caret, true, message->keyboard.ctrl);
					int bytes = control->caret.byte - control->caret2.byte;
					OSCopyMemory(control->text.buffer + control->caret2.byte, control->text.buffer + control->caret.byte, control->text.bytes - control->caret.byte);
					control->text.characters -= 1;
					control->text.bytes -= bytes;
					control->caret = control->caret2;
				} else {
					RemoveSelectedText(control);
				}
			} break;

			case OS_SCANCODE_LEFT_ARROW: {
				if (message->keyboard.shift) {
					MoveCaret(&control->text, &control->caret2, false, message->keyboard.ctrl);
				} else {
					bool move = control->caret2.byte == control->caret.byte;

					if (control->caret2.byte < control->caret.byte) control->caret = control->caret2;
					else control->caret2 = control->caret;

					if (move) {
						MoveCaret(&control->text, &control->caret2, false, message->keyboard.ctrl);
						control->caret = control->caret2;
					}
				}
			} break;

			case OS_SCANCODE_RIGHT_ARROW: {
				if (message->keyboard.shift) {
					MoveCaret(&control->text, &control->caret2, true, message->keyboard.ctrl);
				} else {
					bool move = control->caret2.byte == control->caret.byte;

					if (control->caret2.byte > control->caret.byte) control->caret = control->caret2;
					else control->caret2 = control->caret;

					if (move) {
						MoveCaret(&control->text, &control->caret2, true, message->keyboard.ctrl);
						control->caret = control->caret2;
					}
				}
			} break;

			case OS_SCANCODE_HOME: {
				control->caret2.byte = 0;
				control->caret2.character = 0;
				if (!message->keyboard.shift) control->caret = control->caret2;
			} break;

			case OS_SCANCODE_END: {
				control->caret2.byte = control->text.bytes;
				control->caret2.character = control->text.characters;
				if (!message->keyboard.shift) control->caret = control->caret2;
			} break;
		}

		if (message->keyboard.ctrl && !message->keyboard.alt && !message->keyboard.shift) {
			if (message->keyboard.scancode == OS_SCANCODE_A) {
				control->caret.byte = 0;
				control->caret.character = 0;

				control->caret2.byte = control->text.bytes;
				control->caret2.character = control->text.characters;
			}
		}

		if (ic != -1 && !message->keyboard.alt && !message->keyboard.ctrl) {
			RemoveSelectedText(control);

			char data[4];
			int bytes = utf8_encode(message->keyboard.shift ? isc : ic, data);
			char *old = control->text.buffer;
			control->text.buffer = (char *) OSHeapAllocate(control->text.bytes + bytes, false);
			OSCopyMemory(control->text.buffer, old, control->caret.byte);
			OSCopyMemory(control->text.buffer + control->caret.byte + bytes, old + control->caret.byte, control->text.bytes - control->caret.byte);
			OSHeapFree(old);
			OSCopyMemory(control->text.buffer + control->caret.byte, data, bytes);
			control->text.characters += 1;
			control->caret.character++;
			control->caret.byte += bytes;
			control->text.bytes += bytes;
			control->caret2 = control->caret;
		}

		OSRepaintControl(control);
	}

	if (control->window && control->window->lastFocus == control) {
		bool noSelection = control->caret.byte == control->caret2.byte;
		OSDisableCommand(control->window, osCommandCopy, noSelection);
		OSDisableCommand(control->window, osCommandCut, noSelection);
		OSDisableCommand(control->window, osCommandDelete, noSelection);
	}

	if (result == OS_CALLBACK_NOT_HANDLED) {
		result = ProcessControlMessage(object, message);
	}

	return result;
}

OSObject OSCreateTextbox(unsigned fontSize) {
	Textbox *control = (Textbox *) OSHeapAllocate(sizeof(Textbox), true);

	control->type = API_OBJECT_CONTROL;
	control->preferredWidth = 160;
	control->drawParentBackground = true;
	control->backgrounds = textboxBackgrounds;
	control->cursor = OS_CURSOR_TEXT;
	control->focusable = true;
	control->customTextRendering = true;
	control->textAlign = OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT;
	control->textSize = fontSize ? fontSize : FONT_SIZE;
	control->textColor = 0x000000;
	control->minimumHeight = control->preferredHeight = 23 - 9 + control->textSize;
	control->minimumWidth = 16;
	control->rightClickMenu = osMenuTextboxContext;

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessTextboxMessage, control));

	return control;
}

OSCallbackResponse ProcessMenuItemMessage(OSObject object, OSMessage *message) {
	MenuItem *control = (MenuItem *) object;
	OSCallbackResponse result = OS_CALLBACK_NOT_HANDLED;

	if (message->type == OS_MESSAGE_LAYOUT_TEXT) {
		if (!control->menubar) {
			control->textBounds = control->bounds;
			// Leave room for the icons.
			control->textBounds.left += 24;
			control->textBounds.right -= 4;
			result = OS_CALLBACK_HANDLED;
		}
	} else if ((message->type == OS_MESSAGE_CLICKED) || (message->type == OS_MESSAGE_START_HOVER && openMenuCount)) {
		if (control->item.type == OSMenuItem::SUBMENU) {
			if (message->type != OS_MESSAGE_CLICKED || !openMenuCount) {
				OSCreateMenu((OSMenuSpecification *) control->item.value, control, OS_CREATE_MENU_AT_SOURCE, 
						control->menubar ? OS_FLAGS_DEFAULT : OS_CREATE_SUBMENU);
			}

			if (message->type != OS_MESSAGE_START_HOVER) result = OS_CALLBACK_HANDLED;
			OSAnimateControl(control, true);
		}
	}

	if (result == OS_CALLBACK_NOT_HANDLED) {
		result = OSForwardMessage(object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
	}

	return result;
}

static OSCallbackResponse ListViewScrollbarMoved(OSObject object, OSMessage *message) {
	(void) object;
	ListView *control = (ListView *) message->context;

	if (message->type == OS_NOTIFICATION_VALUE_CHANGED) {
		OSRepaintControl(control);
		control->scrollY = message->valueChanged.newValue;
		return OS_CALLBACK_HANDLED;
	}

	return OS_CALLBACK_NOT_HANDLED;
}

static OSCallbackResponse ProcessListViewMessage(OSObject object, OSMessage *message) {
	ListView *control = (ListView *) object;
	OSCallbackResponse result = OS_CALLBACK_NOT_HANDLED;

	bool redrawScrollbar = false;

	int margin = LIST_VIEW_WITH_BORDER_MARGIN;
	OSRectangle bounds = control->bounds;
	bounds.left += margin;
	bounds.right -= margin + SCROLLBAR_SIZE;

	if (control->flags & OS_CREATE_LIST_VIEW_BORDER) {
		bounds.top += STANDARD_BORDER_SIZE;
		bounds.bottom -= STANDARD_BORDER_SIZE;
	}

	uintptr_t previousHighlightRow = control->highlightRow;

	switch (message->type) {
		case OS_MESSAGE_PAINT: {
			// If the list gets redraw, the scrollbar will need to be as well.
			if (control->repaint) redrawScrollbar = true;
		} break;

		case OS_MESSAGE_CUSTOM_PAINT: {
			OSHandle surface = message->paint.surface;

			if (PushClipRectangle(control->bounds)) {
				if (!(control->flags & OS_CREATE_LIST_VIEW_BORDER)) {
					// Draw the background.
					OSFillRectangle(surface, CLIP_RECTANGLE, OSColor(0xFFFFFFFF));
					margin = LIST_VIEW_MARGIN;
				}
		
				PushClipRectangle(bounds);

				uintptr_t i = 0;
				int y = -control->scrollY + LIST_VIEW_MARGIN / 2;

				{
					i += control->scrollY / LIST_VIEW_ROW_HEIGHT;
					y += i * LIST_VIEW_ROW_HEIGHT;
				}

				bool previousHadBox = false;

				for (; i < control->itemCount; i++) {
					OSRectangle row = OS_MAKE_RECTANGLE(bounds.left, bounds.right, bounds.top + y, bounds.top + y + LIST_VIEW_ROW_HEIGHT);
		
					if (PushClipRectangle(row)) {
						OSMessage message;
						message.type = OS_NOTIFICATION_GET_ITEM;
						message.listViewItem.index = i;
						message.listViewItem.mask = OS_LIST_VIEW_ITEM_TEXT | OS_LIST_VIEW_ITEM_SELECTED;
						message.listViewItem.state = 0;
		
						if (OSForwardMessage(control, control->notificationCallback, &message) != OS_CALLBACK_HANDLED) {
							OSCrashProcess(OS_FATAL_ERROR_MESSAGE_SHOULD_BE_HANDLED);
						}
		
						char *text = message.listViewItem.text;
						size_t textBytes = message.listViewItem.textBytes;
		
						OSString string;
						string.buffer = text;
						string.bytes = textBytes;

						{
							// If the previous row had a box drawn around it, then adjust the row's bounds slightly
							// to prevent a double-border at the boundary.
							bool adjustedRow = previousHadBox;
							if (adjustedRow) row.top--;

							previousHadBox = false;

							if (message.listViewItem.state & OS_LIST_VIEW_ITEM_SELECTED) {
								UIImage image = control->window->focus == control ? listViewSelected : listViewSelected2;
								OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, row,
										image.region, image.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
								previousHadBox = true;
							} else if (control->highlightRow == i) {
								OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, row,
										listViewHighlight.region, listViewHighlight.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
							}

							if (control->lastClickedRow == i && control->window->focus == control) {
								UIImage image = listViewLastClicked;
								OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, row,
										image.region, image.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
								previousHadBox = true;
							}

							if (adjustedRow) row.top++;
						}
		
						{
							OSRectangle region = row;
							region.left += LIST_VIEW_TEXT_MARGIN;
							region.right -= LIST_VIEW_TEXT_MARGIN;
							DrawString(surface, region, &string, OS_DRAW_STRING_HALIGN_LEFT | OS_DRAW_STRING_VALIGN_CENTER,
									0, -1, 0, OS_MAKE_POINT(0, 0), nullptr, 0, 0, true, FONT_SIZE, fontRegular, CLIP_RECTANGLE);
						}
					}
		
					PopClipRectangle();
					y += LIST_VIEW_ROW_HEIGHT;
		
					if (y > bounds.bottom - bounds.top) {
						break;
					}
				}
		
				PopClipRectangle();

				if (control->dragging == ListView::DRAGGING_SELECTION) {
					OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, control->selectionBox,
							listViewSelectionBox.region, listViewSelectionBox.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
				}
			}
		
			PopClipRectangle();
			result = OS_CALLBACK_HANDLED;
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (!IsPointInRectangle(control->bounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				control->highlightRow = -1;
				break;
			}

			OSSendMessage(control->scrollbar, message);

			if (!IsPointInRectangle(bounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				control->highlightRow = -1;
				break;
			}

			int y = message->mouseMoved.newPositionY - bounds.top + control->scrollY;
			control->highlightRow = y / LIST_VIEW_ROW_HEIGHT;
		} break;

		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			if (!IsPointInRectangle(control->bounds, message->mousePressed.positionX, message->mousePressed.positionY)) {
				break;
			}

			// We're going to want to repaint.
			OSRepaintControl(control);

			OSMessage m;

			if (!message->mousePressed.ctrl && !message->mousePressed.shift) {
				// If neither CTRL nor SHIFT were pressed, remove the old selection.
				m.type = OS_NOTIFICATION_DESELECT_ALL;
				OSForwardMessage(control, control->notificationCallback, &m);
			}

			if (!IsPointInRectangle(bounds, message->mousePressed.positionX, message->mousePressed.positionY)) {
				break;
			}

			// This is the row we clicked on.
			int y = message->mousePressed.positionY - bounds.top + control->scrollY;
			uintptr_t row = y / LIST_VIEW_ROW_HEIGHT;

			if (row >= control->itemCount || y < 0) {
				break;
			}

			if (message->mousePressed.shift && control->lastClickedRow != (uintptr_t) -1) {
				// If SHIFT was pressed, select every from the last clicked row to this row.
				uintptr_t low = row < control->lastClickedRow ? row : control->lastClickedRow;
				uintptr_t high = row > control->lastClickedRow ? row : control->lastClickedRow;

				m.type = OS_NOTIFICATION_SET_ITEM;
				m.listViewItem.mask = OS_LIST_VIEW_ITEM_SELECTED;
				m.listViewItem.state = OS_LIST_VIEW_ITEM_SELECTED;

				for (uintptr_t i = low; i <= high; i++) {
					m.listViewItem.index = i;
					OSForwardMessage(control, control->notificationCallback, &m);
				}
			} else if (message->mousePressed.ctrl) {
				// If CTRL was pressed, toggle whether this row is selected.
				m.type = OS_NOTIFICATION_GET_ITEM;
				m.listViewItem.index = row;
				m.listViewItem.mask = OS_LIST_VIEW_ITEM_SELECTED;
				m.listViewItem.state = 0;
				OSForwardMessage(control, control->notificationCallback, &m);
				m.type = OS_NOTIFICATION_SET_ITEM;
				m.listViewItem.state ^= OS_LIST_VIEW_ITEM_SELECTED;
				OSForwardMessage(control, control->notificationCallback, &m);
			} else {
				// If SHIFT wasn't pressed, only add this row to the selection.
				m.type = OS_NOTIFICATION_SET_ITEM;
				m.listViewItem.index = row;
				m.listViewItem.mask = OS_LIST_VIEW_ITEM_SELECTED;
				m.listViewItem.state = OS_LIST_VIEW_ITEM_SELECTED;
				OSForwardMessage(control, control->notificationCallback, &m);
			}

			control->lastClickedRow = row;
		} break;

		case OS_MESSAGE_START_DRAG: {
			control->dragging = ListView::DRAGGING_SELECTION;
			control->selectionBoxAnchor = OS_MAKE_POINT(message->mouseDragged.originalPositionX, message->mouseDragged.originalPositionY);
			control->selectionBoxPosition = OS_MAKE_POINT(message->mouseDragged.newPositionX, message->mouseDragged.newPositionY);

			int y = message->mouseDragged.originalPositionY - bounds.top + control->scrollY;
			int row = y / LIST_VIEW_ROW_HEIGHT;
			if (row < 0) row = -1;
			if (row >= (int) control->itemCount) row = control->itemCount;

			control->selectionBoxFirstRow = row;
			control->selectionBoxLastRow = row;
			OSRepaintControl(control);
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			control->dragging = ListView::DRAGGING_NONE;
			OSRepaintControl(control);
		} break;

		case OS_MESSAGE_MOUSE_DRAGGED: {
			control->selectionBoxPosition = OS_MAKE_POINT(message->mouseDragged.newPositionX, message->mouseDragged.newPositionY);
			OSRepaintControl(control);

			OSRectangle selectionBox;
			OSPoint anchor = control->selectionBoxAnchor;
			OSPoint position = control->selectionBoxPosition;

			if (anchor.x < position.x) {
				selectionBox.left = anchor.x;
				selectionBox.right = position.x;
			} else {
				selectionBox.right = anchor.x;
				selectionBox.left = position.x;
			}

			if (anchor.y < position.y) {
				selectionBox.top = anchor.y;
				selectionBox.bottom = position.y;
			} else {
				selectionBox.bottom = anchor.y;
				selectionBox.top = position.y;
			}

			control->selectionBox = selectionBox;

			int y1 = control->selectionBox.top - bounds.top + control->scrollY;
			int y2 = control->selectionBox.bottom - bounds.top + control->scrollY;

			int oldFirst = control->selectionBoxFirstRow;
			int oldLast = control->selectionBoxLastRow;

			int newFirst = y1 / LIST_VIEW_ROW_HEIGHT;
			int newLast = y2 / LIST_VIEW_ROW_HEIGHT;

			control->selectionBoxFirstRow = newFirst;
			control->selectionBoxLastRow = newLast;

			if (newFirst < 0) newFirst = -1; else if (newFirst > (int) control->itemCount) newFirst = control->itemCount;
			if (newLast < 0) newLast = -1; else if (newLast > (int) control->itemCount) newLast = control->itemCount;

			OSMessage m;
			m.type = OS_NOTIFICATION_SET_ITEM;
			m.listViewItem.mask = OS_LIST_VIEW_ITEM_SELECTED;
			m.listViewItem.state = OS_LIST_VIEW_ITEM_SELECTED;

			while (newFirst < oldFirst) {
				m.listViewItem.index = --oldFirst;

				if (m.listViewItem.index >= 0 && m.listViewItem.index < (int) control->itemCount) {
					OSForwardMessage(control, control->notificationCallback, &m);
				}
			}

			while (newLast > oldLast) {
				m.listViewItem.index = ++oldLast;

				if (m.listViewItem.index >= 0 && m.listViewItem.index < (int) control->itemCount) {
					OSForwardMessage(control, control->notificationCallback, &m);
				}
			}

			m.listViewItem.state = 0;

			while (oldFirst < newFirst) {
				m.listViewItem.index = oldFirst++;

				if (m.listViewItem.index >= 0 && m.listViewItem.index < (int) control->itemCount) {
					OSForwardMessage(control, control->notificationCallback, &m);
				}
			}

			while (oldLast > newLast) {
				m.listViewItem.index = oldLast--;

				if (m.listViewItem.index >= 0 && m.listViewItem.index < (int) control->itemCount) {
					OSForwardMessage(control, control->notificationCallback, &m);
				}
			}
		} break;

		case OS_MESSAGE_END_HOVER: {
			control->highlightRow = -1;
		} break;

		case OS_MESSAGE_CHILD_UPDATED: {
			SetParentDescendentInvalidationFlags(control, DESCENDENT_RELAYOUT);
		} break;

		case OS_MESSAGE_PARENT_UPDATED:
		case OS_MESSAGE_DESTROY: {
			// Propagate the message to our scrollbar.
			OSSendMessage(control->scrollbar, message);
		} break;

		default: {
			// The message is not handled.
		} break;
	}

	if (previousHighlightRow != control->highlightRow) {
		OSRepaintControl(control);
	}

	if (result == OS_CALLBACK_NOT_HANDLED) {
		result = OSForwardMessage(object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
	}

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			// Adjust the layout we're given, and tell the scrollbar where it lives.

			control->inputBounds = control->bounds;

			if (control->flags & OS_CREATE_LIST_VIEW_BORDER) {
				control->inputBounds.left += STANDARD_BORDER_SIZE;
				control->inputBounds.top += STANDARD_BORDER_SIZE;
				control->inputBounds.right -= STANDARD_BORDER_SIZE;
				control->inputBounds.bottom -= STANDARD_BORDER_SIZE;
			}

			control->inputBounds.right -= SCROLLBAR_SIZE;

			OSMessage m;
			m.type = OS_MESSAGE_LAYOUT;
			m.layout.force = true;

			if (control->flags & OS_CREATE_LIST_VIEW_BORDER) {
				m.layout.left = control->bounds.right - SCROLLBAR_SIZE - STANDARD_BORDER_SIZE;
				m.layout.right = control->bounds.right - STANDARD_BORDER_SIZE;
				m.layout.top = control->bounds.top + STANDARD_BORDER_SIZE;
				m.layout.bottom = control->bounds.bottom - STANDARD_BORDER_SIZE;
			} else {
				m.layout.left = control->bounds.right - SCROLLBAR_SIZE;
				m.layout.right = control->bounds.right;
				m.layout.top = control->bounds.top;
				m.layout.bottom = control->bounds.bottom;
			}

			OSSetScrollbarMeasurements(control->scrollbar, control->itemCount * LIST_VIEW_ROW_HEIGHT, 
					control->bounds.bottom - control->bounds.top - ((control->flags & OS_CREATE_LIST_VIEW_BORDER) ? LIST_VIEW_WITH_BORDER_MARGIN : LIST_VIEW_MARGIN));

			OSSendMessage(control->scrollbar, &m);
			control->descendentInvalidationFlags &= ~DESCENDENT_RELAYOUT;
		} break;

		case OS_MESSAGE_PAINT: {
			// Draw the scrollbar after the list.
			if (control->descendentInvalidationFlags & DESCENDENT_REPAINT || message->paint.force || redrawScrollbar) {
				control->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;
				OSMessage m = *message;
				m.paint.force = message->paint.force || redrawScrollbar;
				OSSendMessage(control->scrollbar, &m);
			}
		} break;


		default: {
			// We don't do any further processing with the message.
		} break;
	}

	return result;
}

OSObject OSCreateListView(unsigned flags) {
	ListView *control = (ListView *) OSHeapAllocate(sizeof(ListView), true);
	control->type = API_OBJECT_CONTROL;
	control->flags = flags;

	control->preferredWidth = 40;
	control->preferredHeight = 40;
	control->minimumWidth = 20;
	control->minimumHeight = 20;
	control->ignoreActivationClicks = true;
	control->noAnimations = true;
	control->focusable = true;

	if (flags & OS_CREATE_LIST_VIEW_BORDER) {
		control->drawParentBackground = true;
		control->backgrounds = textboxBackgrounds;
	}

	control->scrollbar = OSCreateScrollbar(OS_ORIENTATION_VERTICAL);
	((Control *) control->scrollbar)->parent = control;
	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessListViewMessage, nullptr));
	OSSetObjectNotificationCallback(control->scrollbar, OS_MAKE_CALLBACK(ListViewScrollbarMoved, control));

	control->lastClickedRow = -1;
	control->highlightRow = -1;

	{
		OSMessage message;
		message.parentUpdated.window = nullptr;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(control->scrollbar, &message);
	}

	return control;
}

void OSListViewInsert(OSObject _listView, uintptr_t index, size_t count) {
	(void) index;
	ListView *listView = (ListView *) _listView;
	listView->itemCount += count;
	OSRepaintControl(listView);
}

static OSObject CreateMenuItem(OSMenuItem item, bool menubar) {
	MenuItem *control = (MenuItem *) OSHeapAllocate(sizeof(MenuItem), true);
	control->type = API_OBJECT_CONTROL;

	control->preferredWidth = !menubar ? 70 : 21;
	control->preferredHeight = 21;
	control->minimumWidth = !menubar ? 70 : 21;
	control->minimumHeight = 21;
	control->drawParentBackground = true;
	control->textAlign = menubar ? OS_FLAGS_DEFAULT : (OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT);
	control->backgrounds = menuItemBackgrounds;
	control->ignoreActivationClicks = menubar;
	control->noAnimations = true;

	control->item = item;
	control->menubar = menubar;

	if (item.type == OSMenuItem::COMMAND) {
		OSCommand *command = (OSCommand *) item.value;
		SetControlCommand(control, command);
		OSSetText(control, command->label, command->labelBytes);
	} else if (item.type == OSMenuItem::SUBMENU) {
		OSMenuSpecification *menu = (OSMenuSpecification *) item.value;
		OSSetText(control, menu->name, menu->nameBytes);
		if (!menubar) control->icons = smallArrowRightIcons;
	}

	if (menubar) {
		control->preferredWidth += 4;
	} else {
		control->preferredWidth += 32;
	}

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessMenuItemMessage, nullptr));

	return control;
}

OSObject OSCreateButton(OSCommand *command) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;

#ifndef SUPER_COOL_BUTTONS
	control->preferredWidth = 80;
	control->preferredHeight = 21;
#else
	control->preferredWidth = 86;
	control->preferredHeight = 29;

	if (!command->checkable) {
		control->textColor = 0xFFFFFF;
		control->textShadow = true;
	}
#endif

	control->drawParentBackground = true;
	control->ignoreActivationClicks = true;

	SetControlCommand(control, command);

	if (control->checkable) {
		UpdateCheckboxIcons(control);
		control->textAlign = OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT;
		control->minimumHeight = control->icons[0]->region.bottom - control->icons[0]->region.top;
		control->checkboxIcons = true;
		control->minimumWidth = control->icons[0]->region.right - control->icons[0]->region.left;
	} else {
		control->backgrounds = command->dangerous ? buttonDangerousBackgrounds : buttonBackgrounds;
		control->minimumWidth = 80;
		control->minimumHeight = 21;
	}

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr));
	OSSetText(control, command->label, command->labelBytes);

	return control;
}

OSObject OSCreateLine(bool orientation) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;
	control->backgrounds = orientation ? lineVerticalBackgrounds : lineHorizontalBackgrounds;
	control->drawParentBackground = true;

	control->preferredWidth = 4;
	control->preferredHeight = 4;
	control->minimumWidth = 4;
	control->minimumHeight = 4;

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr));

	return control;
}

OSObject OSCreateLabel(char *text, size_t textBytes) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;
	control->drawParentBackground = true;

	OSSetText(control, text, textBytes);
	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr));

	control->minimumWidth = control->preferredWidth;
	control->minimumHeight = control->preferredHeight;

	return control;
}

static OSCallbackResponse ProcessProgressBarMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	ProgressBar *control = (ProgressBar *) _object;

	if (message->type == OS_MESSAGE_PAINT) {
		if (control->repaint || message->paint.force) {
			{
				OSMessage m = *message;
				m.type = OS_MESSAGE_PAINT_BACKGROUND;
				m.paintBackground.surface = message->paint.surface;
				m.paintBackground.left = control->bounds.left;
				m.paintBackground.right = control->bounds.right;
				m.paintBackground.top = control->bounds.top;
				m.paintBackground.bottom = control->bounds.bottom;
				OSSendMessage(control->parent, &m);
			}

			if (control->disabled) {
				OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, 
						control->backgrounds[1]->region, control->backgrounds[1]->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
			} else {
				OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, 
						control->backgrounds[0]->region, control->backgrounds[0]->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);

				int pelletCount = (control->bounds.right - control->bounds.left - 6) / 9;

				if (control->maximum) {
					float progress = (float) (control->value - control->minimum) / (float) (control->maximum - control->minimum);

					pelletCount *= progress;

					for (int i = 0; i < pelletCount; i++) {
						OSRectangle bounds = control->bounds;
						bounds.top += 3;
						bounds.bottom = bounds.top + 15;
						bounds.left += 3 + i * 9;
						bounds.right = bounds.left + 8;
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds,
								progressBarPellet.region, progressBarPellet.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
					}
				} else {
					if (control->value >= pelletCount) control->value -= pelletCount;

					for (int i = 0; i < 4; i++) {
						int j = i + control->value;
						if (j >= pelletCount) j -= pelletCount;
						OSRectangle bounds = control->bounds;
						bounds.top += 3;
						bounds.bottom = bounds.top + 15;
						bounds.left += 3 + j * 9;
						bounds.right = bounds.left + 8;
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds,
								progressBarPellet.region, progressBarPellet.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
					}
				}
			}

			control->repaint = false;
		}
	} else if (message->type == OS_MESSAGE_PARENT_UPDATED) {
		OSForwardMessage(_object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
		control->timerHz = 5;
		control->window->timerControls.InsertStart(&control->timerControlItem);
	} else if (message->type == OS_MESSAGE_WM_TIMER) {
		OSSetProgressBarValue(control, control->value + 1);
	} else {
		response = OSForwardMessage(_object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
	}

	return response;
}

void OSSetProgressBarValue(OSObject _control, int newValue) {
	ProgressBar *control = (ProgressBar *) _control;
	control->value = newValue;
	OSRepaintControl(control);
}

OSObject OSCreateProgressBar(int minimum, int maximum, int initialValue) {
	ProgressBar *control = (ProgressBar *) OSHeapAllocate(sizeof(ProgressBar), true);

	control->type = API_OBJECT_CONTROL;
	control->backgrounds = progressBarBackgrounds;

	control->minimum = minimum;
	control->maximum = maximum;
	control->value = initialValue;

	control->preferredWidth = 168;
	control->preferredHeight = 21;
	control->minimumWidth = 168;
	control->minimumHeight = 21;

	control->drawParentBackground = true;

	if (!control->maximum) {
		// Indeterminate progress bar.
		control->timerControlItem.thisItem = control;
	}

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessProgressBarMessage, nullptr));

	return control;
}

void OSSetInstance(OSObject _window, void *instance) {
	((Window *) _window)->instance = instance;
}

void *OSGetInstance(OSObject _window) {
	return ((Window *) _window)->instance;
}

void OSGetText(OSObject _control, OSString *string) {
	Control *control = (Control *) _control;
	*string = control->text;
}

void OSSetText(OSObject _control, char *text, size_t textBytes) {
	Control *control = (Control *) _control;
	CreateString(text, textBytes, &control->text);

	int suggestedWidth = MeasureStringWidth(text, textBytes, FONT_SIZE, fontRegular) + 8;
	int suggestedHeight = FONT_SIZE + 8;

	if (control->icons && control->icons[UI_IMAGE_NORMAL]) {
		suggestedWidth += control->icons[UI_IMAGE_NORMAL]->region.right - control->icons[0]->region.left + 4;
	}

	if (suggestedWidth > control->minimumWidth) control->preferredWidth = suggestedWidth;
	if (suggestedHeight > control->minimumHeight) control->preferredHeight = suggestedHeight;

	control->relayout = true;
	OSRepaintControl(control);

	{
		OSMessage message;
		message.type = OS_MESSAGE_CHILD_UPDATED;
		OSSendMessage(control->parent, &message);
	}

	{
		OSMessage message;
		message.type = OS_MESSAGE_TEXT_UPDATED;
		OSSendMessage(control, &message);
	}
}

void OSAddControl(OSObject _grid, unsigned column, unsigned row, OSObject _control, unsigned layout) {
	GUIObject *_object = (GUIObject *) _grid;

	if (_object->type == API_OBJECT_WINDOW) {
		Window *window = (Window *) _grid;
		_grid = window->root;

		if (window->flags & OS_CREATE_WINDOW_MENU) {
			column = 0;
			row = 0;
		} else if (window->flags & OS_CREATE_WINDOW_WITH_MENUBAR) {
			_grid = window->root->objects[7];
			column = 0;
			row = 1;
		} else if (window->flags & OS_CREATE_WINDOW_NORMAL) {
			column = 1;
			row = 2;
		}
	}

	Grid *grid = (Grid *) _grid;

	grid->relayout = true;
	SetParentDescendentInvalidationFlags(grid, DESCENDENT_RELAYOUT);

	if (column >= grid->columns || row >= grid->rows) {
		OSCrashProcess(OS_FATAL_ERROR_OUT_OF_GRID_BOUNDS);
	}

	GUIObject *control = (GUIObject *) _control;
	if (control->type != API_OBJECT_CONTROL && control->type != API_OBJECT_GRID) OSCrashProcess(OS_FATAL_ERROR_INVALID_PANE_OBJECT);
	control->layout = layout;
	control->parent = grid;

	GUIObject **object = grid->objects + (row * grid->columns + column);
	if (*object) OSCrashProcess(OS_FATAL_ERROR_OVERWRITE_GRID_OBJECT);
	*object = control;

	{
		OSMessage message;
		message.parentUpdated.window = grid->window;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(control, &message);
	}
}

static OSCallbackResponse ProcessGridMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Grid *grid = (Grid *) _object;

	switch (message->type) {
		case OS_MESSAGE_LAYOUT: {
			if (grid->relayout || message->layout.force) {
				grid->relayout = false;

				grid->bounds = OS_MAKE_RECTANGLE(
						message->layout.left, message->layout.right,
						message->layout.top, message->layout.bottom);

				if (!grid->layout) grid->layout = OS_CELL_H_EXPAND | OS_CELL_V_EXPAND;
				StandardCellLayout(grid);

				OSZeroMemory(grid->widths, sizeof(int) * grid->columns);
				OSZeroMemory(grid->heights, sizeof(int) * grid->rows);
				OSZeroMemory(grid->minimumWidths, sizeof(int) * grid->columns);
				OSZeroMemory(grid->minimumHeights, sizeof(int) * grid->rows);

				int pushH = 0, pushV = 0;

#if 0
				OSPrint("->Laying out grid %x (%d by %d) into %d->%d, %d->%d (given %d->%d, %d->%d), layout = %X%X\n", 
 						grid, grid->columns, grid->rows, grid->bounds.left, grid->bounds.right, grid->bounds.top, grid->bounds.bottom,
 						message->layout.left, message->layout.right, message->layout.top, message->layout.bottom, grid->layout);
#endif

				for (uintptr_t i = 0; i < grid->columns; i++) {
					for (uintptr_t j = 0; j < grid->rows; j++) {
						GUIObject **object = grid->objects + (j * grid->columns + i);
						message->type = OS_MESSAGE_MEASURE;
						if (OSSendMessage(*object, message) == OS_CALLBACK_NOT_HANDLED) continue;

						int width = message->measure.preferredWidth;
						int height = message->measure.preferredHeight;

						// OSPrint("Measuring %d, %d: %d, %d, %d, %d\n", i, j, width, height, message->measure.minimumWidth, message->measure.minimumHeight);

						if (width == DIMENSION_PUSH) { bool a = grid->widths[i] == DIMENSION_PUSH; grid->widths[i] = DIMENSION_PUSH; if (!a) pushH++; }
						else if (grid->widths[i] < width && grid->widths[i] != DIMENSION_PUSH) grid->widths[i] = width;
						if (height == DIMENSION_PUSH) { bool a = grid->heights[j] == DIMENSION_PUSH; grid->heights[j] = DIMENSION_PUSH; if (!a) pushV++; }
						else if (grid->heights[j] < height && grid->heights[j] != DIMENSION_PUSH) grid->heights[j] = height;

						if (grid->minimumWidths[i] < message->measure.minimumWidth) grid->minimumWidths[i] = message->measure.minimumWidth;
						if (grid->minimumHeights[j] < message->measure.minimumHeight) grid->minimumHeights[j] = message->measure.minimumHeight;
					}
				}

#if 0
				OSPrint("->Results for grid %x (%d by %d)\n", grid, grid->columns, grid->rows);

				for (uintptr_t i = 0; i < grid->columns; i++) {
					OSPrint("Column %d is pref: %dpx, min: %dpx\n", i, grid->widths[i], grid->minimumWidths[i]);
				}

				for (uintptr_t j = 0; j < grid->rows; j++) {
					OSPrint("Row %d is pref: %dpx, min: %dpx\n", j, grid->heights[j], grid->minimumHeights[j]);
				}
#endif

				if (pushH) {
					int usedWidth = grid->borderSize * 2 + grid->gapSize * (grid->columns - 1); 
					for (uintptr_t i = 0; i < grid->columns; i++) if (grid->widths[i] != DIMENSION_PUSH) usedWidth += grid->widths[i];
					int widthPerPush = (grid->bounds.right - grid->bounds.left - usedWidth) / pushH;

					for (uintptr_t i = 0; i < grid->columns; i++) {
						if (grid->widths[i] == DIMENSION_PUSH) {
							if (widthPerPush >= grid->minimumWidths[i]) {
								grid->widths[i] = widthPerPush;
							} else {
								grid->widths[i] = grid->minimumWidths[i];
							}
						}
					}
				}

				if (pushV) {
					int usedHeight = grid->borderSize * 2 + grid->gapSize * (grid->rows - 1); 
					for (uintptr_t j = 0; j < grid->rows; j++) if (grid->heights[j] != DIMENSION_PUSH) usedHeight += grid->heights[j];
					int heightPerPush = (grid->bounds.bottom - grid->bounds.top - usedHeight) / pushV; 

					for (uintptr_t j = 0; j < grid->rows; j++) {
						if (grid->heights[j] == DIMENSION_PUSH) {
							if (heightPerPush >= grid->minimumHeights[j]) {
								grid->heights[j] = heightPerPush;
							} else {
								grid->heights[j] = grid->minimumHeights[j];
							}
						}
					}
				}

				PushClipRectangle(grid->bounds);

				int posX = grid->bounds.left + grid->borderSize - grid->xOffset;

				for (uintptr_t i = 0; i < grid->columns; i++) {
					int posY = grid->bounds.top + grid->borderSize - grid->yOffset;

					for (uintptr_t j = 0; j < grid->rows; j++) {
						GUIObject **object = grid->objects + (j * grid->columns + i);

						message->type = OS_MESSAGE_LAYOUT;
						message->layout.left = posX;
						message->layout.right = posX + grid->widths[i];
						message->layout.top = posY;
						message->layout.bottom = posY + grid->heights[j];
						message->layout.force = true;

						// OSPrint("Sending %d->%d, %d->%d to %d,%d\n", message->layout.left, message->layout.right, message->layout.top, message->layout.bottom, i, j);
						OSSendMessage(*object, message);

						posY += grid->heights[j] + grid->gapSize;
					}

					posX += grid->widths[i] + grid->gapSize;
				}

				PopClipRectangle();

				grid->repaint = true;
				SetParentDescendentInvalidationFlags(grid, DESCENDENT_REPAINT);
			} else if (grid->descendentInvalidationFlags & DESCENDENT_RELAYOUT) {
				for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
					if (grid->objects[i]) {
						message->layout.left   = ((GUIObject *) grid->objects[i])->cellBounds.left;
						message->layout.right  = ((GUIObject *) grid->objects[i])->cellBounds.right;
						message->layout.top    = ((GUIObject *) grid->objects[i])->cellBounds.top;
						message->layout.bottom = ((GUIObject *) grid->objects[i])->cellBounds.bottom;

						OSSendMessage(grid->objects[i], message);
					}
				}
			}

			grid->descendentInvalidationFlags &= ~DESCENDENT_RELAYOUT;
		} break;

		case OS_MESSAGE_MEASURE: {
			OSZeroMemory(grid->widths, sizeof(int) * grid->columns);
			OSZeroMemory(grid->heights, sizeof(int) * grid->rows);
			OSZeroMemory(grid->minimumWidths, sizeof(int) * grid->columns);
			OSZeroMemory(grid->minimumHeights, sizeof(int) * grid->rows);

			bool pushH = false, pushV = false;

			for (uintptr_t i = 0; i < grid->columns; i++) {
				for (uintptr_t j = 0; j < grid->rows; j++) {
					GUIObject **object = grid->objects + (j * grid->columns + i);
					if (OSSendMessage(*object, message) == OS_CALLBACK_NOT_HANDLED) continue;

					int width = message->measure.preferredWidth;
					int height = message->measure.preferredHeight;

					int minimumWidth = message->measure.minimumWidth;
					int minimumHeight = message->measure.minimumHeight;

					if (width == DIMENSION_PUSH) pushH = true;
					if (height == DIMENSION_PUSH) pushV = true;

					if (grid->widths[i] < width) grid->widths[i] = width;
					if (grid->heights[j] < height) grid->heights[j] = height;

					if (grid->minimumWidths[i] < minimumWidth) grid->minimumWidths[i] = minimumWidth;
					if (grid->minimumHeights[j] < minimumHeight) grid->minimumHeights[j] = minimumHeight;
				}
			}

			int width = pushH ? DIMENSION_PUSH : grid->borderSize, height = pushV ? DIMENSION_PUSH : grid->borderSize;
			int minimumWidth = grid->borderSize, minimumHeight = grid->borderSize;

			if (!pushH) for (uintptr_t i = 0; i < grid->columns; i++) width += grid->widths[i] + (i == grid->columns - 1 ? grid->borderSize : grid->gapSize);
			if (!pushV) for (uintptr_t j = 0; j < grid->rows; j++) height += grid->heights[j] + (j == grid->rows - 1 ? grid->borderSize : grid->gapSize);
			for (uintptr_t i = 0; i < grid->columns; i++) minimumWidth += grid->minimumWidths[i] + (i == grid->columns - 1 ? grid->borderSize : grid->gapSize);
			for (uintptr_t j = 0; j < grid->rows; j++) minimumHeight += grid->minimumHeights[j] + (j == grid->rows - 1 ? grid->borderSize : grid->gapSize);

			grid->preferredWidth = message->measure.preferredWidth = width;
			grid->preferredHeight = message->measure.preferredHeight = height;
			grid->minimumWidth = message->measure.minimumWidth = minimumWidth;
			grid->minimumHeight = message->measure.minimumHeight = minimumHeight;
		} break;

		case OS_MESSAGE_PAINT: {
			if (grid->descendentInvalidationFlags & DESCENDENT_REPAINT || grid->repaint || message->paint.force) {
				grid->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;

				OSMessage m = *message;
				m.paint.force = message->paint.force || grid->repaint;
				grid->repaint = false;

				if (PushClipRectangle(grid->bounds)) {
					if (m.paint.force) {
						if (grid->background) {
							OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, grid->bounds, grid->background->region,
									grid->background->border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
						} else if (grid->backgroundColor) {
							if (PushClipRectangle(grid->bounds)) {
								OSFillRectangle(message->paint.surface, CLIP_RECTANGLE, OSColor(grid->backgroundColor));
							}

							PopClipRectangle();
						}
					}

					for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
						if (grid->objects[i]) {
							if (PushClipRectangle(grid->objects[i]->bounds)) {
								OSSendMessage(grid->objects[i], &m);
							}

							PopClipRectangle();
						}
					}
				}

				PopClipRectangle();
			}
		} break;

		case OS_MESSAGE_PAINT_BACKGROUND: {
			OSRectangle destination = OS_MAKE_RECTANGLE(message->paintBackground.left, message->paintBackground.right, 
					message->paintBackground.top, message->paintBackground.bottom);
			OSRectangle full = OS_MAKE_RECTANGLE(grid->bounds.left, grid->bounds.right, 
					grid->bounds.top, grid->bounds.bottom);

			if (grid->background) {
				OSRectangle region = grid->background->region;
				OSRectangle border = grid->background->border;

				if (PushClipRectangle(destination)) {
					OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, full, region,
							border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
				}

				PopClipRectangle();
			} else if (grid->backgroundColor) {
				if (PushClipRectangle(destination)) {
					OSFillRectangle(message->paint.surface, CLIP_RECTANGLE, OSColor(grid->backgroundColor));
				}

				PopClipRectangle();
			}
		} break;

		case OS_MESSAGE_PARENT_UPDATED: {
			grid->window = (Window *) message->parentUpdated.window;

			for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
				OSSendMessage(grid->objects[i], message);
			}
		} break;

		case OS_MESSAGE_CHILD_UPDATED: {
			grid->relayout = true;
			SetParentDescendentInvalidationFlags(grid, DESCENDENT_RELAYOUT);
		} break;

		case OS_MESSAGE_DESTROY: {
			for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
				OSSendMessage(grid->objects[i], message);
			}

			OSHeapFree(grid);
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (!IsPointInRectangle(grid->bounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				break;
			}

			for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
				OSSendMessage(grid->objects[i], message);
			}
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

OSObject OSCreateGrid(unsigned columns, unsigned rows, unsigned flags) {
	uint8_t *memory = (uint8_t *) OSHeapAllocate(sizeof(Grid) + sizeof(OSObject) * columns * rows + 2 * sizeof(int) * (columns + rows), true);

	Grid *grid = (Grid *) memory;
	grid->type = API_OBJECT_GRID;

	grid->backgroundColor = STANDARD_BACKGROUND_COLOR;
	grid->columns = columns;
	grid->rows = rows;
	grid->objects = (GUIObject **) (memory + sizeof(Grid));
	grid->widths = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows);
	grid->heights = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns);
	grid->minimumWidths = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns + sizeof(int) * rows);
	grid->minimumHeights = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns + sizeof(int) * rows + sizeof(int) * columns);
	grid->flags = flags;

	if (flags & OS_CREATE_GRID_NO_BORDER) grid->borderSize = 0; else grid->borderSize = (flags & OS_CREATE_GRID_MENU) ? 4 : 8;
	if (flags & OS_CREATE_GRID_NO_GAP) grid->gapSize = 0; else grid->gapSize = (flags & OS_CREATE_GRID_MENU) ? 4 : 6;
	if (flags & OS_CREATE_GRID_DRAW_BOX) { grid->borderSize += 4; grid->background = &gridBox;  }
	if (flags & OS_CREATE_GRID_NO_BACKGROUND) { grid->backgroundColor = 0; }

	OSSetCallback(grid, OS_MAKE_CALLBACK(ProcessGridMessage, nullptr));

	return grid;
}


static OSCallbackResponse ProcessScrollPaneMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_NOT_HANDLED;
	Grid *grid = (Grid *) _object;

	if (message->type == OS_MESSAGE_LAYOUT) {
		if (grid->relayout || message->layout.force) {
			grid->relayout = false;
			grid->bounds = OS_MAKE_RECTANGLE(
					message->layout.left, message->layout.right,
					message->layout.top, message->layout.bottom);
			grid->repaint = true;
			SetParentDescendentInvalidationFlags(grid, DESCENDENT_REPAINT);

#if 0
			OSPrint("->Laying out grid %x (%d by %d) into %d->%d, %d->%d (given %d->%d, %d->%d), layout = %X\n", 
					grid, grid->columns, grid->rows, grid->bounds.left, grid->bounds.right, grid->bounds.top, grid->bounds.bottom,
					message->layout.left, message->layout.right, message->layout.top, message->layout.bottom, grid->layout);
#endif

			OSMessage m = {};
			m.type = OS_MESSAGE_MEASURE;
			OSCallbackResponse r = OSSendMessage(grid->objects[0], &m);
			if (r != OS_CALLBACK_HANDLED) OSCrashProcess(OS_FATAL_ERROR_MESSAGE_SHOULD_BE_HANDLED);

			int contentWidth = message->layout.right - message->layout.left - (grid->objects[1] ? SCROLLBAR_SIZE : 0);
			int contentHeight = message->layout.bottom - message->layout.top - (grid->objects[2] ? SCROLLBAR_SIZE : 0);
			int minimumWidth = m.measure.minimumWidth;
			int minimumHeight = m.measure.minimumHeight;

			m.type = OS_MESSAGE_LAYOUT;
			m.layout.force = true;

			if (grid->objects[1]) {
				m.layout.top = message->layout.top;
				m.layout.bottom = message->layout.top + contentHeight;
				m.layout.left = message->layout.right - SCROLLBAR_SIZE;
				m.layout.right = message->layout.right;
				OSSendMessage(grid->objects[1], &m);
				OSSetScrollbarMeasurements(grid->objects[1], minimumHeight, contentHeight);
			}

			if (grid->objects[2]) {
				m.layout.top = message->layout.bottom - SCROLLBAR_SIZE;
				m.layout.bottom = message->layout.bottom;
				m.layout.left = message->layout.left;
				m.layout.right = message->layout.left + contentWidth;
				OSSendMessage(grid->objects[2], &m);
				OSSetScrollbarMeasurements(grid->objects[2], minimumWidth, contentWidth);
			}

			m.layout.top = message->layout.top;
			m.layout.bottom = message->layout.top + contentHeight;
			m.layout.left = message->layout.left;
			m.layout.right = message->layout.left + contentWidth;
			OSSendMessage(grid->objects[0], &m);

			response = OS_CALLBACK_HANDLED;
		}
	} else if (message->type == OS_MESSAGE_MEASURE) {
		message->measure.preferredWidth = DIMENSION_PUSH;
		message->measure.preferredHeight = DIMENSION_PUSH;
		message->measure.minimumWidth = SCROLLBAR_SIZE * 3;
		message->measure.minimumHeight = SCROLLBAR_SIZE * 3;
		response = OS_CALLBACK_HANDLED;
	}

	if (response == OS_CALLBACK_NOT_HANDLED) {
		response = OSForwardMessage(_object, OS_MAKE_CALLBACK(ProcessGridMessage, nullptr), message);
	}

	return response;
}

OSCallbackResponse ScrollPaneBarMoved(OSObject _object, OSMessage *message) {
	Scrollbar *scrollbar = (Scrollbar *) _object;
	Grid *grid = (Grid *) message->context;

	if (scrollbar->orientation) {
		// Vertical scrollbar.
		grid->yOffset = message->valueChanged.newValue;
	} else {
		// Horizontal scrollbar.
		grid->xOffset = message->valueChanged.newValue;
	}

	SetParentDescendentInvalidationFlags(grid, DESCENDENT_RELAYOUT);
	grid->relayout = true;

	return OS_CALLBACK_HANDLED;
}

OSObject OSCreateScrollPane(OSObject content, unsigned flags) {
	OSObject grid = OSCreateGrid(2, 2, OS_CREATE_GRID_NO_GAP | OS_CREATE_GRID_NO_BORDER);
	OSSetCallback(grid, OS_MAKE_CALLBACK(ProcessScrollPaneMessage, nullptr));
	OSAddGrid(grid, 0, 0, content, OS_CELL_FILL);

	if (flags & OS_CREATE_SCROLL_PANE_VERTICAL) {
		OSObject scrollbar = OSCreateScrollbar(OS_ORIENTATION_VERTICAL);
		OSAddGrid(grid, 1, 0, scrollbar, OS_FLAGS_DEFAULT);
		OSSetObjectNotificationCallback(scrollbar, OS_MAKE_CALLBACK(ScrollPaneBarMoved, content));
		// OSPrint("vertical %x\n", scrollbar);
	}

	if (flags & OS_CREATE_SCROLL_PANE_HORIZONTAL) {
		OSObject scrollbar = OSCreateScrollbar(OS_ORIENTATION_HORIZONTAL);
		OSAddGrid(grid, 0, 1, scrollbar, OS_FLAGS_DEFAULT);
		OSSetObjectNotificationCallback(scrollbar, OS_MAKE_CALLBACK(ScrollPaneBarMoved, content));
		// OSPrint("horizontal %x\n", scrollbar);
	}

	return grid;
}

#define SCROLLBAR_BUTTON_UP   ((void *) 1)
#define SCROLLBAR_BUTTON_DOWN ((void *) 2)
#define SCROLLBAR_NUDGE_UP    ((void *) 3)
#define SCROLLBAR_NUDGE_DOWN  ((void *) 4)

int OSGetScrollbarPosition(OSObject object) {
	Scrollbar *scrollbar = (Scrollbar *) object;

	int position = 0;

	if (scrollbar->enabled) {
		float fraction = (float) scrollbar->position / (float) scrollbar->maxPosition;
		float range = (float) (scrollbar->contentSize - scrollbar->viewportSize);
		position = (int) (fraction * range);
	}

	return position;
}

static void ScrollbarPositionChanged(Scrollbar *scrollbar) {
	OSMessage message;
	message.type = OS_NOTIFICATION_VALUE_CHANGED;
	message.valueChanged.newValue = OSGetScrollbarPosition(scrollbar);
	OSForwardMessage(scrollbar, scrollbar->notificationCallback, &message);
}

static OSCallbackResponse ScrollbarButtonPressed(OSObject object, OSMessage *message) {
	Control *button = (Control *) object;
	Scrollbar *scrollbar = (Scrollbar *) button->context;

	int position = OSGetScrollbarPosition(scrollbar);
	int amount = SCROLLBAR_BUTTON_AMOUNT;

	if (message->context == SCROLLBAR_BUTTON_UP) {
		position -= amount;
	} else if (message->context == SCROLLBAR_BUTTON_DOWN) {
		position += amount;
	} else if (message->context == SCROLLBAR_NUDGE_UP) {
		position -= scrollbar->viewportSize / 2;
	} else if (message->context == SCROLLBAR_NUDGE_DOWN) {
		position += scrollbar->viewportSize / 2;
	}

	OSSetScrollbarPosition(scrollbar, position, true);

	return OS_CALLBACK_HANDLED;
}

void OSSetScrollbarPosition(OSObject object, int newPosition, bool sendValueChangedNotification) {
	Scrollbar *scrollbar = (Scrollbar *) object;

	if (scrollbar->enabled) {
		float range = (float) (scrollbar->contentSize - scrollbar->viewportSize);
		float fraction = (float) newPosition / range;
		scrollbar->position = (int) (fraction * (float) scrollbar->maxPosition);

		if (scrollbar->position < 0) scrollbar->position = 0;
		else if (scrollbar->position >= scrollbar->maxPosition) scrollbar->position = scrollbar->maxPosition;

		if (sendValueChangedNotification) {
			ScrollbarPositionChanged(scrollbar);
		}

		{
			OSMessage message;
			message.type = OS_MESSAGE_CHILD_UPDATED;
			OSSendMessage(scrollbar, &message);
		}
	}
}

static OSCallbackResponse ProcessScrollbarGripMessage(OSObject object, OSMessage *message) {
	Control *grip = (Control *) object;
	Scrollbar *scrollbar = (Scrollbar *) grip->context;

	if (scrollbar->enabled) {
		if (message->type == OS_MESSAGE_START_DRAG) {
			scrollbar->anchor = scrollbar->orientation ? message->mouseMoved.newPositionY : message->mouseMoved.newPositionX;
		} else if (message->type == OS_MESSAGE_MOUSE_DRAGGED) {
			{
				OSMessage message;
				message.type = OS_MESSAGE_CHILD_UPDATED;
				OSSendMessage(scrollbar, &message);
			}

			int mouse = scrollbar->orientation ? message->mouseDragged.newPositionY : message->mouseDragged.newPositionX;
			scrollbar->position += mouse - scrollbar->anchor;

			if (scrollbar->position < 0) scrollbar->position = 0;
			else if (scrollbar->position >= scrollbar->maxPosition) scrollbar->position = scrollbar->maxPosition;
			else scrollbar->anchor = mouse;

			ScrollbarPositionChanged(scrollbar);
		} else if (message->type == OS_MESSAGE_CUSTOM_PAINT) {
			OSRectangle bounds;

			if (scrollbar->orientation) {
				bounds.left = grip->bounds.left + 6;
				bounds.right = bounds.left + 5;
				bounds.top = (grip->bounds.top + grip->bounds.bottom) / 2 - 4;
				bounds.bottom = bounds.top + 8;
			} else {
				bounds.top = grip->bounds.top + 6;
				bounds.bottom = bounds.top + 5;
				bounds.left = (grip->bounds.left + grip->bounds.right) / 2 - 4;
				bounds.right = bounds.left + 8;
			}

			OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, 
					scrollbar->orientation ? scrollbarNotchesHorizontal.region : scrollbarNotchesVertical.region,
					scrollbar->orientation ? scrollbarNotchesHorizontal.border : scrollbarNotchesVertical.border,
					OS_DRAW_MODE_REPEAT_FIRST, 0xFF, CLIP_RECTANGLE);
		}
	}

	return OSForwardMessage(object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
}

static OSCallbackResponse ProcessScrollbarMessage(OSObject object, OSMessage *message) {
	Scrollbar *grid = (Scrollbar *) object;
	OSCallbackResponse result = OS_CALLBACK_NOT_HANDLED;

	switch (message->type) {
		case OS_MESSAGE_MEASURE: {
			message->measure.preferredWidth = grid->preferredWidth;
			message->measure.preferredHeight = grid->preferredHeight;
			message->measure.minimumWidth = grid->minimumWidth;
			message->measure.minimumHeight = grid->minimumHeight;
			result = OS_CALLBACK_HANDLED;
		} break;

		case OS_MESSAGE_LAYOUT: {
			if (grid->relayout || message->layout.force) {
				grid->relayout = false;

				grid->bounds = OS_MAKE_RECTANGLE(
						message->layout.left, message->layout.right,
						message->layout.top, message->layout.bottom);

				StandardCellLayout(grid);

				if (grid->enabled) {
					OSSetScrollbarMeasurements(grid, grid->contentSize, grid->viewportSize);
				}

				grid->repaint = true;
				SetParentDescendentInvalidationFlags(grid, DESCENDENT_REPAINT);

				{
					OSMessage message;
					message.type = OS_MESSAGE_LAYOUT;
					message.layout.force = true;

					if (grid->orientation) {
						message.layout.left = grid->bounds.left;
						message.layout.right = grid->bounds.right;

						message.layout.top = grid->bounds.top;
						message.layout.bottom = message.layout.top + SCROLLBAR_SIZE;
						OSSendMessage(grid->objects[0], &message);

						if (!grid->enabled) {
							message.layout.top = 0;
							message.layout.bottom = 0;
							OSSendMessage(grid->objects[1], &message);
							OSSendMessage(grid->objects[3], &message);

							message.layout.top = grid->bounds.top + SCROLLBAR_SIZE;
							message.layout.bottom = grid->bounds.bottom - SCROLLBAR_SIZE;
							OSSendMessage(grid->objects[4], &message);
						} else {
							int x = grid->bounds.top + SCROLLBAR_SIZE + grid->position + grid->size;
							message.layout.top = grid->bounds.top + SCROLLBAR_SIZE + grid->position;
							message.layout.bottom = x;
							OSSendMessage(grid->objects[1], &message);
							message.layout.bottom = message.layout.top;
							message.layout.top = grid->bounds.top + SCROLLBAR_SIZE;
							OSSendMessage(grid->objects[3], &message);
							message.layout.top = x;
							message.layout.bottom = grid->bounds.bottom - SCROLLBAR_SIZE;
							OSSendMessage(grid->objects[4], &message);
						}

						message.layout.bottom = grid->bounds.bottom;
						message.layout.top = message.layout.bottom - SCROLLBAR_SIZE;
						OSSendMessage(grid->objects[2], &message);
					} else {
						message.layout.top = grid->bounds.top;
						message.layout.bottom = grid->bounds.bottom;

						message.layout.left = grid->bounds.left;
						message.layout.right = message.layout.left + SCROLLBAR_SIZE;
						OSSendMessage(grid->objects[0], &message);

						if (!grid->enabled) {
							message.layout.left = 0;
							message.layout.right = 0;
							OSSendMessage(grid->objects[1], &message);
							OSSendMessage(grid->objects[3], &message);

							message.layout.left = grid->bounds.left + SCROLLBAR_SIZE;
							message.layout.right = grid->bounds.right - SCROLLBAR_SIZE;
							OSSendMessage(grid->objects[4], &message);
						} else {
							int x = grid->bounds.left + SCROLLBAR_SIZE + grid->position + grid->size;
							message.layout.left = grid->bounds.left + SCROLLBAR_SIZE + grid->position;
							message.layout.right = x;
							OSSendMessage(grid->objects[1], &message);
							message.layout.right = message.layout.left;
							message.layout.left = grid->bounds.left + SCROLLBAR_SIZE;
							OSSendMessage(grid->objects[3], &message);
							message.layout.left = x;
							message.layout.right = grid->bounds.right - SCROLLBAR_SIZE;
							OSSendMessage(grid->objects[4], &message);
						}

						message.layout.right = grid->bounds.right;
						message.layout.left = message.layout.right - SCROLLBAR_SIZE;
						OSSendMessage(grid->objects[2], &message);
					}

				}

				result = OS_CALLBACK_HANDLED;
			}
		} break;

		default: {} break;
	}

	if (result == OS_CALLBACK_NOT_HANDLED) {
		result = OSForwardMessage(object, OS_MAKE_CALLBACK(ProcessGridMessage, nullptr), message);
	}

	return result;
}

void OSSetScrollbarMeasurements(OSObject _scrollbar, int contentSize, int viewportSize) {
	Scrollbar *scrollbar = (Scrollbar *) _scrollbar;

	// OSPrint("Set scrollbar %x to %dpx in %dpx viewport\n", scrollbar, contentSize, viewportSize);

	if (contentSize <= viewportSize) {
		scrollbar->enabled = false;
		scrollbar->height = 0;

		OSDisableControl(scrollbar->objects[0], true);
		OSDisableControl(scrollbar->objects[2], true);
		OSDisableControl(scrollbar->objects[3], true);
		OSDisableControl(scrollbar->objects[4], true);
	} else {
		int height = scrollbar->orientation ? (scrollbar->bounds.bottom - scrollbar->bounds.top) : (scrollbar->bounds.right - scrollbar->bounds.left);
		height -= SCROLLBAR_SIZE * 2;

		float fraction = -1;

		if (height != scrollbar->height && scrollbar->height > 0) {
			fraction = (float) scrollbar->position / (float) scrollbar->maxPosition;
		}

		scrollbar->enabled = true;
		scrollbar->height = height;
		scrollbar->contentSize = contentSize;
		scrollbar->viewportSize = viewportSize;

		float screens = (float) scrollbar->contentSize / (float) scrollbar->viewportSize;
		scrollbar->size = height / screens;

		scrollbar->maxPosition = height - scrollbar->size;
		if (fraction != -1) scrollbar->position = fraction * scrollbar->maxPosition;

		if (scrollbar->position < 0) scrollbar->position = 0;
		else if (scrollbar->position >= scrollbar->maxPosition) scrollbar->position = scrollbar->maxPosition;

		OSEnableControl(scrollbar->objects[0], true);
		OSEnableControl(scrollbar->objects[2], true);
		OSEnableControl(scrollbar->objects[3], true);
		OSEnableControl(scrollbar->objects[4], true);
	}

	{
		OSMessage message;
		message.type = OS_MESSAGE_CHILD_UPDATED;
		OSSendMessage(scrollbar, &message);
	}

	OSRepaintControl(scrollbar->objects[0]);
	OSRepaintControl(scrollbar->objects[1]);
	OSRepaintControl(scrollbar->objects[2]);
	OSRepaintControl(scrollbar->objects[3]);
	OSRepaintControl(scrollbar->objects[4]);

	ScrollbarPositionChanged(scrollbar);
}

OSObject OSCreateScrollbar(bool orientation) {
	uint8_t *memory = (uint8_t *) OSHeapAllocate(sizeof(Scrollbar) + sizeof(OSObject) * 5, true);

	Scrollbar *scrollbar = (Scrollbar *) memory;
	scrollbar->type = API_OBJECT_GRID;
	OSSetCallback(scrollbar, OS_MAKE_CALLBACK(ProcessScrollbarMessage, nullptr));

	scrollbar->orientation = orientation;

	scrollbar->columns = 1;
	scrollbar->rows = 5;
	scrollbar->objects = (GUIObject **) (memory + sizeof(Scrollbar));
	scrollbar->preferredWidth = !orientation ? DIMENSION_PUSH : SCROLLBAR_SIZE;
	scrollbar->preferredHeight = !orientation ? SCROLLBAR_SIZE : DIMENSION_PUSH;
	scrollbar->minimumWidth = !orientation ? SCROLLBAR_SIZE * 2 + 4 : SCROLLBAR_SIZE;
	scrollbar->minimumHeight = !orientation ? SCROLLBAR_SIZE : SCROLLBAR_SIZE * 2 + 4;

	OSCommand command = {};
	command.defaultDisabled = true;
	command.identifier = OS_COMMAND_DYNAMIC;

	Control *nudgeUp = (Control *) OSHeapAllocate(sizeof(Control), true);
	nudgeUp->type = API_OBJECT_CONTROL;
	nudgeUp->context = scrollbar;
	nudgeUp->notificationCallback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_NUDGE_UP);
	nudgeUp->backgrounds = orientation ? scrollbarTrackVerticalBackgrounds : scrollbarTrackHorizontalBackgrounds;
	OSSetCallback(nudgeUp, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr));

	command.callback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_NUDGE_DOWN);
	Control *nudgeDown = (Control *) OSHeapAllocate(sizeof(Control), true);
	nudgeDown->type = API_OBJECT_CONTROL;
	nudgeDown->context = scrollbar;
	nudgeDown->notificationCallback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_NUDGE_DOWN);
	nudgeDown->backgrounds = orientation ? scrollbarTrackVerticalBackgrounds : scrollbarTrackHorizontalBackgrounds;
	OSSetCallback(nudgeDown, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr));

	command.callback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_BUTTON_UP);
	Control *up = (Control *) OSCreateButton(&command);
	up->backgrounds = scrollbarButtonHorizontalBackgrounds;
	up->context = scrollbar;
	up->icons = orientation ? smallArrowUpIcons : smallArrowLeftIcons;
	up->centerIcons = true;

	Control *grip = (Control *) OSHeapAllocate(sizeof(Control), true);
	grip->type = API_OBJECT_CONTROL;
	grip->context = scrollbar;
	grip->backgrounds = orientation ? scrollbarButtonVerticalBackgrounds : scrollbarButtonHorizontalBackgrounds;
	OSSetCallback(grip, OS_MAKE_CALLBACK(ProcessScrollbarGripMessage, nullptr));

	command.callback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_BUTTON_DOWN);
	Control *down = (Control *) OSCreateButton(&command);
	down->backgrounds = scrollbarButtonHorizontalBackgrounds;
	down->context = scrollbar;
	down->icons = orientation ? smallArrowDownIcons : smallArrowRightIcons;
	down->centerIcons = true;

	OSAddControl(scrollbar, 0, 0, up, OS_FLAGS_DEFAULT);
	OSAddControl(scrollbar, 0, 1, grip, OS_CELL_V_EXPAND | OS_CELL_H_EXPAND);
	OSAddControl(scrollbar, 0, 2, down, OS_FLAGS_DEFAULT);
	OSAddControl(scrollbar, 0, 3, nudgeUp, OS_CELL_V_EXPAND | OS_CELL_H_EXPAND);
	OSAddControl(scrollbar, 0, 4, nudgeDown, OS_CELL_V_EXPAND | OS_CELL_H_EXPAND);

	scrollbar->enabled = false;

	return scrollbar;
}

void OSDisableControl(OSObject _control, bool disabled) {
	Control *control = (Control *) _control;
	if (control->disabled == disabled) return;
	control->disabled = disabled;

	OSAnimateControl(control, false);

	if (control->window->focus == control) {
		OSMessage message;
		message.type = OS_MESSAGE_END_FOCUS;
		OSSendMessage(control, &message);
		control->window->focus = nullptr;
	}

	if (control->window->lastFocus == control) {
		OSMessage message;
		message.type = OS_MESSAGE_END_LAST_FOCUS;
		OSSendMessage(control, &message);
		control->window->lastFocus = nullptr;
	}
}

void OSSetObjectNotificationCallback(OSObject _object, OSCallback callback) {
	APIObject *object = (APIObject *) _object;

	if (object->type == API_OBJECT_CONTROL) {
		Control *control = (Control *) _object;
		control->notificationCallback = callback;
	} else if (object->type == API_OBJECT_GRID) {
		Grid *grid = (Grid *) _object;
		grid->notificationCallback = callback;
	} else {
		OSCrashProcess(OS_FATAL_ERROR_BAD_OBJECT_TYPE);
	}
}

void OSSetCommandNotificationCallback(OSObject _window, OSCommand *_command, OSCallback callback) {
	Window *window = (Window *) _window;
	CommandWindow *command = window->commands + _command->identifier;
	command->notificationCallback = callback;

	LinkedItem<Control> *item = command->controls.firstItem;

	while (item) {
		Control *control = item->thisItem;
		control->notificationCallback = callback;
		item = item->nextItem;
	}
}

void OSDisableCommand(OSObject _window, OSCommand *_command, bool disabled) {
	Window *window = (Window *) _window;
	CommandWindow *command = window->commands + _command->identifier;
	if (command->disabled == disabled) return;
	command->disabled = disabled;

	LinkedItem<Control> *item = command->controls.firstItem;

	while (item) {
		Control *control = item->thisItem;
		OSDisableControl(control, disabled);
		item = item->nextItem;
	}
}

static inline int DistanceSquared(int x, int y) {
	return x * x + y * y;
}

static OSCallbackResponse ProcessWindowMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Window *window = (Window *) _object;

	static int lastClickX = 0, lastClickY = 0;
	static bool draggingStarted = false;
	bool updateWindow = false;
	bool rightClick = false;

	if (window->destroyed && message->type != OS_MESSAGE_WINDOW_DESTROYED) {
		return OS_CALLBACK_NOT_HANDLED;
	}

	switch (message->type) {
		case OS_MESSAGE_WINDOW_CREATED: {
			window->created = true;
			updateWindow = true;
		} break;

		case OS_MESSAGE_WINDOW_ACTIVATED: {
			if (!window->created) break;

			for (int i = 0; i < 12; i++) {
				if (i == 7 || (window->flags & OS_CREATE_WINDOW_MENU)) continue;
				OSDisableControl(window->root->objects[i], false);
			}

			if (window->lastFocus) {
				OSMessage message;
				message.type = OS_MESSAGE_CLIPBOARD_UPDATED;
				OSSyscall(OS_SYSCALL_GET_CLIPBOARD_HEADER, 0, (uintptr_t) &message.clipboard, 0, 0);
				OSSendMessage(window->lastFocus, &message);
			}
		} break;

		case OS_MESSAGE_WINDOW_DEACTIVATED: {
			if (!window->created) break;

			for (int i = 0; i < 12; i++) {
				if (i == 7 || (window->flags & OS_CREATE_WINDOW_MENU)) continue;
				OSDisableControl(window->root->objects[i], true);
			}

			if (window->focus) {
				OSMessage message;
				message.type = OS_MESSAGE_END_FOCUS;
				OSSendMessage(window->focus, &message);
				window->focus = nullptr;
			}

			if (window->flags & OS_CREATE_WINDOW_MENU) {
				message->type = OS_MESSAGE_DESTROY;
				OSSendMessage(window, message);
			}
		} break;

		case OS_MESSAGE_WINDOW_DESTROYED: {
			if (!window->hasParent) {
				OSHeapFree(window->commands);
			}

			OSHeapFree(window);
			return response;
		} break;

		case OS_MESSAGE_KEY_PRESSED: {
			if (message->keyboard.scancode == OS_SCANCODE_F4 && message->keyboard.alt) {
				message->type = OS_MESSAGE_DESTROY;
				OSSendMessage(window, message);
			} else if (window->focus) {
				message->type = OS_MESSAGE_KEY_TYPED;
				OSSendMessage(window->focus, message);
			}
		} break;

		case OS_MESSAGE_DESTROY: {
			for (uintptr_t i = 0; i < openMenuCount; i++) {
				if (openMenus[i].window == window) {
					if (i + 1 != openMenuCount) OSSendMessage(openMenus[i + 1].window, message);
					
					if (openMenus[i].source) {
						OSAnimateControl(openMenus[i].source, true);
					}

					openMenuCount = i;
					break;
				}
			}

			OSSendMessage(window->root, message);
			OSCloseHandle(window->surface);
			OSCloseHandle(window->window);
			window->destroyed = true;
		} break;

		case OS_MESSAGE_MOUSE_RIGHT_PRESSED:
			rightClick = true;
			// Fallthrough
		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			bool allowFocus = false;

			// If there is a control we're hovering over that isn't disabled,
			// and this either isn't an window activation click or the control is fine with activation clicks:
			if (window->hover && !window->hover->disabled
					&& (!message->mousePressed.activationClick || !window->hover->ignoreActivationClicks)) {
				// Send the raw WM message to the control.
				OSSendMessage(window->hover, message);

				// This control can get the focus from this message.
				allowFocus = true;

				// If this was a left click then press the control.
				if (!rightClick) {
					window->pressed = window->hover;

					draggingStarted = false;
					lastClickX = message->mousePressed.positionX;
					lastClickY = message->mousePressed.positionY;

					OSMessage m = *message;
					m.type = OS_MESSAGE_START_PRESS;
					OSSendMessage(window->pressed, &m);
				}
			}

			OSMessage message;

			// If a different control had focus, it must lose it.
			if (window->focus != window->hover) {
				message.type = OS_MESSAGE_END_FOCUS;
				OSSendMessage(window->focus, &message);
				window->focus = nullptr;
			}

			// If this control should be given focus...
			if (allowFocus) {
				Control *control = window->hover;

				// And it is focusable...
				if (control->focusable) {
					// Remove previous any last focus.
					if (window->lastFocus && control != window->lastFocus) {
						message.type = OS_MESSAGE_END_LAST_FOCUS;
						OSSendMessage(window->lastFocus, &message);
					}

					// And give this control focus, if it doesn't already have it.
					if (control != window->focus) {
						window->focus = control;
						window->lastFocus = control;
						message.type = OS_MESSAGE_START_FOCUS;
						OSSendMessage(control, &message);
						OSSyscall(OS_SYSCALL_RESET_CLICK_CHAIN, 0, 0, 0, 0);
					}
				}
			}
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			if (window->pressed) {
				// Send the raw message.
				OSSendMessage(window->hover, message);

				OSRepaintControl(window->pressed);

				if (window->pressed == window->hover) {
					OSMessage clicked;
					clicked.type = OS_MESSAGE_CLICKED;
					OSSendMessage(window->pressed, &clicked);
				}

				OSMessage message;
				message.type = OS_MESSAGE_END_PRESS;
				OSSendMessage(window->pressed, &message);
				window->pressed = nullptr;
			}

			OSMessage m = *message;
			m.type = OS_MESSAGE_MOUSE_MOVED;
			m.mouseMoved.oldPositionX = message->mousePressed.positionX;
			m.mouseMoved.oldPositionY = message->mousePressed.positionY;
			m.mouseMoved.newPositionX = message->mousePressed.positionX;
			m.mouseMoved.newPositionY = message->mousePressed.positionY;
			m.mouseMoved.newPositionXScreen = message->mousePressed.positionXScreen;
			m.mouseMoved.newPositionYScreen = message->mousePressed.positionYScreen;
			OSSendMessage(window->root, &m);
		} break;

		case OS_MESSAGE_MOUSE_EXIT: {
			if (window->hover) {
				OSMessage message;
				message.type = OS_MESSAGE_END_HOVER;
				OSSendMessage(window->hover, &message);
				window->hover = nullptr;
			}
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (window->pressed) {
				OSMessage hitTest;
				hitTest.type = OS_MESSAGE_HIT_TEST;
				hitTest.hitTest.positionX = message->mouseMoved.newPositionX;
				hitTest.hitTest.positionY = message->mouseMoved.newPositionY;
				OSCallbackResponse response = OSSendMessage(window->pressed, &hitTest);

				if (response == OS_CALLBACK_HANDLED && !hitTest.hitTest.result && window->hover) {
					OSMessage message;
					message.type = OS_MESSAGE_END_HOVER;
					OSSendMessage(window->hover, &message);
					window->hover = nullptr;
				} else if (response == OS_CALLBACK_HANDLED && hitTest.hitTest.result && !window->hover) {
					OSMessage message;
					message.type = OS_MESSAGE_START_HOVER;
					window->hover = window->pressed;
					OSSendMessage(window->hover, &message);
				}

				if (draggingStarted || DistanceSquared(message->mouseMoved.newPositionX, message->mouseMoved.newPositionY) >= 4) {
					message->mouseDragged.originalPositionX = lastClickX;
					message->mouseDragged.originalPositionY = lastClickY;

					if (!draggingStarted) {
						draggingStarted = true;
						message->type = OS_MESSAGE_START_DRAG;
						OSSendMessage(window->pressed, message);
					}

					message->type = OS_MESSAGE_MOUSE_DRAGGED;
					OSSendMessage(window->pressed, message);
				}
			} else {
				Control *old = window->hover;

				OSMessage hitTest;
				hitTest.type = OS_MESSAGE_HIT_TEST;
				hitTest.hitTest.positionX = message->mouseMoved.newPositionX;
				hitTest.hitTest.positionY = message->mouseMoved.newPositionY;
				OSCallbackResponse response = OSSendMessage(old, &hitTest);

				if (!hitTest.hitTest.result || response == OS_CALLBACK_NOT_HANDLED) {
					window->hover = nullptr;
					OSSendMessage(window->root, message);
				} else {
					OSSendMessage(old, message);
				}

				if (!window->hover) {
					window->cursor = OS_CURSOR_NORMAL;
				}

				if (window->hover != old && old) {
					OSMessage message;
					message.type = OS_MESSAGE_END_HOVER;
					OSSendMessage(old, &message);
				}
			}
		} break;

		case OS_MESSAGE_WM_TIMER: {
			LinkedItem<Control> *item = window->timerControls.firstItem;

			while (item) {
				int ticksPerMessage = window->timerHz / item->thisItem->timerHz;
				item->thisItem->timerStep++;

				if (item->thisItem->timerStep >= ticksPerMessage) {
					item->thisItem->timerStep = 0;
					OSSendMessage(item->thisItem, message);
				}

				item = item->nextItem;
			}

			if (window->lastFocus) {
				window->caretBlinkStep++;

				if (window->caretBlinkStep >= window->timerHz / CARET_BLINK_HZ) {
					window->caretBlinkStep = 0;

					if (window->caretBlinkPause) {
						window->caretBlinkPause--;
					} else {
						OSMessage message;
						message.type = OS_MESSAGE_CARET_BLINK;
						OSSendMessage(window->lastFocus, &message);
					}
				}
			}
		} break;

		case OS_MESSAGE_CLIPBOARD_UPDATED: {
			if (window->lastFocus) {
				OSSendMessage(window->lastFocus, message);
			}
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	if (window->destroyed) {
		return response;
	}

	{
		// TODO Only do this if the list or focus has changed.

		LinkedItem<Control> *item = window->timerControls.firstItem;
		int largestHz = window->focus ? CARET_BLINK_HZ : 0;

		while (item) {
			int thisHz = item->thisItem->timerHz;
			if (thisHz > largestHz) largestHz = thisHz;
			item = item->nextItem;
		}

		if (window->timerHz != largestHz) {
			window->timerHz = largestHz;
			OSSyscall(OS_SYSCALL_NEED_WM_TIMER, window->window, largestHz, 0, 0);
		}
	}

	if (window->descendentInvalidationFlags & DESCENDENT_RELAYOUT) {
		window->descendentInvalidationFlags &= ~DESCENDENT_RELAYOUT;
		clipStack[0] = OS_MAKE_RECTANGLE(0, window->width, 0, window->height);

		OSMessage message;
		message.type = OS_MESSAGE_LAYOUT;
		message.layout.left = 0;
		message.layout.top = 0;
		message.layout.right = window->width;
		message.layout.bottom = window->height;
		message.layout.force = false;
		OSSendMessage(window->root, &message);
	}

	if (window->cursor != window->cursorOld) {
		OSSyscall(OS_SYSCALL_SET_CURSOR_STYLE, window->window, window->cursor, 0, 0);
		window->cursorOld = window->cursor;
		updateWindow = true;
	}

	if (window->descendentInvalidationFlags & DESCENDENT_REPAINT) {
		window->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;
		clipStack[0] = OS_MAKE_RECTANGLE(0, window->width, 0, window->height);

		OSMessage message;
		message.type = OS_MESSAGE_PAINT;
		message.paint.surface = window->surface;
		message.paint.force = false;
		OSSendMessage(window->root, &message);
		updateWindow = true;
	}

	if (updateWindow) {
		OSSyscall(OS_SYSCALL_UPDATE_WINDOW, window->window, 0, 0, 0);
	}

	return response;
}

static Window *CreateWindow(OSWindowSpecification *specification, Window *parent, unsigned x = 0, unsigned y = 0) {
	unsigned flags = specification->flags;

	if (!flags) {
		flags = OS_CREATE_WINDOW_NORMAL;
	}

	if (specification->menubar) {
		flags |= OS_CREATE_WINDOW_WITH_MENUBAR;
	}

	if (!(flags & OS_CREATE_WINDOW_MENU)) {
		specification->width += totalBorderWidth;
		specification->minimumWidth += totalBorderWidth;
		specification->height += totalBorderHeight;
		specification->minimumHeight += totalBorderHeight;
	}

	Window *window = (Window *) OSHeapAllocate(sizeof(Window), true);
	window->type = API_OBJECT_WINDOW;

	OSRectangle bounds;
	bounds.left = x;
	bounds.right = x + specification->width;
	bounds.top = y;
	bounds.bottom = y + specification->height;

	OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) &window->window, (uintptr_t) &bounds, (uintptr_t) window, parent ? (uintptr_t) parent->window : 0);
	OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, window->window, (uintptr_t) &bounds, 0, 0);

	window->width = bounds.right - bounds.left;
	window->height = bounds.bottom - bounds.top;
	window->flags = flags;
	window->cursor = OS_CURSOR_NORMAL;
	window->minimumWidth = specification->minimumWidth;
	window->minimumHeight = specification->minimumHeight;

	if (!parent) {
		window->commands = (CommandWindow *) OSHeapAllocate(sizeof(CommandWindow) * _commandCount, true);

		for (uintptr_t i = 0; i < _commandCount; i++) {
			CommandWindow *command = window->commands + i;
			command->disabled = _commands[i]->defaultDisabled;
			command->checked = _commands[i]->defaultCheck;
			command->notificationCallback = _commands[i]->callback;
		}
	} else {
		window->commands = parent->commands;
		window->hasParent = true;
	}

	OSSetCallback(window, OS_MAKE_CALLBACK(ProcessWindowMessage, nullptr));

	window->root = (Grid *) OSCreateGrid(3, 4, OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_GAP);
	window->root->parent = window;

	{
		OSMessage message;
		message.parentUpdated.window = window;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(window->root, &message);
	}

	if (flags & OS_CREATE_WINDOW_NORMAL) {
		OSObject titlebar = CreateWindowResizeHandle(windowBorder22, RESIZE_MOVE);
		OSAddControl(window->root, 1, 1, titlebar, OS_CELL_H_PUSH | OS_CELL_H_EXPAND | OS_CELL_V_EXPAND);
		OSSetText(titlebar, specification->title, specification->titleBytes);

		OSAddControl(window->root, 0, 0, CreateWindowResizeHandle(windowBorder11, RESIZE_TOP_LEFT), 0);
		OSAddControl(window->root, 1, 0, CreateWindowResizeHandle(windowBorder12, RESIZE_TOP), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSAddControl(window->root, 2, 0, CreateWindowResizeHandle(windowBorder13, RESIZE_TOP_RIGHT), 0);
		OSAddControl(window->root, 0, 1, CreateWindowResizeHandle(windowBorder21, RESIZE_LEFT), 0);
		OSAddControl(window->root, 2, 1, CreateWindowResizeHandle(windowBorder23, RESIZE_RIGHT), 0);
		OSAddControl(window->root, 0, 2, CreateWindowResizeHandle(windowBorder31, RESIZE_LEFT), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
		OSAddControl(window->root, 2, 2, CreateWindowResizeHandle(windowBorder33, RESIZE_RIGHT), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
		OSAddControl(window->root, 0, 3, CreateWindowResizeHandle(windowBorder41, RESIZE_BOTTOM_LEFT), 0);
		OSAddControl(window->root, 1, 3, CreateWindowResizeHandle(windowBorder42, RESIZE_BOTTOM), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSAddControl(window->root, 2, 3, CreateWindowResizeHandle(windowBorder43, RESIZE_BOTTOM_RIGHT), 0);

		if (flags & OS_CREATE_WINDOW_WITH_MENUBAR) {
			OSObject grid = OSCreateGrid(1, 2, OS_CREATE_GRID_NO_GAP | OS_CREATE_GRID_NO_BORDER | OS_CREATE_GRID_NO_BACKGROUND);
			OSAddGrid(window->root, 1, 2, grid, OS_CELL_FILL);
			OSAddGrid(grid, 0, 0, OSCreateMenu(specification->menubar, nullptr, OS_MAKE_POINT(0, 0), OS_CREATE_MENUBAR), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		}
	}

	return window;
}

OSObject OSCreateWindow(OSWindowSpecification *specification) {
	return CreateWindow(specification, nullptr);
}

OSObject OSCreateMenu(OSMenuSpecification *menuSpecification, OSObject _source, OSPoint position, unsigned flags) {
	Control *source = (Control *) _source;

	size_t itemCount = menuSpecification->itemCount;
	bool menubar = flags & OS_CREATE_MENUBAR;

	Control *items[itemCount];
	int width = 0, height = 8;

	for (uintptr_t i = 0; i < itemCount; i++) {
		switch (menuSpecification->items[i].type) {
			case OSMenuItem::SEPARATOR: {
				items[i] = (Control *) OSCreateLine(menubar ? OS_ORIENTATION_VERTICAL : OS_ORIENTATION_HORIZONTAL);
			} break;

			case OSMenuItem::SUBMENU:
			case OSMenuItem::COMMAND: {
				items[i] = (Control *) CreateMenuItem(menuSpecification->items[i], menubar);
			} break;

			default: {} continue;
		}

		if (menubar) {
			if (items[i]->preferredHeight > height) {
				height = items[i]->preferredHeight;
			}

			width += items[i]->preferredWidth;
		} else {
			if (items[i]->preferredWidth > width) {
				width = items[i]->preferredWidth;
			}

			height += items[i]->preferredHeight;
		}
	}

	if (!menubar) {
		if (width < 100) width = 100;
		width += 8;
	}


	OSObject grid = OSCreateGrid(menubar ? itemCount : 1, !menubar ? itemCount : 1, OS_CREATE_GRID_MENU | OS_CREATE_GRID_NO_GAP | (menubar ? OS_CREATE_GRID_NO_BORDER : 0));
	((Grid *) grid)->background = menubar ? &menubarBackground : &menuBox;

	OSObject returnValue = grid;

	if (!menubar) {
		OSWindowSpecification specification = {};
		specification.width = width;
		specification.height = height;
		specification.flags = OS_CREATE_WINDOW_MENU;

		Window *window;
		int x = position.x;
		int y = position.y;

		if (x == -1 && y == -1 && source && x != -2) {
			if (flags & OS_CREATE_SUBMENU) {
				x = source->bounds.right;
				y = source->bounds.top;
			} else {
				x = source->bounds.left;
				y = source->bounds.bottom;
			}

			OSRectangle bounds;
			OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, source->window->window, (uintptr_t) &bounds, 0, 0);

			x += bounds.left;
			y += bounds.top;
		}

		if ((x == -1 && y == -1) || x == -2) {
			OSPoint position;
			OSGetMousePosition(nullptr, &position);

			x = position.x;
			y = position.y;
		}

		window = CreateWindow(&specification, source ? source->window : nullptr, x, y);

		for (uintptr_t i = 0; i < openMenuCount; i++) {
			if (openMenus[i].source->window == source->window) {
				OSMessage message;
				message.type = OS_MESSAGE_DESTROY;
				OSSendMessage(openMenus[i].window, &message);
				break;
			}
		}

		openMenus[openMenuCount].source = source;
		openMenus[openMenuCount].window = window;
		openMenuCount++;

		OSSetRootGrid(window, grid);
		returnValue = window;
	}

	for (uintptr_t i = 0; i < itemCount; i++) {
		OSAddControl(grid, menubar ? i : 0, !menubar ? i : 0, items[i], OS_CELL_H_EXPAND | OS_CELL_V_EXPAND);
	}

	return returnValue;
}

void OSGetMousePosition(OSObject relativeWindow, OSPoint *position) {
	OSSyscall(OS_SYSCALL_GET_CURSOR_POSITION, (uintptr_t) position, 0, 0, 0);

	if (relativeWindow) {
		OSRectangle bounds;
		OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, ((Window *) relativeWindow)->window, (uintptr_t) &bounds, 0, 0);

		position->x -= bounds.left;
		position->y -= bounds.top;
	}
}

void OSInitialiseGUI() {
}
