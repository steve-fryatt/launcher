/* Launcher - choices.h
 * (c) Stephen Fryatt, 2012
 *
 */

#ifndef LAUNCHER_CHOICES
#define LAUNCHER_CHOICES


/**
 * Initialise the Choices module.
 */

void choices_initialise(void);


/**
 * Open the Choices window at the mouse pointer.
 *
 * \param *pointer		The details of the pointer to open the window at.
 */

void choices_open_window(wimp_pointer *pointer);


/**
 * Identify whether the Choices window is currently open.
 *
 * \return		TRUE if the window is open; else FALSE.
 */

osbool choices_window_is_open(void);

#endif

