
#ifndef _CONFIG_H_
#define _CONFIG_H_

// ============================
// Configuration header for wmx
// ============================
// 
// To configure: change the relevant bit of the file, and rebuild the
// manager.  Make sure all necessary source files are built (by
// running "make depend" before you start).
// 
// This file is in four sections.  (This might imply that it's getting
// too long.)  The sections are:
// 
// I.   Straightforward operational parameters
// II.  Key bindings
// III. Colours and fonts
// IV.  Flashy stuff: channels, pixmaps, skeletal feedback &c.
// 
// All timing values are in milliseconds, but accuracy depends on the
// minimum timer value available to select, so they should be taken
// with a pinch of salt.  On the machine I'm using now, I mentally
// double all the given values.
// 
// -- Chris Cannam, January 1998


// =================================================
// Section I. Straightforward operational parameters
// =================================================

// List visible as well as hidden clients on the root menu?  (Visible
// ones will be towards the bottom of the menu, flush-right.)
#define CONFIG_EVERYTHING_ON_ROOT_MENU 1

// Spawn a temporary new shell between the wm and each new process?
#define CONFIG_EXEC_USING_SHELL   True

// What to run to get a new window (from the "New" menu option)
#define CONFIG_NEW_WINDOW_LABEL "New"
#define CONFIG_NEW_WINDOW_COMMAND "x-terminal-emulator"
#define CONFIG_NEW_WINDOW_COMMAND_OPTIONS 0
// or, for example,
//#define CONFIG_NEW_WINDOW_COMMAND_OPTIONS "-ls","-sb","-sl","1024",0
// alternatively,
#define CONFIG_DISABLE_NEW_WINDOW_COMMAND 0

// Area where [exit wmx] is added (0 -> everywhere -# -> px from other side)
#define CONFIG_EXIT_CLICK_SIZE_X 0
#define CONFIG_EXIT_CLICK_SIZE_Y -3
 
// Directory under $HOME in which to look for commands for the
// middle-button menu
#define CONFIG_COMMAND_MENU       ".wmx"
// only used if COMMAND_MENU is not found; ignored if invalid directory:
#define CONFIG_SYSTEM_COMMAND_MENU      "/usr/local/lib/wmx/menu"
// append screennumber to COMMAND_MENU directory;
// use non screen style as fallback
#define CONFIG_ADD_SCREEN_TO_COMMAND_MENU False
 
// Focus possibilities.
// 
// You can't have CLICK_TO_FOCUS without RAISE_ON_FOCUS, but the other
// combinations should be okay.  If you set AUTO_RAISE you must leave
// the other two False; you'll then get focus-follows, auto-raise, and
// a delay on auto-raise as configured in the DELAY settings below.

#define CONFIG_CLICK_TO_FOCUS     0
#define CONFIG_RAISE_ON_FOCUS     0
#define CONFIG_AUTO_RAISE         0

#define CONFIG_PASS_FOCUS_CLICK   1

// Delays when using AUTO_RAISE focus method
// 
// In theory these only apply when using AUTO_RAISE, not when just
// using RAISE_ON_FOCUS without CLICK_TO_FOCUS.  First of these is the
// usual delay before raising; second is the delay after the pointer
// has stopped moving (only when over simple X windows such as xvt).

#define CONFIG_AUTO_RAISE_DELAY       400
#define CONFIG_POINTER_STOPPED_DELAY  80
#define CONFIG_DESTROY_WINDOW_DELAY   600

// Number of pixels off the screen you have to push a window
// before the manager notices the window is off-screen (the higher
// the value, the easier it is to place windows at the screen edges)

#define CONFIG_BUMP_DISTANCE      16

// If CONFIG_BUMP_EVERYWHERE is defined, windows will "bump" against
// other window edges as well as the edges of the screen

#define CONFIG_BUMP_EVERYWHERE    True

// If CONFIG_PROD_SHAPE is True, all frame element shapes will be
// recalculated afresh every time their focus changes.  This will
// probably slow things down hideously, but has been reported as
// necessary on some systems (possibly SunOS 4.x with OpenWindows).

#define CONFIG_PROD_SHAPE         False

// If RESIZE_UPDATE is True, windows will opaque-resize "correctly";
// if False, behaviour will be as in wm2 (stretching the background
// image only).

#define CONFIG_RESIZE_UPDATE      True

// If USE_COMPOSITE is true, wmx will enable composite redirects for
// all windows if the Composite extension is present.  This should
// make no difference at all to the appearance or behaviour of wmx,
// but it may make it substantially faster with modern video cards
// that optimise rendering more than old-fashioned window operations.

#define CONFIG_USE_COMPOSITE      True

// If RAISELOWER_ON_CLICK is True, clicking on the title of the
// topmost window will lower instead of raising it (patch due to
// Kazushi (Jam) Marukawa)

#define CONFIG_RAISELOWER_ON_CLICK      False

// Specify the maximum length of an entry in the client menu or the command
// menu. Set this to zero if you want no limitation

#define MENU_ENTRY_MAXLENGTH            80


// ========================
// Section II. Key bindings
// ========================

// Allow keyboard control?
#define CONFIG_USE_KEYBOARD       1

// This is the key for wm controls: e.g. Alt/Left and Alt/Right to
// flip channels, and Alt/Tab to switch windows.  (On my 105-key
// PC keyboard, Meta_L corresponds to the left Windows key.)

#define CONFIG_ALT_KEY            XK_Super_L

// And these define the rest of the keyboard controls, when the above
// modifier is pressed; they're keysyms as defined in <X11/keysym.h>
// and <X11/keysymdef.h>

#define CONFIG_FLIP_UP_KEY        XK_Right
#define CONFIG_FLIP_DOWN_KEY      XK_Left
#define CONFIG_HIDE_KEY           XK_Return
#define CONFIG_STICKY_KEY         XK_Pause
#define CONFIG_RAISE_KEY          XK_Up
#define CONFIG_LOWER_KEY          XK_Down
// Prior and Next should be the same as Page_Up and Page_Down in R6
#define CONFIG_FULLHEIGHT_KEY     XK_Prior
#define CONFIG_NORMALHEIGHT_KEY   XK_Next
#define CONFIG_FULLWIDTH_KEY      XK_KP_Add
#define CONFIG_NORMALWIDTH_KEY    XK_KP_Subtract
#define CONFIG_MAXIMISE_KEY       XK_Home
#define CONFIG_UNMAXIMISE_KEY     XK_End
#define CONFIG_SAME_KEY_MAX_UNMAX False

// With modifier, print a list of client data to stdout
#define CONFIG_DEBUG_KEY          XK_Print

// The next two may clash badly with Emacs, if you use Alt as the
// modifier.  The commented variants might work better for some.
#define CONFIG_CIRCULATE_KEY    XK_Tab
//#define CONFIG_CIRCULATE_KEY  XK_grave
//#define CONFIG_CIRCULATE_KEY  XK_section
#define CONFIG_DESTROY_KEY      XK_BackSpace
//#define CONFIG_DESTROY_KEY    XK_Delete
//#define CONFIG_DESTROY_KEY    XK_Insert

// If WANT_KEYBOARD_MENU is True, then the MENU_KEY, when pressed with
// the modifier, will call up a client menu with keyboard navigation
#define CONFIG_WANT_KEYBOARD_MENU       True
#define CONFIG_CLIENT_MENU_KEY          XK_Menu
#define CONFIG_COMMAND_MENU_KEY         XK_Multi_key
#define CONFIG_EXIT_ON_KBD_MENU         True
// these are for navigating on the menu; they don't require a modifier
#define CONFIG_MENU_UP_KEY      XK_Up
#define CONFIG_MENU_DOWN_KEY    XK_Down
#define CONFIG_MENU_SELECT_KEY  XK_Return
#define CONFIG_MENU_CANCEL_KEY  XK_Escape

// Mouse Configuration
// Use this section to remap your mouse button actions.
//   Button1 = LMB, Button2 = MMB, Button3 = RMB 
//   Button4 = WheelUp, Button5 = WheelDown 
// To prevent one or more of these from being supported
// at all, define it to CONFIG_NO_BUTTON.
#define CONFIG_NO_BUTTON 999
#define CONFIG_CLIENTMENU_BUTTON  Button1
#define CONFIG_COMMANDMENU_BUTTON Button3
#define CONFIG_CIRCULATE_BUTTON   999 // switch window, when over frame
#define CONFIG_PREVCHANNEL_BUTTON 999 // flip channel, when over frame
#define CONFIG_NEXTCHANNEL_BUTTON 999 // flip channel, when over frame

#define CONFIG_RIGHT_CIRCULATE       True
#define CONFIG_RIGHT_LOWER           False
#define CONFIG_RIGHT_TOGGLE_HEIGHT   False


// ==============================
// Section III. Colours and fonts
// ==============================

// Use Xft for fonts and cursors.

// Fonts used all over the place.  FRAME_FONT is a comma-separated
// list of font names to use for the frame text (the first available
// font in the list is used).  MENU_FONT is likewise for menu text.
// The SIZE values are in pixels.

//!!! no proper way to handle italic/bold yet

#define CONFIG_FRAME_FONT "DejaVu Sans"
#define CONFIG_FRAME_FONT_SIZE 12

#define CONFIG_MENU_FONT "DejaVu Sans"
#define CONFIG_MENU_FONT_SIZE 12

// CONFIG_TAB_MARGIN defines the size of the gap on the left and right of the
// text in the tab.

#define CONFIG_TAB_MARGIN   2

// Colours for window decorations.  The BORDERS one is for the
// one-pixel border around the edge of each piece of decoration, not
// for the whole decoration

#define CONFIG_TAB_FOREGROUND     "black"
#define CONFIG_TAB_BACKGROUND     "gray80"
#define CONFIG_FRAME_BACKGROUND   "gray95"
#define CONFIG_BUTTON_BACKGROUND  "gray95"
#define CONFIG_BORDERS            "black"
#define CONFIG_CHANNEL_NUMBER     "green"

#define CONFIG_MENU_FOREGROUND    "black"
#define CONFIG_MENU_BACKGROUND    "gray80"
#define CONFIG_MENU_BORDERS       "black"

// Pixel width for the bit of frame to the left of the window and the
// sum of the two bits at the top

#define CONFIG_FRAME_THICKNESS    8


// ========================
// Section IV. Flashy stuff
// ========================

// Set CHANNEL_SURF for multi-channel switching; CHANNEL_CLICK_SIZE is
// how close you have to middle-button-click to the top-right corner
// of the root window before the channel change happens.  Set
// USE_CHANNEL_KEYS if you want Alt-F1, Alt-F2 etc for quick channel
// changes, provided USE_KEYBOARD is also True.  Set USE_CHANNEL_MENU
// if you want to change channels via a keyboard-controlled menu
// instead of linearly up and down one at a time like TV.

#define CONFIG_CHANNEL_SURF       False
#define CONFIG_CHANNEL_CLICK_SIZE 120
#define CONFIG_CHANNEL_NUMBER_SIZE 20
#define CONFIG_USE_CHANNEL_KEYS   False
#define CONFIG_USE_CHANNEL_MENU   False

// FLIP_DELAY is the length of time the big green number stays in the
// top-right when flipping channels, before the windows reappear.
// QUICK_FLIP_DELAY is the equivalent figure used when flipping with
// the Alt-Fn keys or mouse wheel.  Milliseconds.

#define CONFIG_FLIP_DELAY         1000

// Set MAD_FEEDBACK for skeletal representations of windows when
// flicking through the client menu and changing channels.  The DELAY
// is how long skeletal feedback has to persist before wmx decides to
// post the entire window contents instead; if it's negative, the
// contents will never be posted.  Experiment with these -- if your
// machine is fast and you're really hyper, you might even like a
// delay of 0ms.

#define CONFIG_MAD_FEEDBACK       1
#define CONFIG_FEEDBACK_DELAY     300

// Position of the geometry window:
// X < 0 left, X > 0 right,  X = 0 center
// Y < 0 top,  Y > 0 bottom, Y = 0 center
#define CONFIG_GEOMETRY_X_POS     0
#define CONFIG_GEOMETRY_Y_POS     0

#endif

