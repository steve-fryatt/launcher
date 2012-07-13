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


/**
 * Terminate the buttons window.
 */

void buttons_terminate(void);


/**
 * Create a full set of buttons from the contents of the application database.
 */

void buttons_create_from_db(void);

#endif

