/* Copyright 2012-2020, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Launcher:
 *
 *   http://www.stevefryatt.org.uk/risc-os
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: choices.c
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
#include "sflib/ihelp.h"
#include "sflib/windows.h"
#include "sflib/string.h"
#include "sflib/templates.h"

/* Application header files. */

#include "choices.h"

#include "panel.h"


/* Choices window icons. */

#define CHOICE_ICON_APPLY 0
#define CHOICE_ICON_SAVE 1
#define CHOICE_ICON_CANCEL 2
#define CHOICE_ICON_SIZE 6
#define CHOICE_ICON_SIZE_DOWN 7
#define CHOICE_ICON_SIZE_UP 8
#define CHOICE_ICON_SPACING 11
#define CHOICE_ICON_SPACING_DOWN 12
#define CHOICE_ICON_SPACING_UP 13
#define CHOICE_ICON_SIDEBAR 16
#define CHOICE_ICON_SIDEBAR_DOWN 17
#define CHOICE_ICON_SIDEBAR_UP 18
#define CHOICE_ICON_MOUSE_OVER 22
#define CHOICE_ICON_DELAY 24
#define CHOICE_ICON_DELAY_DOWN 25
#define CHOICE_ICON_DELAY_UP 26
#define CHOICE_ICON_CONFIRM_DELETE 28

/* Bump field limits and step values. */

#define CHOICE_SIZE_MIN 20
#define CHOICE_SIZE_MAX 100
#define CHOICE_SIZE_STEP 2

#define CHOICE_SPACING_MIN 0
#define CHOICE_SPACING_MAX 10
#define CHOICE_SPACING_STEP 2

#define CHOICE_DELAY_MIN 10
#define CHOICE_DELAY_MAX 999
#define CHOICE_DELAY_STEP 1

#define CHOICE_SIDEBAR_MIN 2
#define CHOICE_SIDEBAR_MAX 30
#define CHOICE_SIDEBAR_STEP 2

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

	event_add_window_icon_bump(choices_window, CHOICE_ICON_SIZE,
			CHOICE_ICON_SIZE_UP, CHOICE_ICON_SIZE_DOWN,
			CHOICE_SIZE_MIN, CHOICE_SIZE_MAX, CHOICE_SIZE_STEP);
	event_add_window_icon_bump(choices_window, CHOICE_ICON_SPACING,
			CHOICE_ICON_SPACING_UP, CHOICE_ICON_SPACING_DOWN,
			CHOICE_SPACING_MIN, CHOICE_SPACING_MAX, CHOICE_SPACING_STEP);
	event_add_window_icon_bump(choices_window, CHOICE_ICON_DELAY,
			CHOICE_ICON_DELAY_UP, CHOICE_ICON_DELAY_DOWN,
			CHOICE_DELAY_MIN, CHOICE_DELAY_MAX, CHOICE_DELAY_STEP);
	event_add_window_icon_bump(choices_window, CHOICE_ICON_SIDEBAR,
			CHOICE_ICON_SIDEBAR_UP, CHOICE_ICON_SIDEBAR_DOWN,
			CHOICE_SIDEBAR_MIN, CHOICE_SIDEBAR_MAX, CHOICE_SIDEBAR_STEP);
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
	icons_printf(choices_window, CHOICE_ICON_SIZE, "%d", config_int_read("GridSize"));
	icons_printf(choices_window, CHOICE_ICON_SPACING, "%d", config_int_read("GridSpacing"));
	icons_printf(choices_window, CHOICE_ICON_DELAY, "%d", config_int_read("OpenDelay"));
	icons_printf(choices_window, CHOICE_ICON_SIDEBAR, "%d", config_int_read("SideBarSize"));

	icons_set_selected(choices_window, CHOICE_ICON_CONFIRM_DELETE, config_opt_read("ConfirmDelete"));
	icons_set_selected(choices_window, CHOICE_ICON_MOUSE_OVER, config_opt_read("MouseOver"));
}


/**
 * Update the configuration settings from the values in the Choices window.
 */

static void choices_read_window(void)
{
	config_int_set("GridSize", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SIZE)));
	config_int_set("GridSpacing", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SPACING)));
	config_int_set("OpenDelay", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_DELAY)));
	config_int_set("SideBarSize", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SIDEBAR)));

	config_opt_set("ConfirmDelete", icons_get_selected(choices_window, CHOICE_ICON_CONFIRM_DELETE));
	config_opt_set("MouseOver", icons_get_selected(choices_window, CHOICE_ICON_MOUSE_OVER));
}


/**
 * Refresh the Choices dialogue, to reflech changed icon states.
 */

static void choices_redraw_window(void)
{
	wimp_set_icon_state(choices_window, CHOICE_ICON_SIZE, 0, 0);
	wimp_set_icon_state(choices_window, CHOICE_ICON_SPACING, 0, 0);
	wimp_set_icon_state(choices_window, CHOICE_ICON_DELAY, 0, 0);
	wimp_set_icon_state(choices_window, CHOICE_ICON_SIDEBAR, 0, 0);
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

	switch (pointer->i) {
	case CHOICE_ICON_APPLY:
	case CHOICE_ICON_SAVE:
		if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
			choices_read_window();

			if (pointer->i == CHOICE_ICON_SAVE)
				config_save();

			panel_refresh_choices();

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
		panel_refresh_choices();
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

