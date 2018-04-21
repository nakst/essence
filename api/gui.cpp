#include "../bin/OS/standard.manifest.h"

static void EnterDebugger() {
	asm volatile ("xchg %bx,%bx");
}

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
#define RESIZE_NONE		(11)

#define SCROLLBAR_SIZE (17)
#define SCROLLBAR_MINIMUM (16)
#define SCROLLBAR_BUTTON_AMOUNT (64)

#define STANDARD_BORDER_SIZE (2)

#define LIST_VIEW_MARGIN (10)
#define LIST_VIEW_TEXT_MARGIN (6)
#define LIST_VIEW_WITH_BORDER_MARGIN (LIST_VIEW_MARGIN + STANDARD_BORDER_SIZE)
#define LIST_VIEW_ROW_HEIGHT (21)
#define LIST_VIEW_HEADER_HEIGHT (25)

#define ICON_TEXT_GAP (5)

uint32_t STANDARD_BACKGROUND_COLOR = 0xF5F6F9;

uint32_t LIST_VIEW_COLUMN_TEXT_COLOR = 0x4D6278;
uint32_t LIST_VIEW_PRIMARY_TEXT_COLOR = 0x000000;
uint32_t LIST_VIEW_SECONDARY_TEXT_COLOR = 0x686868;
uint32_t LIST_VIEW_BACKGROUND_COLOR = 0xFFFFFFFF;

uint32_t TEXT_COLOR_DEFAULT = 0x000515;
uint32_t TEXT_COLOR_DISABLED = 0x777777;
uint32_t TEXT_COLOR_DISABLED_SHADOW = 0xEEEEEE;
uint32_t TEXT_COLOR_HEADING = 0x003296;
uint32_t TEXT_COLOR_TITLEBAR = 0xFFFFFF;
uint32_t TEXT_COLOR_TOOLBAR = 0xFFFFFF;

uint32_t TEXTBOX_SELECTED_COLOR_1 = 0xFFC4D9F9;
uint32_t TEXTBOX_SELECTED_COLOR_2 = 0xFFDDDDDD;

uint32_t DISABLE_TEXT_SHADOWS = 1;

// TODO Keyboard controls.
// 	- Enter and escape in dialog boxes
// 	- Escape in normal windows to restore default focus
// 	- Keyboard shortcuts and access keys
// 	- Menus and list view navigation
// TODO Scrollbar buttons are broken.
// TODO Send repeat messages for held left press? Scrollbar buttons, scrollbar nudges, scroll-selections.
// TODO Minor menu[bar] border adjustments; menu icons.
// TODO Calculator textbox - selection extends out of top of textbox
// TODO Memory "arenas".
// TODO Is the automatic scrollbar positioning correct?
// TODO Multiple-cell positions.
// TODO Wrapping.

struct UIImage {
	OSRectangle region;
	OSRectangle border;
	OSDrawMode drawMode;

	UIImage Translate(int x, int y) {
		UIImage a = *this;
		a.region.left += x;
		a.region.right += x;
		a.border.left += x;
		a.border.right += x;
		a.region.top += y;
		a.region.bottom += y;
		a.border.top += y;
		a.border.bottom += y;
		return a;
	}
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

static UIImage activeDialogBorder11	= {{1, 1 + 3, 144, 144 + 3}, 	{1, 1, 144, 144}};
static UIImage activeDialogBorder12	= {{8, 8 + 1, 144, 144 + 3}, 	{8, 9, 144, 144}};
static UIImage activeDialogBorder13	= {{13, 13 + 3, 144, 144 + 3}, 	{13, 13, 144, 144}};
static UIImage activeDialogBorder21	= {{1, 1 + 3, 151, 151 + 24}, 	{1, 1, 151, 151}};
static UIImage activeDialogBorder23	= {{13, 13 + 3, 151, 151 + 24},	{13, 13, 151, 151}};
static UIImage activeDialogBorder31	= {{1, 1 + 3, 185, 185 + 1}, 	{1, 1, 185, 186}};
static UIImage activeDialogBorder33	= {{1, 1 + 3, 187, 187 + 1}, 	{1, 1, 187, 188}};
static UIImage activeDialogBorder41	= {{1, 1 + 3, 181, 181 + 3}, 	{1, 1, 181, 181}};
static UIImage activeDialogBorder42	= {{5, 5 + 1, 185, 185 + 3}, 	{5, 6, 185, 185}};
static UIImage activeDialogBorder43	= {{13, 13 + 3, 181, 181 + 3}, 	{13, 13, 181, 181}};

static UIImage inactiveDialogBorder11	= {{16 + 1, 16 + 1 + 3, 144, 144 + 3}, 	{16 + 1, 16 + 1, 144, 144}};
static UIImage inactiveDialogBorder12	= {{16 + 8, 16 + 8 + 1, 144, 144 + 3}, 	{16 + 8, 16 + 9, 144, 144}};
static UIImage inactiveDialogBorder13	= {{16 + 10 + 3, 16 + 10 + 3 + 3, 144, 144 + 3},{16 + 10 + 3, 16 + 10 + 3, 144, 144}};
static UIImage inactiveDialogBorder21	= {{16 + 1, 16 + 1 + 3, 151, 151 + 24}, {16 + 1, 16 + 1, 151, 151}};
static UIImage inactiveDialogBorder23	= {{16 + 10 + 3, 16 + 10 + 3 + 3, 151, 151 + 24},{16 + 10 + 3, 16 + 10 + 3, 151, 151}};
static UIImage inactiveDialogBorder31	= {{1, 1 + 3, 189, 189 + 1}, 	{1, 1, 189, 190}};
static UIImage inactiveDialogBorder33	= {{1, 1 + 3, 191, 191 + 1}, {1, 1, 191, 192}};
static UIImage inactiveDialogBorder41	= {{16 + 1, 16 + 1 + 3, 181, 181 + 3}, 	{16 + 1, 16 + 1, 181, 181}};
static UIImage inactiveDialogBorder42	= {{5, 5 + 1, 189, 189 + 3}, 	{5, 5 + 1, 189, 189}};
static UIImage inactiveDialogBorder43	= {{16 + 10 + 3, 16 + 10 + 3 + 3, 181, 181 + 3}, {16 + 10 + 3, 16 + 10 + 3, 181, 181}};

static UIImage progressBarBackground 	= {{1, 8, 122, 143}, {3, 6, 125, 139}};
static UIImage progressBarDisabled   	= {{9, 16, 122, 143}, {11, 14, 125, 139}};
static UIImage progressBarPellet     	= {{18, 26, 128, 143}, {18, 18, 128, 128}};

static UIImage buttonNormal		= {{0 * 9 + 0, 0 * 9 + 8, 88, 109}, {0 * 9 + 3, 0 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };
static UIImage buttonDisabled		= {{1 * 9 + 0, 1 * 9 + 8, 88, 109}, {1 * 9 + 3, 1 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };
static UIImage buttonHover		= {{2 * 9 + 0, 2 * 9 + 8, 88, 109}, {2 * 9 + 3, 2 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };
static UIImage buttonPressed		= {{3 * 9 + 0, 3 * 9 + 8, 88, 109}, {3 * 9 + 3, 3 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };
static UIImage buttonFocused		= {{4 * 9 + 0, 4 * 9 + 8, 88, 109}, {4 * 9 + 3, 4 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };
static UIImage buttonDangerousHover	= {{5 * 9 + 0, 5 * 9 + 8, 88, 109}, {5 * 9 + 3, 5 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };
static UIImage buttonDangerousPressed	= {{6 * 9 + 0, 6 * 9 + 8, 88, 109}, {6 * 9 + 3, 6 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };
static UIImage buttonDangerousFocused	= {{7 * 9 + 0, 7 * 9 + 8, 88, 109}, {7 * 9 + 3, 7 * 9 + 5, 91, 106}, OS_DRAW_MODE_STRECH };

static UIImage checkboxHover		= {{99, 112, 242, 255}, {99, 100, 242, 243}};

static UIImage textboxNormal		= {{52, 61, 166, 189}, {55, 58, 169, 186}};
static UIImage textboxFocus		= {{11 + 52, 11 + 61, 166, 189}, {11 + 55, 11 + 58, 169, 186}};
static UIImage textboxHover		= {{-11 + 52, -11 + 61, 166, 189}, {-11 + 55, -11 + 58, 169, 186}};
static UIImage textboxDisabled		= {{22 + 52, 22 + 61, 166, 189}, {22 + 55, 22 + 58, 169, 186}};
static UIImage textboxCommand		= {{33 + 52, 33 + 61, 166, 189}, {33 + 55, 33 + 58, 169, 186}};
static UIImage textboxCommandHover	= {{44 + 52, 44 + 61, 166, 189}, {44 + 55, 44 + 58, 169, 186}};

static UIImage gridBox 			= {{1, 7, 17, 23}, {3, 4, 19, 20}};
static UIImage menuBox 			= {{1, 32, 1, 15}, {28, 30, 4, 6}};
static UIImage menubarBackground	= {{34, 40, 124, 145}, {35, 38, 128, 144}, OS_DRAW_MODE_STRECH};
static UIImage dialogAltAreaBox		= {{18, 19, 17, 22}, {18, 19, 21, 22}};

static UIImage menuItemHover		= {{42, 50, 142, 159}, {45, 46, 151, 157}, OS_DRAW_MODE_STRECH};
static UIImage menuItemDragged		= {{18 + 42, 18 + 50, 142, 159}, {18 + 45, 18 + 46, 151, 157}, OS_DRAW_MODE_STRECH};

static UIImage toolbarBackground	= {{0, 0 + 60, 195, 195 + 31}, {0 + 1, 0 + 59, 195 + 1, 195 + 29}, OS_DRAW_MODE_STRECH};
static UIImage toolbarBackgroundAlt	= {{98 + 0, 98 + 0 + 60, 195, 195 + 31}, {98 + 0 + 1, 98 + 0 + 59, 195 + 1, 195 + 29}, OS_DRAW_MODE_STRECH};

static UIImage toolbarHover		= {{73, 84, 195, 226}, {78, 79, 203, 204}};
static UIImage toolbarPressed		= {{73 - 12, 84 - 12, 195, 226}, {78 - 12, 79 - 12, 203, 204}};
static UIImage toolbarNormal		= {{73 + 12, 84 + 12, 195, 226}, {78 + 12, 79 + 12, 203, 204}};

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

static UIImage smallArrowUpNormal      = {{206, 217, 25, 34}, {206, 206, 25, 25}};
static UIImage smallArrowDownNormal    = {{206, 217, 35, 44}, {206, 206, 35, 35}};
static UIImage smallArrowLeftNormal    = {{204, 213, 14, 25}, {204, 204, 14, 14}};
static UIImage smallArrowRightNormal   = {{204, 213, 3, 14}, {204, 204, 3, 3}};

static UIImage listViewHighlight           = {{228, 241, 59, 72}, {228 + 3, 241 - 3, 59 + 3, 72 - 3}};
static UIImage listViewSelected            = {{14 + 228, 14 + 241, 59, 72}, {14 + 228 + 3, 14 + 241 - 3, 59 + 3, 72 - 3}};
static UIImage listViewSelected2           = {{14 + 228, 14 + 241, 28 + 59, 28 + 72}, {14 + 228 + 3, 14 + 241 - 3, 28 + 59 + 3, 28 + 72 - 3}};
static UIImage listViewLastClicked         = {{14 + 228, 14 + 241, 59 - 14, 72 - 14}, {14 + 228 + 6, 14 + 228 + 7, 59 + 6 - 14, 59 + 7 - 14}};
static UIImage listViewSelectionBox        = {{14 + 228 - 14, 14 + 231 - 14, 42 + 59 - 14, 42 + 62 - 14}, {14 + 228 + 1 - 14, 14 + 228 + 2 - 14, 42 + 59 + 1 - 14, 42 + 59 + 2 - 14}};
static UIImage listViewColumnHeaderDivider = {{239, 240, 87, 112}, {239, 239, 87, 88}};
static UIImage listViewColumnHeader        = {{233, 239, 87, 112}, {233, 239, 87, 88}};

static UIImage lineHorizontal		= {{40, 52, 115, 116}, {41, 42, 115, 115}};
static UIImage *lineHorizontalBackgrounds[] = { &lineHorizontal, &lineHorizontal, &lineHorizontal, &lineHorizontal, };
static UIImage lineVertical		= {{35, 36, 110, 122}, {35, 35, 111, 112}};
static UIImage *lineVerticalBackgrounds[] = { &lineVertical, &lineVertical, &lineVertical, &lineVertical, };

// static UIImage testImage = {{57, 61, 111, 115}, {58, 60, 112, 114}};

static struct UIImage *scrollbarTrackVerticalBackgrounds[] = { 
	&scrollbarTrackVerticalEnabled, 
	&scrollbarTrackVerticalDisabled, 
	&scrollbarTrackVerticalEnabled, 
	&scrollbarTrackVerticalPressed, 
	&scrollbarTrackVerticalEnabled, 
};
static struct UIImage *scrollbarTrackHorizontalBackgrounds[] = { 
	&scrollbarTrackHorizontalEnabled, 
	&scrollbarTrackHorizontalDisabled, 
	&scrollbarTrackHorizontalEnabled, 
	&scrollbarTrackHorizontalPressed, 
	&scrollbarTrackHorizontalEnabled, 
};

#define ICON16(x, y) {{x, x + 16, y, y + 16}, {x, x, y, y}}
#define ICON24(x, y) {{x, x + 24, y, y + 24}, {x, x, y, y}}
#define ICON32(x, y) {{x, x + 32, y, y + 32}, {x, x, y, y}}

static UIImage icons16[] = {
	{{}, {}},
	ICON16(237, 117),
	ICON16(220, 117),
	{{}, {}},
	ICON16(512 + 320, 208),
	ICON16(512 + 320, 160),
	ICON16(512 + 64, 192),
	ICON16(512 + 320, 288),
};

static UIImage icons32[] = {
	{{}, {}},
	{{}, {}},
	{{}, {}},
	ICON32(220, 135),
	{{}, {}},
	{{}, {}},
};

static UIImage *scrollbarButtonHorizontalBackgrounds[] = {
	&scrollbarButtonHorizontalNormal,
	&scrollbarButtonDisabled,
	&scrollbarButtonHorizontalHover,
	&scrollbarButtonHorizontalPressed,
	&scrollbarButtonVerticalNormal,
};

static UIImage *scrollbarButtonVerticalBackgrounds[] = {
	&scrollbarButtonVerticalNormal,
	&scrollbarButtonDisabled,
	&scrollbarButtonVerticalHover,
	&scrollbarButtonVerticalPressed,
	&scrollbarButtonVerticalNormal,
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
	&menuItemHover,
};

struct UIImage *toolbarItemBackgrounds[] = {
	nullptr,
	nullptr,
	&toolbarHover,
	&toolbarPressed,
	&toolbarNormal,
	nullptr,
	&toolbarHover,
	&toolbarPressed,
};

static UIImage *textboxBackgrounds[] = {
	&textboxNormal,
	&textboxDisabled,
	&textboxHover,
	&textboxFocus,
	&textboxFocus,
};

static UIImage *textboxCommandBackgrounds[] = {
	&textboxCommand,
	&textboxDisabled,
	&textboxCommandHover,
	&textboxFocus,
	&textboxFocus,
};

static UIImage *buttonBackgrounds[] = {
	&buttonNormal,
	&buttonDisabled,
	&buttonHover,
	&buttonPressed,
	&buttonFocused,
};

static UIImage *buttonDangerousBackgrounds[] = {
	&buttonNormal,
	&buttonDisabled,
	&buttonDangerousHover,
	&buttonDangerousPressed,
	&buttonDangerousFocused,
};

static UIImage *progressBarBackgrounds[] = {
	&progressBarBackground,
	&progressBarDisabled,
	&progressBarBackground,
	&progressBarBackground,
	&progressBarBackground,
};

static UIImage *windowBorder11[] = {&activeWindowBorder11, &inactiveWindowBorder11, &activeWindowBorder11, &activeWindowBorder11, &activeWindowBorder11};
static UIImage *windowBorder12[] = {&activeWindowBorder12, &inactiveWindowBorder12, &activeWindowBorder12, &activeWindowBorder12, &activeWindowBorder12};
static UIImage *windowBorder13[] = {&activeWindowBorder13, &inactiveWindowBorder13, &activeWindowBorder13, &activeWindowBorder13, &activeWindowBorder13};
static UIImage *windowBorder21[] = {&activeWindowBorder21, &inactiveWindowBorder21, &activeWindowBorder21, &activeWindowBorder21, &activeWindowBorder21};
static UIImage *windowBorder22[] = {&activeWindowBorder22, &inactiveWindowBorder22, &activeWindowBorder22, &activeWindowBorder22, &activeWindowBorder22};
static UIImage *windowBorder23[] = {&activeWindowBorder23, &inactiveWindowBorder23, &activeWindowBorder23, &activeWindowBorder23, &activeWindowBorder23};
static UIImage *windowBorder31[] = {&activeWindowBorder31, &inactiveWindowBorder31, &activeWindowBorder31, &activeWindowBorder31, &activeWindowBorder31};
static UIImage *windowBorder33[] = {&activeWindowBorder33, &inactiveWindowBorder33, &activeWindowBorder33, &activeWindowBorder33, &activeWindowBorder33};
static UIImage *windowBorder41[] = {&activeWindowBorder41, &inactiveWindowBorder41, &activeWindowBorder41, &activeWindowBorder41, &activeWindowBorder41};
static UIImage *windowBorder42[] = {&activeWindowBorder42, &inactiveWindowBorder42, &activeWindowBorder42, &activeWindowBorder42, &activeWindowBorder42};
static UIImage *windowBorder43[] = {&activeWindowBorder43, &inactiveWindowBorder43, &activeWindowBorder43, &activeWindowBorder43, &activeWindowBorder43};
                                                                                                                                
static UIImage *dialogBorder11[] = {&activeDialogBorder11, &inactiveDialogBorder11, &activeDialogBorder11, &activeDialogBorder11, &activeDialogBorder11};
static UIImage *dialogBorder12[] = {&activeDialogBorder12, &inactiveDialogBorder12, &activeDialogBorder12, &activeDialogBorder12, &activeDialogBorder12};
static UIImage *dialogBorder13[] = {&activeDialogBorder13, &inactiveDialogBorder13, &activeDialogBorder13, &activeDialogBorder13, &activeDialogBorder13};
static UIImage *dialogBorder21[] = {&activeDialogBorder21, &inactiveDialogBorder21, &activeDialogBorder21, &activeDialogBorder21, &activeDialogBorder21};
static UIImage *dialogBorder23[] = {&activeDialogBorder23, &inactiveDialogBorder23, &activeDialogBorder23, &activeDialogBorder23, &activeDialogBorder23};
static UIImage *dialogBorder31[] = {&activeDialogBorder31, &inactiveDialogBorder31, &activeDialogBorder31, &activeDialogBorder31, &activeDialogBorder31};
static UIImage *dialogBorder33[] = {&activeDialogBorder33, &inactiveDialogBorder33, &activeDialogBorder33, &activeDialogBorder33, &activeDialogBorder33};
static UIImage *dialogBorder41[] = {&activeDialogBorder41, &inactiveDialogBorder41, &activeDialogBorder41, &activeDialogBorder41, &activeDialogBorder41};
static UIImage *dialogBorder42[] = {&activeDialogBorder42, &inactiveDialogBorder42, &activeDialogBorder42, &activeDialogBorder42, &activeDialogBorder42};
static UIImage *dialogBorder43[] = {&activeDialogBorder43, &inactiveDialogBorder43, &activeDialogBorder43, &activeDialogBorder43, &activeDialogBorder43};

static const int totalBorderWidth = 6 + 6;
static const int totalBorderHeight = 6 + 24 + 6;

struct GUIObject : APIObject {
	OSRectangle bounds, cellBounds, inputBounds;
	uint16_t descendentInvalidationFlags;
	uint16_t layout;
	uint16_t preferredWidth, preferredHeight;
	uint16_t horizontalMargin : 4, verticalMargin : 4,
		verbose : 1,
		suggestWidth : 1, // Prevent recalculation of the preferred[Width/Height]
		suggestHeight : 1, // on MEASURE messages with grids.
		relayout : 1, repaint : 1,
		tabStop : 1, disabled : 1;
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
	UIImage **backgrounds;
	UIImage *icon;

	void *context;

	OSCommand *command;
	LinkedItem<Control> commandItem;

	OSCallback notificationCallback;
	OSMenuSpecification *rightClickMenu;

	OSString text;
	OSRectangle textBounds;
	uint32_t textColor;
	uint8_t textSize, textAlign;
	uint8_t textShadow : 1, 
		textBold : 1,
		textShadowBlur : 1,
		customTextRendering : 1;

	// Configuration:

	uint32_t focusable : 1,
		checkable : 1,

		noAnimations : 1,
		noDisabledTextColorChange : 1,
		ignoreActivationClicks : 1,
		drawParentBackground : 1,

		additionalCheckedBackgrounds : 1,
		additionalCheckedIcons : 1,
		hasFocusedBackground : 1,
		centerIcons : 1,
		iconHasVariants : 1,

		keepCustomCursorWhenDisabled : 1,
		cursor : 5;

	// State:

	uint32_t isChecked : 1, 
		firstPaint : 1,
		repaintCustomOnly : 1,
		pressedByKeyboard : 1;

	LinkedItem<Control> timerControlItem;

	// Animation data:
	uint8_t timerHz, timerStep;
	uint8_t animationStep, finalAnimationStep;
	uint8_t from1, from2, from3, from4, from5;
	uint8_t current1, current2, current3, current4, current5;
};

struct ListView : Control {
	unsigned flags;
	size_t itemCount;
	OSObject scrollbar;
	int scrollY;
	int32_t highlightRow, lastClickedRow;

	OSPoint selectionBoxAnchor;
	OSPoint selectionBoxPosition;
	OSRectangle selectionBox;

	OSListViewColumn *columns;
	int32_t columnsCount;
	int32_t rowWidth;

	int repaintFirstRow, repaintLastRow;
	int draggingColumnIndex, draggingColumnX;
	bool repaintSelectionBox;
	OSRectangle oldSelectionBox;
	
	enum {
		DRAGGING_NONE,
		DRAGGING_ITEMS,
		DRAGGING_SELECTION,
		DRAGGING_COLUMN,
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
	bool sentEditResultNotification;
	OSString previousString;
	OSTextboxStyle style;
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
	OSGridStyle style;
	OSRectangle borderSize;
	uint16_t gapSize;
	UIImage *background;
	uint32_t backgroundColor;
	OSCallback notificationCallback;
	int xOffset, yOffset;
	bool treatPreferredDimensionsAsMinima; // Used with scroll panes for PUSH objects.
};

struct Scrollbar : Grid {
	bool enabled;
	bool orientation;

	int contentSize;
	int viewportSize;
	int height;

	int anchor;
	float position;
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
	bool hasMenuParent;
};

struct OpenMenu {
	Window *window;
	Control *source;
};

static OpenMenu openMenus[8];
static unsigned openMenuCount;

static bool ClipRectangle(OSRectangle parent, OSRectangle rectangle, OSRectangle *output) {
	OSRectangle current = parent;
	OSRectangle intersection;

	if (!((current.left > rectangle.right && current.right > rectangle.left)
			|| (current.top > rectangle.bottom && current.bottom > rectangle.top))) {
		intersection.left = current.left > rectangle.left ? current.left : rectangle.left;
		intersection.top = current.top > rectangle.top ? current.top : rectangle.top;
		intersection.right = current.right < rectangle.right ? current.right : rectangle.right;
		intersection.bottom = current.bottom < rectangle.bottom ? current.bottom : rectangle.bottom;
	} else {
		intersection = OS_MAKE_RECTANGLE(0, 0, 0, 0);
	}

	if (output) {
		*output = intersection;
	}

	return intersection.left < intersection.right && intersection.top < intersection.bottom;
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

static inline void OSRepaintControl(OSObject _control, bool customOnly = false) {
	Control *control = (Control *) _control;

	if (!control->repaint) {
		control->repaint = true;
		SetParentDescendentInvalidationFlags(control, DESCENDENT_REPAINT);
	}

	if (!customOnly) {
		control->repaintCustomOnly = false;
	}
}

static inline bool IsPointInRectangle(OSRectangle rectangle, int x, int y) {
	if (rectangle.left > x || rectangle.right <= x || rectangle.top > y || rectangle.bottom <= y) {
		return false;
	}
	
	return true;
}

void OSSetControlCommand(OSObject _control, OSCommand *_command) {
	Control *control = (Control *) _control;
	
	if (_command->iconID) {
		control->icon = icons16 + _command->iconID;
		control->iconHasVariants = true;
	}

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
		control->from5 = control->current5;

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

	int preferredWidth = (object->layout & OS_CELL_H_EXPAND) ? width : (object->preferredWidth + object->horizontalMargin * 2);
	int preferredHeight = (object->layout & OS_CELL_V_EXPAND) ? height : (object->preferredHeight + object->verticalMargin * 2);

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

static void SetGUIObjectProperty(GUIObject *object, OSMessage *message) {
	uintptr_t value = (uintptr_t) message->setProperty.value;

	switch (message->setProperty.index) {
		case OS_GUI_OBJECT_PROPERTY_SUGGESTED_WIDTH: {
			object->suggestWidth = true;
			object->preferredWidth = value;
			object->relayout = true;

			{
				OSMessage message;
				message.type = OS_MESSAGE_CHILD_UPDATED;
				OSSendMessage(object->parent, &message);
			}
		} break;

		case OS_GUI_OBJECT_PROPERTY_SUGGESTED_HEIGHT: {
			object->suggestHeight = true;
			object->preferredHeight = value;
			object->relayout = true;

			{
				OSMessage message;
				message.type = OS_MESSAGE_CHILD_UPDATED;
				OSSendMessage(object->parent, &message);
			}
		} break;
	}
}

OSObject OSGetFocusedControl(OSObject _window, bool ignoreWeakFocus) {
	Window *window = (Window *) _window;

	if (ignoreWeakFocus) {
		return window->focus;
	} else {
		return window->lastFocus;
	}
}

void OSSetFocusedControl(OSObject _control) {
	Control *control = (Control *) _control;
	Window *window = control->window;
	OSMessage message;

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

			if (control->verbose) {
				OSPrint("Cell layout for control %x: %d->%d, %d->%d\n", control, control->bounds.left, control->bounds.right, control->bounds.top, control->bounds.bottom);
			}

			StandardCellLayout(control);
			ClipRectangle(message->layout.clip, control->bounds, &control->inputBounds);

			if (control->verbose) {
				OSPrint("Layout control %x: %d->%d, %d->%d; input %d->%d, %d->%d\n", control, control->bounds.left, control->bounds.right, control->bounds.top, control->bounds.bottom,
						control->inputBounds.left, control->inputBounds.right, control->inputBounds.top, control->inputBounds.bottom);
			}

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
			OSRectangle contentBounds = control->bounds;
			contentBounds.left += control->horizontalMargin;
			contentBounds.right -= control->horizontalMargin;
			contentBounds.top += control->verticalMargin;
			contentBounds.bottom -= control->verticalMargin;

			control->textBounds = contentBounds;

			if (control->icon) {
				control->textBounds.left += control->icon->region.right - control->icon->region.left + ICON_TEXT_GAP;
			}
		} break;

		case OS_MESSAGE_MEASURE: {
			message->measure.preferredWidth = control->preferredWidth + control->horizontalMargin * 2;
			message->measure.preferredHeight = control->preferredHeight + control->verticalMargin * 2;
		} break;

		case OS_MESSAGE_KEY_PRESSED: {
			if (control->tabStop && control->window->lastFocus != control) {
				OSSetFocusedControl(control);
			} else {
				response = OS_CALLBACK_NOT_HANDLED;
			}
		} break;

		case OS_MESSAGE_PAINT: {
			if (control->repaint || message->paint.force) {
				// if (!control->firstPaint) OSPrint("first paint %x\n", control);
				control->firstPaint = true;

#if 0
				if (control->window->focus == control) {
					OSFillRectangle(message->paint.surface, message->paint.clip, OSColor(255, 0, 255));
					break;
				}
#endif

				bool menuSource;
				bool normal, hover, pressed, disabled, focused;
				uint32_t textShadowColor, textColor;

				OSRectangle contentBounds = control->bounds;
				contentBounds.left += control->horizontalMargin;
				contentBounds.right -= control->horizontalMargin;
				contentBounds.top += control->verticalMargin;
				contentBounds.bottom -= control->verticalMargin;
				if (control->repaintCustomOnly && !message->paint.force) {
					goto repaintCustom;
				} else {
					control->repaintCustomOnly = false;
				}
				
				if (control->drawParentBackground) {
					OSMessage m = *message;
					m.type = OS_MESSAGE_PAINT_BACKGROUND;
					m.paintBackground.surface = message->paint.surface;
					m.paintBackground.left = control->bounds.left;
					m.paintBackground.right = control->bounds.right;
					m.paintBackground.top = control->bounds.top;
					m.paintBackground.bottom = control->bounds.bottom;
					m.paintBackground.clip = message->paint.clip;
					OSSendMessage(control->parent, &m);
				}

				menuSource = false;

				for (uintptr_t i = 0; i < openMenuCount; i++) {
					if (openMenus[i].source == control) {
						menuSource = true;
						break;
					}
				}

				disabled = control->disabled;
				pressed = ((control->window->pressed == control && (control->window->hover == control || control->pressedByKeyboard)) 
						|| (control->window->focus == control && !control->hasFocusedBackground) || menuSource) && !disabled;
				hover = (control->window->hover == control || control->window->pressed == control) && !pressed && !disabled;
				focused = (control->window->focus == control && control->hasFocusedBackground) && !pressed && !hover && !disabled;
				normal = !hover && !pressed && !disabled && !focused;

				control->current1 = ((normal   ? 15 : 0) - control->from1) * control->animationStep / control->finalAnimationStep + control->from1;
				control->current2 = ((hover    ? 15 : 0) - control->from2) * control->animationStep / control->finalAnimationStep + control->from2;
				control->current3 = ((pressed  ? 15 : 0) - control->from3) * control->animationStep / control->finalAnimationStep + control->from3;
				control->current4 = ((disabled ? 15 : 0) - control->from4) * control->animationStep / control->finalAnimationStep + control->from4;
				control->current5 = ((focused  ? 15 : 0) - control->from5) * control->animationStep / control->finalAnimationStep + control->from5;

				if (control->backgrounds) {
					uintptr_t offset = (control->isChecked && control->additionalCheckedBackgrounds) ? 4 : 0;

					if (control->backgrounds[offset + 0]) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[offset + 0]->region, 
								control->backgrounds[offset + 0]->border, control->backgrounds[offset + 0]->drawMode, 0xFF, message->paint.clip);
					}

					if (control->backgrounds[offset + 2] && control->current2) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[offset + 2]->region, 
								control->backgrounds[offset + 2]->border, control->backgrounds[offset + 2]->drawMode, 
								control->current2 == 15 ? 0xFF : 0xF * control->current2, message->paint.clip);
					}

					if (control->backgrounds[offset + 3] && control->current3) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[3]->region, 
								control->backgrounds[3]->border, control->backgrounds[3]->drawMode, 
								control->current3 == 15 ? 0xFF : 0xF * control->current3, message->paint.clip);
					}

					if (control->backgrounds[offset + 1] && control->current4) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[offset + 1]->region, 
								control->backgrounds[offset + 1]->border, control->backgrounds[offset + 1]->drawMode, 
								control->current4 == 15 ? 0xFF : 0xF * control->current4, message->paint.clip);
					}

					if (control->current5 && control->backgrounds[offset + 4]) {
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, control->backgrounds[offset + 4]->region, 
								control->backgrounds[offset + 4]->border, control->backgrounds[offset + 4]->drawMode, 
								control->current5 == 15 ? 0xFF : 0xF * control->current5, message->paint.clip);
					}
				}

				if (control->icon) {
					OSRectangle bounds = contentBounds;
					UIImage *icon = control->icon;
					
					if (control->centerIcons) {
						bounds.left += (bounds.right - bounds.left) / 2 - (icon->region.right - icon->region.left) / 2;
						bounds.right = bounds.left + icon->region.right - icon->region.left;
						bounds.top += (bounds.bottom - bounds.top) / 2 - (icon->region.bottom - icon->region.top) / 2;
						bounds.bottom = bounds.top + icon->region.bottom - icon->region.top;
					} else {
						bounds.right = bounds.left + icon->region.right - icon->region.left;
						bounds.top += (bounds.bottom - bounds.top) / 2 - (icon->region.bottom - icon->region.top) / 2;
						bounds.bottom = bounds.top + icon->region.bottom - icon->region.top;
					}

					uintptr_t offset = (control->isChecked && control->additionalCheckedIcons) ? 4 : 0;

					if (control->current1 || !control->iconHasVariants) {
						UIImage icon = control->icon->Translate((control->icon->region.right - control->icon->region.left) * (0 + offset), 0);
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, icon.region, 
								icon.border, icon.drawMode, 0xFF, message->paint.clip);
					}

					if (control->current2 && control->iconHasVariants) {
						UIImage icon = control->icon->Translate((control->icon->region.right - control->icon->region.left) * (3 + offset), 0);
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, icon.region,
								icon.border, icon.drawMode, control->current2 == 15 ? 0xFF : 0xF * control->current2, message->paint.clip);
					}

					if (control->current3 && control->iconHasVariants) {
						UIImage icon = control->icon->Translate((control->icon->region.right - control->icon->region.left) * (2 + offset), 0);
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, icon.region,
								icon.border, icon.drawMode, control->current3 == 15 ? 0xFF : 0xF * control->current3, message->paint.clip);
					}

					if (control->current4 && control->iconHasVariants) {
						UIImage icon = control->icon->Translate((control->icon->region.right - control->icon->region.left) * (1 + offset), 0);
						OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, bounds, icon.region,
								icon.border, icon.drawMode, control->current4 == 15 ? 0xFF : 0xF * control->current4, message->paint.clip);
					}
				}

				textColor = control->textColor ? control->textColor : TEXT_COLOR_DEFAULT;
				textShadowColor = 0xFFFFFF - textColor;

				if (control->disabled && !control->noDisabledTextColorChange) {
					textColor ^= TEXT_COLOR_DISABLED;
					textShadowColor = TEXT_COLOR_DISABLED_SHADOW;
				}

				if (!control->customTextRendering) {
					OSRectangle textBounds = control->textBounds;

					if (control->textShadow && !DISABLE_TEXT_SHADOWS && (!control->disabled || control->noDisabledTextColorChange)) {
						OSRectangle bounds = textBounds;
						bounds.top++; bounds.bottom++; 
						// bounds.left++; bounds.right++;

						OSDrawString(message->paint.surface, bounds, &control->text, control->textSize,
								control->textAlign, textShadowColor, -1, control->textBold, message->paint.clip, 
								control->textShadowBlur ? 3 : 0);
					}

					OSDrawString(message->paint.surface, textBounds, &control->text, control->textSize,
							control->textAlign, textColor, -1, control->textBold, message->paint.clip, 0);
				}

				repaintCustom:;

				{
					OSMessage m = *message;
					m.type = OS_MESSAGE_CUSTOM_PAINT;
					OSSendMessage(control, &m);
				}

				control->repaint = false;
				control->repaintCustomOnly = true;
			}
		} break;

		case OS_MESSAGE_PARENT_UPDATED: {
			control->window = (Window *) message->parentUpdated.window;
			control->animationStep = 16;
			control->finalAnimationStep = 16;
			OSRepaintControl(control);

			if (control->command) {
				OSSetControlCommand(control, control->command);
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

			if (control->window->hover != control) {
				if (!control->disabled || control->keepCustomCursorWhenDisabled) {
					control->window->cursor = (OSCursorStyle) control->cursor;
				}

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

		case OS_MESSAGE_SET_PROPERTY: {
			SetGUIObjectProperty(control, message);
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

static void CreateString(char *text, size_t textBytes, OSString *string, size_t characterCount = 0) {
	OSHeapFree(string->buffer);
	string->buffer = (char *) OSHeapAllocate(textBytes, false);
	string->bytes = textBytes;
	string->characters = characterCount;
	OSCopyMemory(string->buffer, text, textBytes);

	char *m = string->buffer;

	if (!string->characters) {
		while (m < string->buffer + textBytes) {
			m = utf8_advance(m);
			string->characters++;
		}
	}
}

static OSCallbackResponse ProcessWindowResizeHandleMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	WindowResizeControl *control = (WindowResizeControl *) _object;

	if (message->type == OS_MESSAGE_MOUSE_DRAGGED && control->direction != RESIZE_NONE) {
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
		layout.layout.clip = OS_MAKE_RECTANGLE(0, window->width, 0, window->height);
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
			control->textColor = TEXT_COLOR_TITLEBAR;
			control->textShadow = true;
			control->textBold = true;
			control->textSize = 10;
			control->textShadowBlur = true;
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
				&control->text, control->textAlign, control->textColor, -1, control->window->focus == control ? TEXTBOX_SELECTED_COLOR_1 : TEXTBOX_SELECTED_COLOR_2,
				{0, 0}, nullptr, control->caret.character, control->window->lastFocus == control 
				&& !control->disabled ? control->caret2.character : control->caret.character, 
				control->window->lastFocus != control || control->caretBlink,
				control->textSize, fontRegular, message->paint.clip, 0);

#if 0
		OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->textBounds, 
				testImage.region, testImage.border, OS_DRAW_MODE_STRECH, 0xFF, message->paint.clip);
#endif

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
	} else if (message->type == OS_MESSAGE_END_FOCUS) {
		OSMessage message;

		if (!control->sentEditResultNotification) {
			message.type = OS_NOTIFICATION_CANCEL_EDIT;
			OSForwardMessage(control, control->notificationCallback, &message);

			if (control->style == OS_TEXTBOX_STYLE_COMMAND) {
				CreateString(control->previousString.buffer, control->previousString.bytes, &control->text, control->previousString.characters);
			}
		}

		message.type = OS_NOTIFICATION_END_EDIT;
		OSForwardMessage(control, control->notificationCallback, &message);
		control->cursor = control->style == OS_TEXTBOX_STYLE_COMMAND ? OS_CURSOR_NORMAL : OS_CURSOR_TEXT;
		control->window->cursor = (OSCursorStyle) control->cursor;

		OSHeapFree(control->previousString.buffer);
		control->previousString.buffer = nullptr;
	} else if (message->type == OS_MESSAGE_START_FOCUS) {
		control->caretBlink = false;
		control->window->caretBlinkPause = CARET_BLINK_PAUSE;

		control->sentEditResultNotification = false;

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

		{
			OSMessage message;
			message.type = OS_NOTIFICATION_START_EDIT;
			OSForwardMessage(control, control->notificationCallback, &message);
			control->cursor = OS_CURSOR_TEXT;
			control->window->cursor = (OSCursorStyle) control->cursor;

			if (control->style == OS_TEXTBOX_STYLE_COMMAND) {
				CreateString(control->text.buffer, control->text.bytes, &control->previousString, control->text.characters);
			}
		}

		OSSyscall(OS_SYSCALL_RESET_CLICK_CHAIN, 0, 0, 0, 0);
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
	} else if (message->type == OS_MESSAGE_DESTROY) {
		OSHeapFree(control->previousString.buffer);
	} else if (message->type == OS_MESSAGE_KEY_TYPED) {
		control->caretBlink = false;
		control->window->caretBlinkPause = CARET_BLINK_PAUSE;

		int ic = -1, isc = -1;

		result = OS_CALLBACK_HANDLED;

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

			case OS_SCANCODE_ENTER: {
				OSMessage message;
				message.type = OS_NOTIFICATION_CONFIRM_EDIT;
				OSForwardMessage(control, control->notificationCallback, &message);
				control->sentEditResultNotification = true;

				message.type = OS_NOTIFICATION_COMMAND;
				message.command.window = control->window;
				message.command.command = control->command;

				OSCallbackResponse response = OSForwardMessage(control, control->notificationCallback, &message);

				if (response == OS_CALLBACK_REJECTED) {
					CreateString(control->previousString.buffer, control->previousString.bytes, &control->text, control->previousString.characters);
				}

				message.type = OS_MESSAGE_END_FOCUS;
				OSSendMessage(control, &message);
				control->window->focus = nullptr;
				message.type = OS_MESSAGE_END_LAST_FOCUS;
				OSSendMessage(control, &message);
				control->window->lastFocus = nullptr;
			} break;

			default: {
				result = OS_CALLBACK_NOT_HANDLED;
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

OSObject OSCreateTextbox(OSTextboxStyle style) {
	Textbox *control = (Textbox *) OSHeapAllocate(sizeof(Textbox), true);

	control->type = API_OBJECT_CONTROL;
	control->tabStop = true;
	control->preferredWidth = 160;
	control->drawParentBackground = true;
	control->backgrounds = style == OS_TEXTBOX_STYLE_COMMAND ? textboxCommandBackgrounds : textboxBackgrounds;
	control->cursor = style == OS_TEXTBOX_STYLE_COMMAND ? OS_CURSOR_NORMAL : OS_CURSOR_TEXT;
	control->focusable = true;
	control->customTextRendering = true;
	control->textAlign = OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT;
	control->textSize = style == OS_TEXTBOX_STYLE_LARGE ? FONT_SIZE * 2 : FONT_SIZE;
	control->textColor = TEXT_COLOR_DEFAULT;
	control->preferredHeight = 23 - 9 + control->textSize;
	control->style = style;
	control->rightClickMenu = osMenuTextboxContext;

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessTextboxMessage, control));

	return control;
}

static void IssueCommandForControl(Control *control) {
	if (control->checkable) {
		// Update the checked state.
		control->isChecked = !control->isChecked;

		if (control->command) {
			// Update the command.
			OSCheckCommand(control->window, control->command, control->isChecked);
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
}

OSCallbackResponse ProcessButtonMessage(OSObject object, OSMessage *message) {
	Control *control = (Control *) object;
	OSCallbackResponse result = OS_CALLBACK_NOT_HANDLED;
	
	if (message->type == OS_MESSAGE_CLICKED) {
		IssueCommandForControl(control);
	} else if (message->type == OS_MESSAGE_KEY_PRESSED) {
		if (message->keyboard.scancode == OS_SCANCODE_SPACE) {
			control->window->pressed = control;
			control->pressedByKeyboard = true;
			OSAnimateControl(control, true);
			result = OS_CALLBACK_HANDLED;
		}
	} else if (message->type == OS_MESSAGE_KEY_RELEASED) {
		if (message->keyboard.scancode == OS_SCANCODE_SPACE) {
			control->window->pressed = nullptr;
			control->pressedByKeyboard = false;
			OSAnimateControl(control, false);
			result = OS_CALLBACK_HANDLED;
			IssueCommandForControl(control);
		}
	}

	if (result == OS_CALLBACK_NOT_HANDLED) {
		result = OSForwardMessage(object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
	}

	return result;
}

OSObject OSCreateButton(OSCommand *command, OSButtonStyle style) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;
	control->tabStop = true;

	control->preferredWidth = 80;
	control->preferredHeight = 21;
	control->textColor = TEXT_COLOR_DEFAULT;

	control->drawParentBackground = true;
	control->ignoreActivationClicks = true;

	OSSetControlCommand(control, command);

	if (style == OS_BUTTON_STYLE_TOOLBAR) {
		control->textColor = TEXT_COLOR_TOOLBAR;
		control->horizontalMargin = 12;
		control->preferredWidth = 0;
		control->preferredHeight = 31;
		control->textShadowBlur = true;
		control->textShadow = true;
		control->backgrounds = toolbarItemBackgrounds;
		control->additionalCheckedBackgrounds = true;
	} else if (style == OS_BUTTON_STYLE_TOOLBAR_ICON_ONLY) {
		control->horizontalMargin = 6;
		control->preferredWidth = 32;
		control->preferredHeight = 31;
		control->centerIcons = true;
		control->backgrounds = toolbarItemBackgrounds;
		control->additionalCheckedBackgrounds = true;
	} else {
		control->focusable = true;
		control->hasFocusedBackground = true;

		if (control->checkable) {
			control->textAlign = OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT;
			control->icon = &checkboxHover;
			control->iconHasVariants = true;
			control->additionalCheckedIcons = true;
		} else {
			control->backgrounds = command->dangerous ? buttonDangerousBackgrounds : buttonBackgrounds;
		}
	}

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessButtonMessage, nullptr));

	if (style != OS_BUTTON_STYLE_TOOLBAR_ICON_ONLY) {
		OSSetText(control, command->label, command->labelBytes, OS_RESIZE_MODE_GROW_ONLY);
	} 

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
		result = OSForwardMessage(object, OS_MAKE_CALLBACK(ProcessButtonMessage, nullptr), message);
	}

	return result;
}

static void RepaintListViewRows(ListView *control, int from, int to) {
	OSRepaintControl(control, true);

	if (from > to) {
		int temp = from;
		from = to;
		to = temp;
	}

	if (from != 0) from--;
	if (to != (int) control->itemCount) to++;

	if (control->repaintFirstRow == -1 || control->repaintFirstRow > from) control->repaintFirstRow = from;
	if (control->repaintLastRow == -1 || control->repaintLastRow > to) control->repaintLastRow = to;
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
	// TODO Rewrite all of this.

	ListView *control = (ListView *) object;
	OSCallbackResponse result = OS_CALLBACK_NOT_HANDLED;

	bool redrawScrollbar = false;

	int margin = LIST_VIEW_WITH_BORDER_MARGIN;
	OSRectangle bounds = control->bounds;
	bounds.top += control->columns ? LIST_VIEW_HEADER_HEIGHT : 0;
	bounds.top += LIST_VIEW_MARGIN / 2;
	bounds.left += margin;
	bounds.right -= margin + SCROLLBAR_SIZE;

	if (control->flags & OS_CREATE_LIST_VIEW_BORDER) {
		bounds.top += STANDARD_BORDER_SIZE;
		bounds.bottom -= STANDARD_BORDER_SIZE;
	}

	int previousHighlightRow = control->highlightRow;

	switch (message->type) {
		case OS_MESSAGE_PAINT: {
			// If the list gets redrawn, the scrollbar will need to be as well.
			if (control->repaint) redrawScrollbar = true;
		} break;

		case OS_MESSAGE_CUSTOM_PAINT: {
			OSHandle surface = message->paint.surface;

			if (control->repaintFirstRow == -1 || !control->repaintCustomOnly || control->repaintSelectionBox) {
				control->repaintFirstRow = 0;
			}

			if (control->repaintLastRow == (int) control->itemCount || !control->repaintCustomOnly || control->repaintSelectionBox) {
				control->repaintLastRow = control->itemCount - 1;
			}

			OSRectangle clip;

			if (ClipRectangle(message->paint.clip, control->bounds, &clip)) {
				if (!(control->flags & OS_CREATE_LIST_VIEW_BORDER) && !control->repaintCustomOnly) {
					// Draw the background.
					OSFillRectangle(surface, clip, OSColor(LIST_VIEW_BACKGROUND_COLOR));
				}

				OSRectangle listClip;

				{
					OSRectangle area = control->bounds;
					area.top += control->columns ? LIST_VIEW_HEADER_HEIGHT : 0;

					if (control->flags & OS_CREATE_LIST_VIEW_BORDER) {
						area.top += STANDARD_BORDER_SIZE;
						area.bottom -= STANDARD_BORDER_SIZE;
						area.left += STANDARD_BORDER_SIZE;
						area.right -= STANDARD_BORDER_SIZE;
					}

					ClipRectangle(clip, area, &listClip);
				}

				OSRectangle clip2;

				if (control->columns && !control->repaintCustomOnly) {
					OSRectangle headerBounds = OS_MAKE_RECTANGLE(bounds.left, bounds.right, 
							bounds.top - LIST_VIEW_HEADER_HEIGHT - LIST_VIEW_MARGIN / 2, 
							bounds.top - LIST_VIEW_MARGIN / 2);

					if (!(control->flags & OS_CREATE_LIST_VIEW_BORDER)) {
						OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, 
								OS_MAKE_RECTANGLE(control->bounds.left, control->bounds.right, headerBounds.top, headerBounds.bottom),
								listViewColumnHeader.region, listViewColumnHeader.border, 
								OS_DRAW_MODE_STRECH, 0xFF, clip);
					}

					if (ClipRectangle(clip, headerBounds, &clip2)) {
						int x = 0;

						for (int i = 0; i < control->columnsCount; i++) {
							OSListViewColumn *column = control->columns + i;

							OSString string; 
							string.buffer = column->title;
							string.bytes = column->titleBytes;

							OSRectangle region = OS_MAKE_RECTANGLE(headerBounds.left + x + 2, headerBounds.left + x + column->width - 2, 
									headerBounds.top + 2, headerBounds.bottom);

							DrawString(surface, OS_MAKE_RECTANGLE(region.left, region.right - 10, region.top, region.bottom), &string, OS_DRAW_STRING_HALIGN_LEFT | OS_DRAW_STRING_VALIGN_CENTER,
									LIST_VIEW_COLUMN_TEXT_COLOR, -1, 0, OS_MAKE_POINT(0, 0), nullptr, 0, 0, true, FONT_SIZE, fontRegular, clip2, 0);

							OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, 
									OS_MAKE_RECTANGLE(region.right - 8, region.right - 7, 
										headerBounds.top, headerBounds.bottom),
									listViewColumnHeaderDivider.region, listViewColumnHeaderDivider.border, 
									OS_DRAW_MODE_REPEAT_FIRST, 0xFF, clip2);

							x += column->width;
						}
					}
				}
		
				ClipRectangle(clip, bounds, &clip2);

				if (control->dragging == ListView::DRAGGING_SELECTION && control->repaintCustomOnly && control->repaintSelectionBox) {
					OSRectangle r;
					ClipRectangle(control->oldSelectionBox, listClip, &r);
					OSFillRectangle(surface, r, OSColor(LIST_VIEW_BACKGROUND_COLOR));
				}

				int i = 0;
				int y = -control->scrollY;

				{
					i += control->scrollY / LIST_VIEW_ROW_HEIGHT;
					y += i * LIST_VIEW_ROW_HEIGHT;
				}

				if (i < control->repaintFirstRow) {
					y += (control->repaintFirstRow - i) * LIST_VIEW_ROW_HEIGHT;
					i = control->repaintFirstRow;
				}

				int boundsWidth = bounds.right - bounds.left;
				int rowWidthUnclipped = control->rowWidth ? control->rowWidth : boundsWidth;
				int rowWidth = control->rowWidth ? (control->rowWidth > boundsWidth ? boundsWidth : control->rowWidth) : boundsWidth; 

				if (i) {
					i--;
					y -= LIST_VIEW_ROW_HEIGHT;
				}

				for (; i < (int) control->itemCount && i <= control->repaintLastRow; i++) {
					OSRectangle row = OS_MAKE_RECTANGLE(bounds.left, bounds.left + rowWidth, 
							bounds.top + y, bounds.top + y + LIST_VIEW_ROW_HEIGHT - 1);

					OSRectangle clip3;

					if (control->repaintCustomOnly && control->repaintSelectionBox) {
						if (!ClipRectangle(row, control->oldSelectionBox, nullptr)) {
							goto next;
						}
					}
		
					if (ClipRectangle(clip, row, &clip3)) {
						ClipRectangle(clip3, clip2, &clip3);

						OSMessage message;
						message.type = OS_NOTIFICATION_GET_ITEM;
						message.listViewItem.index = i;
						message.listViewItem.column = 0;
						message.listViewItem.mask = OS_LIST_VIEW_ITEM_TEXT | OS_LIST_VIEW_ITEM_SELECTED | OS_LIST_VIEW_ITEM_ICON;
						message.listViewItem.state = 0;
						message.listViewItem.iconID = 0;
		
						if (OSForwardMessage(control, control->notificationCallback, &message) != OS_CALLBACK_HANDLED) {
							OSCrashProcess(OS_FATAL_ERROR_MESSAGE_SHOULD_BE_HANDLED);
						}

						uint16_t iconID = message.listViewItem.iconID;
		
						char *text = message.listViewItem.text;
						size_t textBytes = message.listViewItem.textBytes;

						OSRectangle fullRow = OS_MAKE_RECTANGLE(bounds.left, bounds.left + rowWidthUnclipped, row.top, row.bottom);
						OSRectangle clip4;
						ClipRectangle(clip, fullRow, &clip4);
						ClipRectangle(clip4, OS_MAKE_RECTANGLE(clip4.left, clip4.right, bounds.top - LIST_VIEW_MARGIN / 2, bounds.bottom), &clip4);
		
						{
							// Only redraw the white background if we didn't redraw the whole control.
							if (control->repaintCustomOnly) {
								OSFillRectangle(surface, clip4, OSColor(LIST_VIEW_BACKGROUND_COLOR));
							}

							if (message.listViewItem.state & OS_LIST_VIEW_ITEM_SELECTED) {
								UIImage image = control->window->focus == control ? listViewSelected : listViewSelected2;
								OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, fullRow,
										image.region, image.border, OS_DRAW_MODE_STRECH, 0xFF, clip4);
							} else if (control->highlightRow == i) {
								OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, fullRow,
										listViewHighlight.region, listViewHighlight.border, OS_DRAW_MODE_STRECH, 0xFF, clip4);
							}

							if (control->lastClickedRow == i && control->window->focus == control) {
								UIImage image = listViewLastClicked;
								OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, fullRow,
										image.region, image.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, clip4);
							}
						}

						OSString string;
						string.buffer = text;
						string.bytes = textBytes;
		
						message.type = OS_NOTIFICATION_GET_ITEM;
						message.listViewItem.index = i;
						message.listViewItem.mask = OS_LIST_VIEW_ITEM_TEXT;
						message.listViewItem.state = 0;

						bool primary = true;
						bool rightAligned = false;
						bool icon = iconID;
						int x = 0;

						for (int i = 0; i < (control->columnsCount ? control->columnsCount : 1); i++) {
							if (i) {
								message.listViewItem.column = i;

								if (OSForwardMessage(control, control->notificationCallback, &message) != OS_CALLBACK_HANDLED) {
									continue;
								}

								string.buffer = message.listViewItem.text;
								string.bytes = message.listViewItem.textBytes;
							}

							int width = control->columns ? control->columns[i].width : row.right - row.left;

							OSRectangle region = row;
							region.left = row.left + x + LIST_VIEW_TEXT_MARGIN;
							region.right = row.left + x + width - LIST_VIEW_TEXT_MARGIN - 8;

							x += width;

							if (control->columns) {
								primary = control->columns[i].flags & OS_LIST_VIEW_COLUMN_PRIMARY;
								rightAligned = control->columns[i].flags & OS_LIST_VIEW_COLUMN_RIGHT_ALIGNED;
								icon = iconID && control->columns[i].flags & OS_LIST_VIEW_COLUMN_ICON;
							}

							if (icon) {
								int h = (region.bottom - region.top) / 2 + region.top - 8;
								UIImage image = icons16[iconID];
								OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, 
										OS_MAKE_RECTANGLE(region.left, region.left + 16, h, h + 16),
										image.region, image.border, OS_DRAW_MODE_REPEAT_FIRST, 0xFF, clip4);

								region.left += 20;
							}

							DrawString(surface, region, &string, 
									(rightAligned ? OS_DRAW_STRING_HALIGN_RIGHT : OS_DRAW_STRING_HALIGN_LEFT) 
									| OS_DRAW_STRING_VALIGN_CENTER,
									primary ? LIST_VIEW_PRIMARY_TEXT_COLOR : LIST_VIEW_SECONDARY_TEXT_COLOR, -1, 0, 
									OS_MAKE_POINT(0, 0), nullptr, 0, 0, true, FONT_SIZE, fontRegular, clip4, 0);
						}
					}
		
					next:;
					y += LIST_VIEW_ROW_HEIGHT;
		
					if (y > bounds.bottom - bounds.top) {
						break;
					}
				}
		
				if (control->dragging == ListView::DRAGGING_SELECTION) {
					OSDrawSurfaceClipped(surface, OS_SURFACE_UI_SHEET, control->selectionBox,
							listViewSelectionBox.region, listViewSelectionBox.border, listViewSelectionBox.drawMode, 0xFF, listClip);
				}
			}
		
			control->repaintFirstRow = -1;
			control->repaintLastRow = -1;
			control->repaintSelectionBox = false;

			result = OS_CALLBACK_HANDLED;
		} break;

		case OS_MESSAGE_MOUSE_MOVED: {
			if (!IsPointInRectangle(control->bounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				control->highlightRow = -1;
				break;
			}

			OSSendMessage(control->scrollbar, message);

			OSRectangle headerBounds = OS_MAKE_RECTANGLE(bounds.left, bounds.right, 
					bounds.top - LIST_VIEW_HEADER_HEIGHT - LIST_VIEW_MARGIN / 2, 
					bounds.top - LIST_VIEW_MARGIN / 2);

			control->window->cursor = OS_CURSOR_NORMAL;

			if (control->columns && IsPointInRectangle(headerBounds, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				int x = 0;

				for (int i = 0; i < control->columnsCount; i++) {
					OSListViewColumn *column = control->columns + i;

					OSRectangle region = OS_MAKE_RECTANGLE(headerBounds.left + x + column->width - 14, headerBounds.left + x + column->width - 6, 
							headerBounds.top + 2, headerBounds.bottom);

					if (IsPointInRectangle(region, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
						control->window->cursor = OS_CURSOR_SPLIT_HORIZONTAL;
						break;
					}

					x += column->width;
				}

				break;
			}

			OSRectangle inputArea = OS_MAKE_RECTANGLE(bounds.left, control->columns ? control->rowWidth + bounds.left : bounds.right, bounds.top, bounds.bottom);

			ClipRectangle(inputArea, control->inputBounds, &inputArea);

			{
				OSRectangle area = control->bounds;
				area.top += control->columns ? LIST_VIEW_HEADER_HEIGHT : 0;

				if (control->flags & OS_CREATE_LIST_VIEW_BORDER) {
					area.top += STANDARD_BORDER_SIZE;
					area.bottom -= STANDARD_BORDER_SIZE;
					area.left += STANDARD_BORDER_SIZE;
					area.right -= STANDARD_BORDER_SIZE + SCROLLBAR_SIZE;
				}

				ClipRectangle(inputArea, area, &inputArea);
			}

			if (!IsPointInRectangle(inputArea, message->mouseMoved.newPositionX, message->mouseMoved.newPositionY)) {
				control->highlightRow = -1;
				break;
			}

			int y = message->mouseMoved.newPositionY - bounds.top + control->scrollY;
			control->highlightRow = y / LIST_VIEW_ROW_HEIGHT;
		} break;

		case OS_MESSAGE_MOUSE_LEFT_PRESSED: {
			OSRectangle listClip;

			{
				OSRectangle area = control->bounds;
				area.top += control->columns ? LIST_VIEW_HEADER_HEIGHT : 0;

				if (control->flags & OS_CREATE_LIST_VIEW_BORDER) {
					area.top += STANDARD_BORDER_SIZE;
					area.bottom -= STANDARD_BORDER_SIZE;
					area.left += STANDARD_BORDER_SIZE;
					area.right -= STANDARD_BORDER_SIZE + SCROLLBAR_SIZE;
				}

				ClipRectangle(control->inputBounds, area, &listClip);
			}

			if (!IsPointInRectangle(listClip, message->mousePressed.positionX, message->mousePressed.positionY)
					|| !(control->flags & OS_CREATE_LIST_VIEW_ANY_SELECTIONS)) {
				break;
			}

			OSMessage m;

			if ((!message->mousePressed.ctrl && !message->mousePressed.shift)
					|| !(control->flags & OS_CREATE_LIST_VIEW_MULTI_SELECT)) {
				// If neither CTRL nor SHIFT were pressed, remove the old selection.
				m.type = OS_NOTIFICATION_DESELECT_ALL;
				OSForwardMessage(control, control->notificationCallback, &m);
				OSRepaintControl(control);
			}

			OSRectangle inputArea = OS_MAKE_RECTANGLE(bounds.left, control->columns ? control->rowWidth + bounds.left : bounds.right, bounds.top, bounds.bottom);

			if (!IsPointInRectangle(inputArea, message->mousePressed.positionX, message->mousePressed.positionY)) {
				break;
			}

			// This is the row we clicked on.
			int y = message->mousePressed.positionY - bounds.top + control->scrollY;
			int row = y / LIST_VIEW_ROW_HEIGHT;

			if (row >= (int) control->itemCount || y < 0) {
				break;
			}

			if (message->mousePressed.shift && control->lastClickedRow != -1 && (control->flags & OS_CREATE_LIST_VIEW_MULTI_SELECT)) {
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

				RepaintListViewRows(control, low, high);
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
				RepaintListViewRows(control, row, row);
			} else {
				// If SHIFT wasn't pressed, only add this row to the selection.
				m.type = OS_NOTIFICATION_SET_ITEM;
				m.listViewItem.index = row;
				m.listViewItem.mask = OS_LIST_VIEW_ITEM_SELECTED;
				m.listViewItem.state = OS_LIST_VIEW_ITEM_SELECTED;
				OSForwardMessage(control, control->notificationCallback, &m);
				RepaintListViewRows(control, row, row);

				// If this was a double-click, "choose" this row.
				if (message->mousePressed.clickChainCount == 2) { 
					m.type = OS_NOTIFICATION_CHOOSE_ITEM;
					OSForwardMessage(control, control->notificationCallback, &m);
				}
			}

			control->lastClickedRow = row;
		} break;

		case OS_MESSAGE_START_DRAG: {
			control->selectionBoxAnchor = OS_MAKE_POINT(message->mouseDragged.originalPositionX, message->mouseDragged.originalPositionY);
			control->selectionBoxPosition = OS_MAKE_POINT(message->mouseDragged.newPositionX, message->mouseDragged.newPositionY);

			OSRectangle headerBounds = OS_MAKE_RECTANGLE(bounds.left, bounds.right, 
					bounds.top - LIST_VIEW_HEADER_HEIGHT - LIST_VIEW_MARGIN / 2, 
					bounds.top - LIST_VIEW_MARGIN / 2);

			if (control->columns && IsPointInRectangle(headerBounds, message->mouseDragged.originalPositionX, message->mouseDragged.originalPositionY)) {
				int x = 0;

				for (int i = 0; i < control->columnsCount; i++) {
					OSListViewColumn *column = control->columns + i;

					OSRectangle region = OS_MAKE_RECTANGLE(headerBounds.left + x + column->width - 14, headerBounds.left + x + column->width - 6, 
							headerBounds.top + 2, headerBounds.bottom);

					if (IsPointInRectangle(region, message->mouseDragged.originalPositionX, message->mouseDragged.originalPositionY)) {
						control->dragging = ListView::DRAGGING_COLUMN;
						control->draggingColumnX = x;
						control->draggingColumnIndex = i;
						break;
					}

					x += column->width;
				}

				break;
			}

			if (!(control->flags & OS_CREATE_LIST_VIEW_MULTI_SELECT)) {
				break;
			}

			control->dragging = ListView::DRAGGING_SELECTION;

			OSRepaintControl(control);
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			control->dragging = ListView::DRAGGING_NONE;
			OSRepaintControl(control);
		} break;

		case OS_MESSAGE_MOUSE_DRAGGED: {
			if (control->dragging == ListView::DRAGGING_COLUMN) {
				OSRepaintControl(control);

				int newWidth = message->mouseDragged.newPositionX - control->draggingColumnX - control->bounds.left;
				if (newWidth < 64) newWidth = 64;
				control->rowWidth += newWidth - control->columns[control->draggingColumnIndex].width;
				control->columns[control->draggingColumnIndex].width = newWidth;
			}

			if (control->dragging != ListView::DRAGGING_SELECTION) {
				break;
			}

			control->selectionBoxPosition = OS_MAKE_POINT(message->mouseDragged.newPositionX, message->mouseDragged.newPositionY);
			OSRepaintControl(control, true);
			control->repaintSelectionBox = true;

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

			control->oldSelectionBox = control->selectionBox;
			control->selectionBox = selectionBox;

			int boundsWidth = bounds.right - bounds.left;
			int rowWidth = control->rowWidth ? (control->rowWidth > boundsWidth ? boundsWidth : control->rowWidth) : boundsWidth; 

			int y = -control->scrollY;

			OSMessage m;
			m.type = OS_NOTIFICATION_SET_ITEM;
			m.listViewItem.mask = OS_LIST_VIEW_ITEM_SELECTED;

			// TODO Only check to update items that could have changed.

			for (int i = 0; i < (int) control->itemCount; i++) {
				OSRectangle row = OS_MAKE_RECTANGLE(bounds.left, bounds.left + rowWidth, 
						bounds.top + y, bounds.top + y + LIST_VIEW_ROW_HEIGHT);

				m.listViewItem.index = i;

				if (ClipRectangle(control->selectionBox, row, nullptr)) {
					m.listViewItem.state = OS_LIST_VIEW_ITEM_SELECTED;
					OSForwardMessage(control, control->notificationCallback, &m);
				} else {
					m.listViewItem.state = 0;
					OSForwardMessage(control, control->notificationCallback, &m);
				}

				y += LIST_VIEW_ROW_HEIGHT;
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
		RepaintListViewRows(control, previousHighlightRow, control->highlightRow);
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
			m.layout.clip = message->layout.clip;

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
					control->bounds.bottom - control->bounds.top 
					- ((control->flags & OS_CREATE_LIST_VIEW_BORDER) ? LIST_VIEW_WITH_BORDER_MARGIN : LIST_VIEW_MARGIN)
					- (control->columns ? LIST_VIEW_HEADER_HEIGHT : 0));

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
	control->tabStop = true;

	control->preferredWidth = 80;
	control->preferredHeight = 80;
	control->ignoreActivationClicks = true;
	control->noAnimations = true;
	control->focusable = true;

	if (flags & OS_CREATE_LIST_VIEW_BORDER) {
		control->drawParentBackground = true;
		control->backgrounds = textboxBackgrounds;
	}

	control->scrollbar = OSCreateScrollbar(OS_ORIENTATION_VERTICAL);
	((Control *) control->scrollbar)->parent = control;
	((Control *) control->scrollbar)->layout = OS_CELL_FILL;
	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessListViewMessage, nullptr));
	OSSetObjectNotificationCallback(control->scrollbar, OS_MAKE_CALLBACK(ListViewScrollbarMoved, control));

	control->lastClickedRow = 0;
	control->highlightRow = -1;

	{
		OSMessage message;
		message.parentUpdated.window = nullptr;
		message.type = OS_MESSAGE_PARENT_UPDATED;
		OSSendMessage(control->scrollbar, &message);
	}

	return control;
}

void OSListViewSetColumns(OSObject _listView, OSListViewColumn *columns, int32_t count) {
	ListView *control = (ListView *) _listView;
	control->columns = columns;
	control->columnsCount = count;

	control->rowWidth = 0;

	for (int i = 0; i < count; i++) {
		control->rowWidth += columns[i].width;
	}

	OSSetScrollbarMeasurements(control->scrollbar, control->itemCount * LIST_VIEW_ROW_HEIGHT, 
			control->bounds.bottom - control->bounds.top 
			- ((control->flags & OS_CREATE_LIST_VIEW_BORDER) ? LIST_VIEW_WITH_BORDER_MARGIN : LIST_VIEW_MARGIN)
			- (control->columns ? LIST_VIEW_HEADER_HEIGHT : 0));

	OSRepaintControl(control);
}

void OSListViewReset(OSObject _listView) {
	ListView *control = (ListView *) _listView;
	control->itemCount = 0;

	OSSetScrollbarMeasurements(control->scrollbar, 0, 
			control->bounds.bottom - control->bounds.top 
			- ((control->flags & OS_CREATE_LIST_VIEW_BORDER) ? LIST_VIEW_WITH_BORDER_MARGIN : LIST_VIEW_MARGIN)
			- (control->columns ? LIST_VIEW_HEADER_HEIGHT : 0));

	OSRepaintControl(control);
}

void OSListViewInsert(OSObject _listView, int32_t index, int32_t count) {
	ListView *control = (ListView *) _listView;

	if (index > (int) control->itemCount) {
		OSCrashProcess(OS_FATAL_ERROR_INDEX_OUT_OF_BOUNDS);
	}

	control->itemCount += count;
	RepaintListViewRows(control, index, index + count - 1);

	int scrollY = control->scrollY;

	OSSetScrollbarMeasurements(control->scrollbar, control->itemCount * LIST_VIEW_ROW_HEIGHT, 
			control->bounds.bottom - control->bounds.top 
			- ((control->flags & OS_CREATE_LIST_VIEW_BORDER) ? LIST_VIEW_WITH_BORDER_MARGIN : LIST_VIEW_MARGIN)
			- (control->columns ? LIST_VIEW_HEADER_HEIGHT : 0));

	if (scrollY / LIST_VIEW_ROW_HEIGHT >= index && scrollY) {
		OSSetScrollbarPosition(control->scrollbar, scrollY + count * LIST_VIEW_ROW_HEIGHT, true);
	}
}

void OSListViewInvalidate(OSObject _listView, int32_t index, int32_t count) {
	RepaintListViewRows((ListView *) _listView, index, index + count - 1);
}

static OSObject CreateMenuItem(OSMenuItem item, bool menubar) {
	MenuItem *control = (MenuItem *) OSHeapAllocate(sizeof(MenuItem), true);
	control->type = API_OBJECT_CONTROL;

	control->preferredWidth = !menubar ? 70 : 21;
	control->preferredHeight = 21;
	control->drawParentBackground = true;
	control->textAlign = menubar ? OS_FLAGS_DEFAULT : (OS_DRAW_STRING_VALIGN_CENTER | OS_DRAW_STRING_HALIGN_LEFT);
	control->backgrounds = menuItemBackgrounds;
	control->ignoreActivationClicks = menubar;
	control->noAnimations = true;

	control->item = item;
	control->menubar = menubar;

	if (item.type == OSMenuItem::COMMAND) {
		OSCommand *command = (OSCommand *) item.value;
		OSSetControlCommand(control, command);
		OSSetText(control, command->label, command->labelBytes, OS_RESIZE_MODE_GROW_ONLY);
	} else if (item.type == OSMenuItem::SUBMENU) {
		OSMenuSpecification *menu = (OSMenuSpecification *) item.value;
		OSSetText(control, menu->name, menu->nameBytes, OS_RESIZE_MODE_GROW_ONLY);
		if (!menubar) control->icon = &smallArrowRightNormal;
	}

	if (menubar) {
		control->preferredWidth += 4;
	} else {
		control->preferredWidth += 32;
	}

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessMenuItemMessage, nullptr));

	return control;
}

OSObject OSCreateLine(bool orientation) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;
	control->backgrounds = orientation ? lineVerticalBackgrounds : lineHorizontalBackgrounds;
	control->drawParentBackground = true;

	control->preferredWidth = 1;
	control->preferredHeight = 1;

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr));

	return control;
}

static OSCallbackResponse ProcessIconDisplayMessage(OSObject _object, OSMessage *message) {
	uint16_t iconID = (uintptr_t) message->context;
	Control *control = (Control *) _object;

	if (message->type == OS_MESSAGE_CUSTOM_PAINT) {
		OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, 
				icons32[iconID].region, icons32[iconID].border,
				OS_DRAW_MODE_REPEAT_FIRST, 0xFF, message->paint.clip);
		return OS_CALLBACK_HANDLED;
	} else {
		return OSForwardMessage(_object, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr), message);
	}
}

OSObject OSCreateIconDisplay(uint16_t iconID) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;
	control->drawParentBackground = true;
	control->preferredWidth = 32;
	control->preferredHeight = 32;

	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessIconDisplayMessage, (void *) (uintptr_t) iconID));
	return control;
}

OSObject OSCreateLabel(char *text, size_t textBytes) {
	Control *control = (Control *) OSHeapAllocate(sizeof(Control), true);
	control->type = API_OBJECT_CONTROL;
	control->drawParentBackground = true;
	control->textAlign = OS_DRAW_STRING_HALIGN_LEFT;

	OSSetText(control, text, textBytes, OS_RESIZE_MODE_EXACT);
	OSSetCallback(control, OS_MAKE_CALLBACK(ProcessControlMessage, nullptr));

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
				m.paintBackground.clip = message->paint.clip;
				OSSendMessage(control->parent, &m);
			}

			if (control->disabled) {
				OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, 
						control->backgrounds[1]->region, control->backgrounds[1]->border, control->backgrounds[1]->drawMode, 0xFF, message->paint.clip);
			} else {
				OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, control->bounds, 
						control->backgrounds[0]->region, control->backgrounds[0]->border, control->backgrounds[0]->drawMode, 0xFF, message->paint.clip);

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
								progressBarPellet.region, progressBarPellet.border, progressBarPellet.drawMode, 0xFF, message->paint.clip);
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
								progressBarPellet.region, progressBarPellet.border, progressBarPellet.drawMode, 0xFF, message->paint.clip);
					}
				}
			}

			control->repaint = false;
			control->repaintCustomOnly = true;
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

void *OSGetInstanceFromControl(OSObject object) {
	return ((Control *) object)->window->instance;
}

void OSGetText(OSObject _control, OSString *string) {
	Control *control = (Control *) _control;
	*string = control->text;
}

void OSSetText(OSObject _control, char *text, size_t textBytes, unsigned resizeMode) {
	Control *control = (Control *) _control;
	CreateString(text, textBytes, &control->text);

	int suggestedWidth = MeasureStringWidth(text, textBytes, FONT_SIZE, fontRegular) + 8;
	int suggestedHeight = FONT_SIZE + 8;

	if (control->icon) {
		suggestedWidth += control->icon->region.right - control->icon->region.left + ICON_TEXT_GAP;
	}

	OSMessage message;

	if (resizeMode) {
		if (resizeMode == OS_RESIZE_MODE_EXACT || suggestedWidth > control->preferredWidth) {
			control->preferredWidth = suggestedWidth;
		}

		if (resizeMode == OS_RESIZE_MODE_EXACT || suggestedHeight > control->preferredHeight) {
			control->preferredHeight = suggestedHeight;
		}

		control->relayout = true;
		message.type = OS_MESSAGE_CHILD_UPDATED;
		OSSendMessage(control->parent, &message);
	}

	OSRepaintControl(control);
	message.type = OS_MESSAGE_TEXT_UPDATED;
	OSSendMessage(control, &message);
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

static uintptr_t FindObjectInGrid(Grid *grid, OSObject object) {
	for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
		if (grid->objects[i] == object && object) {
			return i;
		}
	}

	return grid->columns * grid->rows;
}

static OSCallbackResponse ProcessGridMessage(OSObject _object, OSMessage *message) {
	OSCallbackResponse response = OS_CALLBACK_HANDLED;
	Grid *grid = (Grid *) _object;

	switch (message->type) {
		case OS_MESSAGE_SET_PROPERTY: {
			void *value = message->setProperty.value;
			int valueInt = (int) (uintptr_t) value;
			bool repaint = false;

			switch (message->setProperty.index) {
				case OS_GRID_PROPERTY_BORDER_SIZE: {
					grid->borderSize = OS_MAKE_RECTANGLE_ALL(valueInt);
					repaint = true;
				} break;

				case OS_GRID_PROPERTY_GAP_SIZE: {
					grid->gapSize = valueInt;
					repaint = true;
				} break;

				default: {
					SetGUIObjectProperty(grid, message);
				} break;
			}

			if (repaint) {
				grid->repaint = true;
				SetParentDescendentInvalidationFlags(grid, DESCENDENT_REPAINT);
			}
		} break;

		case OS_MESSAGE_LAYOUT: {
			if (grid->relayout || message->layout.force) {
				grid->relayout = false;

				grid->bounds = OS_MAKE_RECTANGLE(
						message->layout.left, message->layout.right,
						message->layout.top, message->layout.bottom);

				if (!grid->layout) grid->layout = OS_CELL_H_EXPAND | OS_CELL_V_EXPAND;
				StandardCellLayout(grid);

				tryAgain:;

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
						OSMessage message;
						message.type = OS_MESSAGE_MEASURE;
						if (OSSendMessage(*object, &message) == OS_CALLBACK_NOT_HANDLED) continue;

						int width = message.measure.preferredWidth;
						int height = message.measure.preferredHeight;

						// OSPrint("Measuring %d, %d: %d, %d, %d, %d\n", i, j, width, height, message.measure.minimumWidth, message.measure.minimumHeight);

						if ((*object)->layout & OS_CELL_H_PUSH) { bool a = grid->widths[i] == DIMENSION_PUSH; grid->widths[i] = DIMENSION_PUSH; if (!a) pushH++; }
						else if (grid->widths[i] < width && grid->widths[i] != DIMENSION_PUSH) grid->widths[i] = width;
						if ((*object)->layout & OS_CELL_V_PUSH) { bool a = grid->heights[j] == DIMENSION_PUSH; grid->heights[j] = DIMENSION_PUSH; if (!a) pushV++; }
						else if (grid->heights[j] < height && grid->heights[j] != DIMENSION_PUSH) grid->heights[j] = height;

						if (grid->minimumWidths[i] < message.measure.preferredWidth) grid->minimumWidths[i] = message.measure.preferredWidth;
						if (grid->minimumHeights[j] < message.measure.preferredHeight) grid->minimumHeights[j] = message.measure.preferredHeight;
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
					int usedWidth = grid->borderSize.left + grid->borderSize.right + grid->gapSize * (grid->columns - 1); 
					for (uintptr_t i = 0; i < grid->columns; i++) if (grid->widths[i] != DIMENSION_PUSH) usedWidth += grid->widths[i];
					int widthPerPush = (grid->bounds.right - grid->bounds.left - usedWidth) / pushH;

					for (uintptr_t i = 0; i < grid->columns; i++) {
						if (grid->widths[i] == DIMENSION_PUSH) {
							if (widthPerPush < grid->minimumWidths[i] && grid->treatPreferredDimensionsAsMinima) {
								grid->widths[i] = grid->minimumWidths[i];
							} else {
								grid->widths[i] = widthPerPush;
							}
						}
					}
				}

				if (pushV) {
					int usedHeight = grid->borderSize.top + grid->borderSize.bottom + grid->gapSize * (grid->rows - 1); 
					for (uintptr_t j = 0; j < grid->rows; j++) if (grid->heights[j] != DIMENSION_PUSH) usedHeight += grid->heights[j];
					int heightPerPush = (grid->bounds.bottom - grid->bounds.top - usedHeight) / pushV; 

					for (uintptr_t j = 0; j < grid->rows; j++) {
						if (grid->heights[j] == DIMENSION_PUSH) {
							if (heightPerPush >= grid->minimumHeights[j] || !grid->treatPreferredDimensionsAsMinima) {
								grid->heights[j] = heightPerPush;
							} else {
								grid->heights[j] = grid->minimumHeights[j];
							}
						}
					}
				}

				{
					OSMessage message;
					message.type = OS_MESSAGE_CHECK_LAYOUT;
					message.checkLayout.widths = grid->widths;
					message.checkLayout.heights = grid->heights;

					OSCallbackResponse response = OSSendMessage(grid, &message);

					if (response == OS_CALLBACK_REJECTED) {
						goto tryAgain;
					}
				}

				OSRectangle clip;
				ClipRectangle(message->layout.clip, grid->bounds, &clip);

				OSMessage message2;

				int posX = grid->bounds.left + grid->borderSize.left - grid->xOffset;

				for (uintptr_t i = 0; i < grid->columns; i++) {
					int posY = grid->bounds.top + grid->borderSize.top - grid->yOffset;

					for (uintptr_t j = 0; j < grid->rows; j++) {
						GUIObject **object = grid->objects + (j * grid->columns + i);

						message2.type = OS_MESSAGE_LAYOUT;
						message2.layout.clip = clip;
						message2.layout.force = true;
						message2.layout.left = posX;
						message2.layout.right = posX + grid->widths[i];
						message2.layout.top = posY;
						message2.layout.bottom = posY + grid->heights[j];

						OSSendMessage(*object, &message2);

						posY += grid->heights[j] + grid->gapSize;
					}

					posX += grid->widths[i] + grid->gapSize;
				}

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

			for (uintptr_t i = 0; i < grid->columns; i++) {
				for (uintptr_t j = 0; j < grid->rows; j++) {
					GUIObject **object = grid->objects + (j * grid->columns + i);
					if (OSSendMessage(*object, message) == OS_CALLBACK_NOT_HANDLED) continue;

					int width = message->measure.preferredWidth;
					int height = message->measure.preferredHeight;

					if (grid->widths[i] < width) grid->widths[i] = width;
					if (grid->heights[j] < height) grid->heights[j] = height;
				}
			}

			int width = grid->borderSize.left, height = grid->borderSize.top;

			for (uintptr_t i = 0; i < grid->columns; i++) width += grid->widths[i] + (i == grid->columns - 1 ? grid->borderSize.left : grid->gapSize);
			for (uintptr_t j = 0; j < grid->rows; j++) height += grid->heights[j] + (j == grid->rows - 1 ? grid->borderSize.top : grid->gapSize);

			if (!grid->suggestWidth) grid->preferredWidth = message->measure.preferredWidth = width;
			else message->measure.preferredWidth = grid->preferredWidth;
			if (!grid->suggestHeight) grid->preferredHeight = message->measure.preferredHeight = height;
			else message->measure.preferredHeight = grid->preferredHeight;
		} break;

		case OS_MESSAGE_PAINT: {
			if (grid->descendentInvalidationFlags & DESCENDENT_REPAINT || grid->repaint || message->paint.force) {
				grid->descendentInvalidationFlags &= ~DESCENDENT_REPAINT;

				OSMessage m = *message;
				m.paint.force = message->paint.force || grid->repaint;
				grid->repaint = false;

				OSRectangle clip;

				if (ClipRectangle(message->paint.clip, grid->bounds, &clip)) {
					if (m.paint.force) {
						if (grid->background) {
							OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, grid->bounds, grid->background->region,
									grid->background->border, grid->background->drawMode, 0xFF, clip);
						} else if (grid->backgroundColor) {
							OSFillRectangle(message->paint.surface, clip, OSColor(grid->backgroundColor));
						}
					}

					for (uintptr_t i = 0; i < grid->columns * grid->rows; i++) {
						if (grid->objects[i]) {
							if (ClipRectangle(clip, grid->objects[i]->bounds, &m.paint.clip)) {
								OSSendMessage(grid->objects[i], &m);
							}
						}
					}
				}
			}
		} break;

		case OS_MESSAGE_PAINT_BACKGROUND: {
			OSRectangle destination = OS_MAKE_RECTANGLE(message->paintBackground.left, message->paintBackground.right, 
					message->paintBackground.top, message->paintBackground.bottom);
			OSRectangle full = OS_MAKE_RECTANGLE(grid->bounds.left, grid->bounds.right, 
					grid->bounds.top, grid->bounds.bottom);
			OSRectangle clip;

			if (ClipRectangle(message->paintBackground.clip, destination, &clip)) {
				if (grid->background) {
					OSRectangle region = grid->background->region;
					OSRectangle border = grid->background->border;

					OSDrawSurfaceClipped(message->paint.surface, OS_SURFACE_UI_SHEET, full, region,
							border, grid->background->drawMode, 0xFF, clip);
				} else if (grid->backgroundColor) {
					OSFillRectangle(message->paint.surface, clip, OSColor(grid->backgroundColor));
				}
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

		case OS_MESSAGE_KEY_PRESSED: {
			response = OS_CALLBACK_NOT_HANDLED;

			if (message->keyboard.ctrl || message->keyboard.alt) {
				break;
			}

			OSObject previousFocus = message->keyboard.notHandledBy;
			int delta, start, end, i = FindObjectInGrid(grid, previousFocus);
			bool loopAround, foundFirstTime = i != (int) (grid->columns * grid->rows);

			if (message->keyboard.scancode == OS_SCANCODE_TAB) {
				delta = message->keyboard.shift ? -1 : 1;
				start = message->keyboard.shift ? grid->columns * grid->rows - 1 : 0;
				end = message->keyboard.shift ? -1 : grid->columns * grid->rows;
				loopAround = false;
			} else if (message->keyboard.scancode == OS_SCANCODE_LEFT_ARROW && foundFirstTime) {
				delta = -1;
				end = i - (i % grid->columns) - 1;
				start = end + grid->columns;
				loopAround = true;
			} else if (message->keyboard.scancode == OS_SCANCODE_UP_ARROW && foundFirstTime) {
				delta = -grid->columns;
				end = (i % grid->columns) - grid->columns;
				start = end + grid->columns * grid->rows;
				loopAround = true;
			} else if (message->keyboard.scancode == OS_SCANCODE_DOWN_ARROW && foundFirstTime) {
				delta = grid->columns;
				start = (i % grid->columns);
				end = start + grid->columns * grid->rows;
				loopAround = true;
			} else if (message->keyboard.scancode == OS_SCANCODE_RIGHT_ARROW && foundFirstTime) {
				delta = 1;
				start = i - (i % grid->columns);
				end = start + grid->columns;
				loopAround = true;
			} else {
				break;
			}

			retryTab:;
			i = FindObjectInGrid(grid, previousFocus);

			if (i == (int) (grid->columns * grid->rows)) {
				i = start;
			} else {
				i += delta;
			}

			while (i != end) {
				if (grid->objects[i] && grid->objects[i]->tabStop && !grid->objects[i]->disabled) {
					break;
				} else {
					i += delta;
				}
			}

			if (i == end && !loopAround) {
				// We're out of tab-stops in this grid.
				response = OS_CALLBACK_NOT_HANDLED;
			} else {
				if (loopAround && i == end) {
					i = start;
				}

				OSObject focus = grid->objects[i];
				response = OSSendMessage(focus, message);

				if (response == OS_CALLBACK_NOT_HANDLED) {
					previousFocus = focus;
					goto retryTab;
				}
			}
		} break;

		default: {
			response = OS_CALLBACK_NOT_HANDLED;
		} break;
	}

	return response;
}

OSObject OSCreateGrid(unsigned columns, unsigned rows, OSGridStyle style) {
	uint8_t *memory = (uint8_t *) OSHeapAllocate(sizeof(Grid) + sizeof(OSObject) * columns * rows + 2 * sizeof(int) * (columns + rows), true);

	Grid *grid = (Grid *) memory;
	grid->type = API_OBJECT_GRID;
	grid->tabStop = true;

	grid->backgroundColor = STANDARD_BACKGROUND_COLOR;
	grid->columns = columns;
	grid->rows = rows;
	grid->objects = (GUIObject **) (memory + sizeof(Grid));
	grid->widths = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows);
	grid->heights = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns);
	grid->minimumWidths = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns + sizeof(int) * rows);
	grid->minimumHeights = (int *) (memory + sizeof(Grid) + sizeof(OSObject) * columns * rows + sizeof(int) * columns + sizeof(int) * rows + sizeof(int) * columns);
	grid->style = style;

	switch (style) {
		case OS_GRID_STYLE_GROUP_BOX: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(12);
			grid->gapSize = 6;
			grid->background = &gridBox;
		} break;

		case OS_GRID_STYLE_MENU: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(4);
			grid->gapSize = 0;
			grid->background = &menuBox;
		} break;

		case OS_GRID_STYLE_MENUBAR: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(0);
			grid->gapSize = 0;
			grid->background = &menubarBackground;
		} break;

		case OS_GRID_STYLE_LAYOUT: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(0);
			grid->gapSize = 0;
			grid->backgroundColor = 0;
		} break;

		case OS_GRID_STYLE_CONTAINER: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(8);
			grid->gapSize = 6;
		} break;

		case OS_GRID_STYLE_CONTAINER_WITHOUT_BORDER: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(0);
			grid->gapSize = 6;
		} break;

		case OS_GRID_STYLE_CONTAINER_ALT: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(8);
			grid->gapSize = 6;
			grid->background = &dialogAltAreaBox;
		} break;

		case OS_GRID_STYLE_STATUS_BAR: {
			grid->borderSize = OS_MAKE_RECTANGLE_ALL(4);
			grid->gapSize = 6;
			grid->background = &dialogAltAreaBox;
		} break;

		case OS_GRID_STYLE_TOOLBAR: {
			grid->borderSize = OS_MAKE_RECTANGLE(5, 5, 0, 0);
			grid->gapSize = 4;
			grid->preferredHeight = 31;
			grid->suggestHeight = true;
			grid->background = &toolbarBackground;
		} break;

		case OS_GRID_STYLE_TOOLBAR_ALT: {
			grid->borderSize = OS_MAKE_RECTANGLE(5, 5, 0, 0);
			grid->gapSize = 4;
			grid->preferredHeight = 31;
			grid->suggestHeight = true;
			grid->background = &toolbarBackgroundAlt;
		} break;
	}

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
			int minimumWidth = m.measure.preferredWidth;
			int minimumHeight = m.measure.preferredHeight;

			m.type = OS_MESSAGE_LAYOUT;
			m.layout.force = true;
			m.layout.clip = message->layout.clip;

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

			{
				m.layout.top = message->layout.bottom - SCROLLBAR_SIZE;
				m.layout.bottom = message->layout.bottom;
				m.layout.left = message->layout.right - SCROLLBAR_SIZE;
				m.layout.right = message->layout.right;
				OSSendMessage(grid->objects[3], &m);
			}

			m.layout.top = message->layout.top;
			m.layout.bottom = message->layout.top + contentHeight;
			m.layout.left = message->layout.left;
			m.layout.right = message->layout.left + contentWidth;
			OSSendMessage(grid->objects[0], &m);

			response = OS_CALLBACK_HANDLED;
		}
	} else if (message->type == OS_MESSAGE_MEASURE) {
		message->measure.preferredWidth = SCROLLBAR_SIZE * 3;
		message->measure.preferredHeight = SCROLLBAR_SIZE * 3;
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
	OSObject grid = OSCreateGrid(2, 2, OS_GRID_STYLE_LAYOUT);
	OSSetCallback(grid, OS_MAKE_CALLBACK(ProcessScrollPaneMessage, nullptr));

	OSAddGrid(grid, 0, 0, content, OS_CELL_FILL);
	((Grid *) content)->treatPreferredDimensionsAsMinima = true;

	if (flags & OS_CREATE_SCROLL_PANE_VERTICAL) {
		OSObject scrollbar = OSCreateScrollbar(OS_ORIENTATION_VERTICAL);
		OSAddGrid(grid, 1, 0, scrollbar, OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
		OSSetObjectNotificationCallback(scrollbar, OS_MAKE_CALLBACK(ScrollPaneBarMoved, content));
		// OSPrint("vertical %x\n", scrollbar);
	}

	if (flags & OS_CREATE_SCROLL_PANE_HORIZONTAL) {
		OSObject scrollbar = OSCreateScrollbar(OS_ORIENTATION_HORIZONTAL);
		OSAddGrid(grid, 0, 1, scrollbar, OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		OSSetObjectNotificationCallback(scrollbar, OS_MAKE_CALLBACK(ScrollPaneBarMoved, content));
		// OSPrint("horizontal %x\n", scrollbar);
	}

	{
		OSObject corner = OSCreateGrid(1, 1, OS_GRID_STYLE_CONTAINER);
		OSAddGrid(grid, 1, 1, corner, OS_CELL_H_EXPAND | OS_CELL_V_EXPAND);
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
		scrollbar->position = (fraction * (float) scrollbar->maxPosition);

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
					OS_DRAW_MODE_REPEAT_FIRST, 0xFF, message->paint.clip);
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
			result = OS_CALLBACK_HANDLED;
		} break;

		case OS_MESSAGE_LAYOUT: {
			if (grid->relayout || message->layout.force) {
				grid->relayout = false;

				grid->bounds = OS_MAKE_RECTANGLE(
						message->layout.left, message->layout.right,
						message->layout.top, message->layout.bottom);

				OSRectangle clip = message->layout.clip;

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
					message.layout.clip = clip;

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
		scrollbar->position = 0;

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

		if (scrollbar->size < SCROLLBAR_MINIMUM) scrollbar->size = SCROLLBAR_MINIMUM;

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
	scrollbar->preferredWidth = !orientation ? 64 : SCROLLBAR_SIZE;
	scrollbar->preferredHeight = !orientation ? SCROLLBAR_SIZE : 64;

	OSCommand command = {};
	command.defaultDisabled = true;
	command.identifier = OS_COMMAND_DYNAMIC;

	Control *nudgeUp = (Control *) OSHeapAllocate(sizeof(Control), true);
	nudgeUp->type = API_OBJECT_CONTROL;
	nudgeUp->context = scrollbar;
	nudgeUp->notificationCallback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_NUDGE_UP);
	nudgeUp->backgrounds = orientation ? scrollbarTrackVerticalBackgrounds : scrollbarTrackHorizontalBackgrounds;
	OSSetCallback(nudgeUp, OS_MAKE_CALLBACK(ProcessButtonMessage, nullptr));

	command.callback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_NUDGE_DOWN);
	Control *nudgeDown = (Control *) OSHeapAllocate(sizeof(Control), true);
	nudgeDown->type = API_OBJECT_CONTROL;
	nudgeDown->context = scrollbar;
	nudgeDown->notificationCallback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_NUDGE_DOWN);
	nudgeDown->backgrounds = orientation ? scrollbarTrackVerticalBackgrounds : scrollbarTrackHorizontalBackgrounds;
	OSSetCallback(nudgeDown, OS_MAKE_CALLBACK(ProcessButtonMessage, nullptr));

	command.callback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_BUTTON_UP);
	Control *up = (Control *) OSCreateButton(&command, OS_BUTTON_STYLE_NORMAL);
	up->backgrounds = scrollbarButtonHorizontalBackgrounds;
	up->context = scrollbar;
	up->icon = orientation ? &smallArrowUpNormal : &smallArrowLeftNormal;
	up->iconHasVariants = true;
	up->centerIcons = true;

	Control *grip = (Control *) OSHeapAllocate(sizeof(Control), true);
	grip->type = API_OBJECT_CONTROL;
	grip->context = scrollbar;
	grip->backgrounds = orientation ? scrollbarButtonVerticalBackgrounds : scrollbarButtonHorizontalBackgrounds;
	OSSetCallback(grip, OS_MAKE_CALLBACK(ProcessScrollbarGripMessage, nullptr));

	command.callback = OS_MAKE_CALLBACK(ScrollbarButtonPressed, SCROLLBAR_BUTTON_DOWN);
	Control *down = (Control *) OSCreateButton(&command, OS_BUTTON_STYLE_NORMAL);
	down->backgrounds = scrollbarButtonHorizontalBackgrounds;
	down->context = scrollbar;
	down->icon = orientation ? &smallArrowDownNormal : &smallArrowRightNormal;
	down->iconHasVariants = true;
	down->centerIcons = true;

	OSAddControl(scrollbar, 0, 0, up, OS_CELL_H_EXPAND);
	OSAddControl(scrollbar, 0, 1, grip, OS_CELL_V_EXPAND | OS_CELL_H_EXPAND);
	OSAddControl(scrollbar, 0, 2, down, OS_CELL_H_EXPAND);
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

void OSCheckCommand(OSObject _window, OSCommand *_command, bool checked) {
	Window *window = (Window *) _window;

	CommandWindow *command = window->commands + _command->identifier;
	command->checked = checked;
	LinkedItem<Control> *item = command->controls.firstItem;

	while (item) {
		item->thisItem->isChecked = checked;
		OSRepaintControl(item->thisItem);
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
			{
				OSMessage message = {};
				message.type = OS_NOTIFICATION_COMMAND;
				message.command.window = window;
				message.command.command = osCommandDestroyWindow;
				int32_t identifier = osCommandDestroyWindow->identifier;
				OSForwardMessage(nullptr, window->commands[identifier].notificationCallback, &message);
			}

			if (!window->hasMenuParent) {
				OSHeapFree(window->commands);
			}

			OSHeapFree(window);
			return response;
		} break;

		case OS_MESSAGE_KEY_RELEASED: {
			GUIObject *control = window->lastFocus;
			OSSendMessage(control, message);
		} break;

		case OS_MESSAGE_KEY_PRESSED: {
			if (message->keyboard.scancode == OS_SCANCODE_F4 && message->keyboard.alt) {
				message->type = OS_MESSAGE_DESTROY;
				OSSendMessage(window, message);
			} else if (message->keyboard.scancode == OS_SCANCODE_F2 && message->keyboard.alt) {
				EnterDebugger();
			} else {
				OSCallbackResponse response = OS_CALLBACK_NOT_HANDLED;

				if (window->focus) {
					message->type = OS_MESSAGE_KEY_TYPED;
					response = OSSendMessage(window->focus, message);
				}

				GUIObject *control = window->lastFocus;
				message->keyboard.notHandledBy = nullptr;
				message->type = OS_MESSAGE_KEY_PRESSED;

				while (control && control != window && response == OS_CALLBACK_NOT_HANDLED) {
					response = OSSendMessage(control, message);
					message->keyboard.notHandledBy = control;
					control = (GUIObject *) control->parent;
				}

				if (response == OS_CALLBACK_NOT_HANDLED) {
					if (message->keyboard.scancode == OS_SCANCODE_TAB 
							&& !message->keyboard.ctrl && !message->keyboard.alt) {
						message->keyboard.notHandledBy = nullptr;
						OSSendMessage(window->root, message);
					}

					// TODO Keyboard shortcuts and access keys.
				}
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
					OSSetFocusedControl(control);
				}
			}
		} break;

		case OS_MESSAGE_MOUSE_LEFT_RELEASED: {
			if (window->pressed) {
				// Send the raw message.
				OSSendMessage(window->pressed, message);

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

		OSMessage message;
		message.type = OS_MESSAGE_LAYOUT;
		message.layout.clip = OS_MAKE_RECTANGLE(0, window->width, 0, window->height);
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

		OSMessage message;
		message.type = OS_MESSAGE_PAINT;
		message.paint.clip = OS_MAKE_RECTANGLE(0, window->width, 0, window->height);
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

static Window *CreateWindow(OSWindowSpecification *specification, Window *menuParent, unsigned x = 0, unsigned y = 0, Window *modalParent = nullptr) {
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

	window->window = modalParent ? modalParent->window : OS_INVALID_HANDLE;
	OSSyscall(OS_SYSCALL_CREATE_WINDOW, (uintptr_t) &window->window, (uintptr_t) &bounds, (uintptr_t) window, menuParent ? (uintptr_t) menuParent->window : 0);
	OSSyscall(OS_SYSCALL_GET_WINDOW_BOUNDS, window->window, (uintptr_t) &bounds, 0, 0);

	window->width = bounds.right - bounds.left;
	window->height = bounds.bottom - bounds.top;
	window->flags = flags;
	window->cursor = OS_CURSOR_NORMAL;
	window->minimumWidth = specification->minimumWidth;
	window->minimumHeight = specification->minimumHeight;

	if (!menuParent) {
		window->commands = (CommandWindow *) OSHeapAllocate(sizeof(CommandWindow) * _commandCount, true);

		for (uintptr_t i = 0; i < _commandCount; i++) {
			CommandWindow *command = window->commands + i;
			command->disabled = _commands[i]->defaultDisabled;
			command->checked = _commands[i]->defaultCheck;
			command->notificationCallback = _commands[i]->callback;
		}
	} else {
		window->commands = menuParent->commands;
		window->hasMenuParent = true;
	}

	OSSetCallback(window, OS_MAKE_CALLBACK(ProcessWindowMessage, nullptr));

	window->root = (Grid *) OSCreateGrid(3, 4, OS_GRID_STYLE_LAYOUT);
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
		OSSetText(titlebar, specification->title, specification->titleBytes, OS_RESIZE_MODE_IGNORE);

		if (flags & OS_CREATE_WINDOW_DIALOG) {
			OSAddControl(window->root, 0, 0, CreateWindowResizeHandle(dialogBorder11, RESIZE_MOVE), 0);
			OSAddControl(window->root, 1, 0, CreateWindowResizeHandle(dialogBorder12, RESIZE_MOVE), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
			OSAddControl(window->root, 2, 0, CreateWindowResizeHandle(dialogBorder13, RESIZE_MOVE), 0);
			OSAddControl(window->root, 0, 1, CreateWindowResizeHandle(dialogBorder21, RESIZE_MOVE), 0);
			OSAddControl(window->root, 2, 1, CreateWindowResizeHandle(dialogBorder23, RESIZE_MOVE), 0);
			OSAddControl(window->root, 0, 2, CreateWindowResizeHandle(dialogBorder31, RESIZE_NONE), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
			OSAddControl(window->root, 2, 2, CreateWindowResizeHandle(dialogBorder33, RESIZE_NONE), OS_CELL_V_PUSH | OS_CELL_V_EXPAND);
			OSAddControl(window->root, 0, 3, CreateWindowResizeHandle(dialogBorder41, RESIZE_NONE), 0);
			OSAddControl(window->root, 1, 3, CreateWindowResizeHandle(dialogBorder42, RESIZE_NONE), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
			OSAddControl(window->root, 2, 3, CreateWindowResizeHandle(dialogBorder43, RESIZE_NONE), 0);
		} else {
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
		}

		if (flags & OS_CREATE_WINDOW_WITH_MENUBAR) {
			OSObject grid = OSCreateGrid(1, 2, OS_GRID_STYLE_LAYOUT);
			OSAddGrid(window->root, 1, 2, grid, OS_CELL_FILL);
			OSAddGrid(grid, 0, 0, OSCreateMenu(specification->menubar, nullptr, OS_MAKE_POINT(0, 0), OS_CREATE_MENUBAR), OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
		}
	}

	return window;
}

OSObject OSCreateWindow(OSWindowSpecification *specification) {
	return CreateWindow(specification, nullptr);
}

static OSCallbackResponse CommandDialogAlertOK(OSObject object, OSMessage *message) {
	(void) object;

	if (message->type == OS_NOTIFICATION_COMMAND) {
		OSMessage m;
		m.type = OS_MESSAGE_DESTROY;
		OSSendMessage(message->context, &m);
		return OS_CALLBACK_HANDLED;
	}

	return OS_CALLBACK_NOT_HANDLED;
}

void OSShowDialogAlert(char *title, size_t titleBytes,
				   char *message, size_t messageBytes,
				   char *description, size_t descriptionBytes,
				   uint16_t iconID, OSObject modalParent) {
	OSWindowSpecification specification = *osDialogStandard;
	specification.title = title;
	specification.titleBytes = titleBytes;

	OSObject dialog = CreateWindow(&specification, nullptr, 0, 0, (Window *) modalParent);

	OSObject layout1 = OSCreateGrid(1, 2, OS_GRID_STYLE_LAYOUT);
	OSObject layout2 = OSCreateGrid(3, 1, OS_GRID_STYLE_CONTAINER);
	OSObject layout3 = OSCreateGrid(1, 2, OS_GRID_STYLE_CONTAINER_WITHOUT_BORDER);
	OSObject layout4 = OSCreateGrid(1, 1, OS_GRID_STYLE_CONTAINER_ALT);

	OSSetRootGrid(dialog, layout1);
	OSAddGrid(layout1, 0, 0, layout2, OS_CELL_FILL);
	OSAddGrid(layout2, 2, 0, layout3, OS_CELL_FILL);
	OSAddGrid(layout1, 0, 1, layout4, OS_CELL_H_EXPAND);

	OSObject okButton = OSCreateButton(osDialogStandardOK, OS_BUTTON_STYLE_NORMAL);
	OSAddControl(layout4, 0, 0, okButton, OS_CELL_H_PUSH | OS_CELL_H_RIGHT);
	OSSetCommandNotificationCallback(dialog, osDialogStandardOK, OS_MAKE_CALLBACK(CommandDialogAlertOK, dialog));

	Control *label = (Control *) OSCreateLabel(message, messageBytes);
	label->textSize = 10;
	label->textColor = TEXT_COLOR_HEADING;
	OSAddControl(layout3, 0, 0, label, OS_CELL_H_EXPAND | OS_CELL_H_PUSH);

	OSAddControl(layout3, 0, 1, OSCreateLabel(description, descriptionBytes), OS_CELL_H_EXPAND | OS_CELL_H_PUSH);
	OSAddControl(layout2, 0, 0, OSCreateIconDisplay(iconID), OS_CELL_V_TOP);
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


	OSObject grid = OSCreateGrid(menubar ? itemCount : 1, !menubar ? itemCount : 1, menubar ? OS_GRID_STYLE_MENUBAR : OS_GRID_STYLE_MENU);

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

void OSSetProperty(OSObject object, uintptr_t index, void *value) {
	OSMessage message;
	message.type = OS_MESSAGE_SET_PROPERTY;
	message.setProperty.index = index;
	message.setProperty.value = value;
	OSSendMessage(object, &message);
}

void OSInitialiseGUI() {
	OSLinearBuffer buffer;
	OSGetLinearBuffer(OS_SURFACE_UI_SHEET, &buffer);

	uint32_t *skin = (uint32_t *) OSMapObject(buffer.handle, 0, buffer.width * buffer.height * 4, OS_FLAGS_DEFAULT);

	STANDARD_BACKGROUND_COLOR = skin[0 * 3 + 25 * buffer.width];
	
	LIST_VIEW_COLUMN_TEXT_COLOR = skin[1 * 3 + 25 * buffer.width];
	LIST_VIEW_PRIMARY_TEXT_COLOR = skin[2 * 3 + 25 * buffer.width];
	LIST_VIEW_SECONDARY_TEXT_COLOR = skin[3 * 3 + 25 * buffer.width];
	LIST_VIEW_BACKGROUND_COLOR = skin[4 * 3 + 25 * buffer.width];
	
	TEXT_COLOR_DEFAULT = skin[5 * 3 + 25 * buffer.width];
	// TEXT_COLOR_DISABLED = skin[6 * 3 + 25 * buffer.width];
	TEXT_COLOR_DISABLED_SHADOW = skin[7 * 3 + 25 * buffer.width];
	TEXT_COLOR_HEADING = skin[8 * 3 + 25 * buffer.width];
	TEXT_COLOR_TITLEBAR = skin[0 * 3 + 28 * buffer.width];
	TEXT_COLOR_TOOLBAR = skin[1 * 3 + 28 * buffer.width];
	
	TEXTBOX_SELECTED_COLOR_1 = skin[2 * 3 + 28 * buffer.width];
	TEXTBOX_SELECTED_COLOR_2 = skin[3 * 3 + 28 * buffer.width];

	DISABLE_TEXT_SHADOWS = skin[4 * 3 + 28 * buffer.width];

	OSFree(skin);
	OSCloseHandle(buffer.handle);
}
