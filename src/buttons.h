/* Launcher - buttons.h
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
 */

#ifndef LAUNCHER_BUTTONS
#define LAUNCHER_BUTTONS

/* ================================================================================================================== */

#define LAUNCHER_TOP    1
#define LAUNCHER_BOTTOM 2
#define LAUNCHER_LEFT   3
#define LAUNCHER_RIGHT  4

#define BUTTON_GUTTER 4

/* ================================================================================================================== */

/* Structure to hold details of a button. */


/* ================================================================================================================== */

/* Launch functions. */

/**
 * Initialise the buttons window.
 */

void buttons_initialise(void);





int create_button_icon (button *button_def);

void toggle_launch_window (void);
void open_launch_window (int columns, wimp_w window_level);

int fill_edit_button_window (wimp_i icon);
int open_edit_button_window (wimp_pointer *pointer);
int redraw_edit_button_window (void);
int read_edit_button_window (button *button_def);
int close_edit_button_window (void);

int press_button (wimp_i icon);

#endif

