/* Launcher - choices.c
 * (c) Stephen Fryatt, 2012
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */


/* OSLib Header files. */

#include "oslib/osbyte.h"
#include "oslib/wimp.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/string.h"

/* Application header files. */

#include "choices.h"

#include "ihelp.h"
#include "templates.h"


/* Choices window icons. */

#define CHOICE_ICON_APPLY 0
#define CHOICE_ICON_SAVE 1
#define CHOICE_ICON_CANCEL 2
#define CHOICE_ICON_COLUMNS 6
#define CHOICE_ICON_COLUMNS_DOWN 7
#define CHOICE_ICON_COLUMNS_UP 8
#define CHOICE_ICON_SIZE 10
#define CHOICE_ICON_SIZE_DOWN 11
#define CHOICE_ICON_SIZE_UP 12
#define CHOICE_ICON_SPACING 15
#define CHOICE_ICON_SPACING_DOWN 16
#define CHOICE_ICON_SPACING_UP 17
#define CHOICE_ICON_CONFIRM_DELETE 19


/* Global variables */

static wimp_w		choices_window = NULL;

static void	choices_close_window(void);
static void	choices_set_window(void);
static void	choices_read_window(void);
static void	choices_redraw_window(void);

static void	choices_click_handler(wimp_pointer *pointer);
static osbool	choices_keypress_handler(wimp_key *key);


/**
 * Initialise the Choices module.
 */

void choices_initialise(void)
{
	choices_window = templates_create_window("Choices");
	ihelp_add_window(choices_window, "Choices", NULL);

	event_add_window_mouse_event(choices_window, choices_click_handler);
	event_add_window_key_event(choices_window, choices_keypress_handler);
}


/**
 * Open the Choices window at the mouse pointer.
 *
 * \param *pointer		The details of the pointer to open the window at.
 */

void choices_open_window(wimp_pointer *pointer)
{
	if (windows_get_open(choices_window))
		return;

	choices_set_window();

	windows_open_centred_at_pointer(choices_window, pointer);
}


/**
 * Close the choices window.
 */

static void choices_close_window(void)
{
	wimp_close_window(choices_window);
}


/**
 * Set the contents of the Choices window to reflect the current settings.
 */

static void choices_set_window(void)
{
	icons_printf(choices_window, CHOICE_ICON_COLUMNS, "%d", config_int_read("WindowColumns"));
	icons_printf(choices_window, CHOICE_ICON_SIZE, "%d", config_int_read("GridSize"));
	icons_printf(choices_window, CHOICE_ICON_SPACING, "%d", config_int_read("GridSpacing"));

	icons_set_selected(choices_window, CHOICE_ICON_CONFIRM_DELETE, config_opt_read("ConfirmDelete"));
}


/**
 * Update the configuration settings from the values in the Choices window.
 */

static void choices_read_window(void)
{
	config_int_set("WindowColumns", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_COLUMNS)));
	config_int_set("GridSize", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SIZE)));
	config_int_set("GridSpacing", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SPACING)));

	config_opt_set("ConfirmDelete", icons_get_selected(choices_window, CHOICE_ICON_CONFIRM_DELETE));
}


/**
 * Refresh the Choices dialogue, to reflech changed icon states.
 */

static void choices_redraw_window(void)
{
	wimp_set_icon_state(choices_window, CHOICE_ICON_COLUMNS, 0, 0);
	wimp_set_icon_state(choices_window, CHOICE_ICON_SIZE, 0, 0);
	wimp_set_icon_state(choices_window, CHOICE_ICON_SPACING, 0, 0);
}


/**
 * Process mouse clicks in the Choices dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void choices_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch ((int) pointer->i) {
	case CHOICE_ICON_APPLY:
		if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
			choices_read_window();

			if (pointer->buttons == wimp_CLICK_SELECT)
				choices_close_window();
		}
		break;

	case CHOICE_ICON_SAVE:
		if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
			choices_read_window();
			config_save();

			if (pointer->buttons == wimp_CLICK_SELECT)
				choices_close_window();
		}
		break;

	case CHOICE_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			choices_close_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			choices_set_window();
			choices_redraw_window();
		}
		break;
	}
}


/**
 * Process keypresses in the Choices window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool choices_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		choices_read_window();
		config_save();
		choices_close_window();
		break;

	case wimp_KEY_ESCAPE:
		choices_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

