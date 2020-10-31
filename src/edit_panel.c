/* Copyright 2002-2020, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Launcher:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: edit_panel.c
 */

/* ANSI C header files. */

#include <string.h>
#include <stdio.h>

/* OSLib header files. */

#include "oslib/wimp.h"
#include "oslib/messagetrans.h"
#include "oslib/os.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files. */

#include "edit_panel.h"

#include "paneldb.h"

/* Edit Dialogue */

#define EDIT_PANEL_ICON_OK 0
#define EDIT_PANEL_ICON_CANCEL 1
#define EDIT_PANEL_ICON_NAME 3
#define EDIT_PANEL_ICON_LOCATION 7
#define EDIT_PANEL_ICON_LOCATION_MENU 8
#define EDIT_PANEL_ICON_WIDTH 10
#define EDIT_PANEL_ICON_WIDTH_DOWN 11
#define EDIT_PANEL_ICON_WIDTH_UP 12
#define EDIT_PANEL_ICON_ORDER 14
#define EDIT_PANEL_ICON_ORDER_DOWN 15
#define EDIT_PANEL_ICON_ORDER_UP 16
#define EDIT_PANEL_ICON_SLAB_WIDTH 20
#define EDIT_PANEL_ICON_SLAB_WIDTH_DOWN 21
#define EDIT_PANEL_ICON_SLAB_WIDTH_UP 22
#define EDIT_PANEL_ICON_SLAB_HEIGHT 25
#define EDIT_PANEL_ICON_SLAB_HEIGHT_DOWN 26
#define EDIT_PANEL_ICON_SLAB_HEIGHT_UP 27
#define EDIT_PANEL_ICON_DEPTH 30
#define EDIT_PANEL_ICON_DEPTH_DOWN 31
#define EDIT_PANEL_ICON_DEPTH_UP 32

/* Static Function Prototypes. */

static void		edit_panel_close_window(void);
static void		edit_panel_fill_window(struct paneldb_entry *data);
static void		edit_panel_redraw_window(void);
static osbool		edit_panel_read_window(void);
static osbool		edit_panel_keypress_handler(wimp_key *key);
static void		edit_panel_click_handler(wimp_pointer *pointer);

/* ================================================================================================================== */

/**
 * The handle of the panel edit dialogue.
 */

static wimp_w			edit_panel_window = NULL;

/**
 * The client callback function for the current dialogue.
 */

static osbool			(*edit_panel_callback)(struct paneldb_entry *entry, void *data) = NULL;

/**
 * The handle of the target button icon.
 */

static void			*edit_panel_target_icon = NULL;

/**
 * The default starting data for the current dialogue.
 */

static struct paneldb_entry	*edit_panel_default_data = NULL;


/**
 * Initialise the Edit dialogue.
 */

void edit_panel_initialise(void)
{
	wimp_menu *location_menu;

	/* Create the Edit dialogue. */

	edit_panel_window = templates_create_window("EditPanel");
	ihelp_add_window(edit_panel_window, "EditPanel", NULL);

	location_menu = templates_get_menu("PanelLocationMenu");
	ihelp_add_menu(location_menu, "PanelLocationMenu");

	event_add_window_mouse_event(edit_panel_window, edit_panel_click_handler);
	event_add_window_key_event(edit_panel_window, edit_panel_keypress_handler);

	event_add_window_icon_popup(edit_panel_window, EDIT_PANEL_ICON_LOCATION_MENU, location_menu, EDIT_PANEL_ICON_LOCATION, NULL);

	event_add_window_icon_bump(edit_panel_window, EDIT_PANEL_ICON_WIDTH,
			EDIT_PANEL_ICON_WIDTH_UP, EDIT_PANEL_ICON_WIDTH_DOWN,
			1, 9999, 1);

	event_add_window_icon_bump(edit_panel_window, EDIT_PANEL_ICON_ORDER,
			EDIT_PANEL_ICON_ORDER_UP, EDIT_PANEL_ICON_ORDER_DOWN,
			1, 9999, 1);

	event_add_window_icon_bump(edit_panel_window, EDIT_PANEL_ICON_SLAB_WIDTH,
			EDIT_PANEL_ICON_SLAB_WIDTH_UP, EDIT_PANEL_ICON_SLAB_WIDTH_DOWN,
			1, 10, 1);

	event_add_window_icon_bump(edit_panel_window, EDIT_PANEL_ICON_SLAB_HEIGHT,
			EDIT_PANEL_ICON_SLAB_HEIGHT_UP, EDIT_PANEL_ICON_SLAB_HEIGHT_DOWN,
			1, 10, 1);

	event_add_window_icon_bump(edit_panel_window, EDIT_PANEL_ICON_DEPTH,
			EDIT_PANEL_ICON_DEPTH_UP, EDIT_PANEL_ICON_DEPTH_DOWN,
			1, 10, 1);
}


/**
 * Terminate the Edit dialogue.
 */

void edit_panel_terminate(void)
{
	/* Nothing to do! */
}


/**
 * Open a Panel Edit dialogue for a given target.
 *
 * \param *pointer	The pointer location at which to open the dialogue.
 * \param *data		The PanelDB data to display in the dialogue.
 * \param *callback	A callback to receive the dialogue data.
 * \param *target	A client-specified target for the callback.
 */

void edit_panel_open_dialogue(wimp_pointer *pointer, struct paneldb_entry *data, osbool (*callback)(struct paneldb_entry *entry, void *data), void *target)
{
	if (data == NULL)
		return;

	if (windows_get_open(edit_panel_window))
		wimp_close_window(edit_panel_window);

	edit_panel_default_data = malloc(sizeof(struct paneldb_entry));
	if (edit_panel_default_data == NULL)
		return;

	paneldb_copy(edit_panel_default_data, data);

	edit_panel_callback = callback;
	edit_panel_target_icon = target;

	edit_panel_fill_window(edit_panel_default_data);
	windows_open_centred_at_pointer(edit_panel_window, pointer);
	icons_put_caret_at_end(edit_panel_window, EDIT_PANEL_ICON_NAME);
}


/**
 * Process mouse clicks in the Edit dialogue.
 *
 * \param *pointer	The mouse event block to handle.
 */

static void edit_panel_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch (pointer->i) {
	case EDIT_PANEL_ICON_OK:
		if (edit_panel_read_window() && (pointer->buttons == wimp_CLICK_SELECT))
			edit_panel_close_window();
		break;

	case EDIT_PANEL_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			edit_panel_close_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			edit_panel_fill_window(edit_panel_default_data);
			edit_panel_redraw_window();
		}
		break;
	}
}


/**
 * Process keypresses in the Edit dialogue.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool edit_panel_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		if (edit_panel_read_window())
			edit_panel_close_window();
		break;

	case wimp_KEY_ESCAPE:
		edit_panel_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Close the current instance of the edit dialogue.
 */

static void edit_panel_close_window(void)
{
	wimp_close_window(edit_panel_window);

	edit_panel_callback = NULL;
	edit_panel_target_icon = NULL;

	if (edit_panel_default_data != NULL) {
		free(edit_panel_default_data);
		edit_panel_default_data = NULL;
	
	}
}


/**
 * Fill the fields of the edit dialogue.
 *
 * \param *data		The data to write into the dialogue.
 */

static void edit_panel_fill_window(struct paneldb_entry *data)
{
	if (data == NULL)
		return;

	icons_strncpy(edit_panel_window, EDIT_PANEL_ICON_NAME, data->name);
	event_set_window_icon_popup_selection(edit_panel_window, EDIT_PANEL_ICON_LOCATION_MENU,
			(data->position < PANELDB_POSITION_MAX) ? data->position : PANELDB_POSITION_LEFT);
	icons_printf(edit_panel_window, EDIT_PANEL_ICON_ORDER, "%d", data->sort);
	icons_printf(edit_panel_window, EDIT_PANEL_ICON_WIDTH, "%d", data->width);
	icons_printf(edit_panel_window, EDIT_PANEL_ICON_SLAB_WIDTH, "%d", data->slab_size.x);
	icons_printf(edit_panel_window, EDIT_PANEL_ICON_SLAB_HEIGHT, "%d", data->slab_size.y);
	icons_printf(edit_panel_window, EDIT_PANEL_ICON_DEPTH, "%d", data->depth);
}


/**
 * Redraw the contents of the button edit window.
 */

static void edit_panel_redraw_window(void)
{
	wimp_set_icon_state(edit_panel_window, EDIT_PANEL_ICON_NAME, 0, 0);
	wimp_set_icon_state(edit_panel_window, EDIT_PANEL_ICON_LOCATION, 0, 0);
	wimp_set_icon_state(edit_panel_window, EDIT_PANEL_ICON_ORDER, 0, 0);
	wimp_set_icon_state(edit_panel_window, EDIT_PANEL_ICON_WIDTH, 0, 0);
	wimp_set_icon_state(edit_panel_window, EDIT_PANEL_ICON_SLAB_WIDTH, 0, 0);
	wimp_set_icon_state(edit_panel_window, EDIT_PANEL_ICON_SLAB_HEIGHT, 0, 0);
	wimp_set_icon_state(edit_panel_window, EDIT_PANEL_ICON_DEPTH, 0, 0);

	icons_replace_caret_in_window(edit_panel_window);
}


/**
 * Read the data from the edit window, and feed it back to the client.
 *
 * \return		TRUE if the client accepted the date; otherwise FALSE.
 */

static osbool edit_panel_read_window(void)
{
	struct paneldb_entry	entry;

	if (edit_panel_callback == NULL)
		return TRUE;

	icons_copy_text(edit_panel_window, EDIT_PANEL_ICON_NAME, entry.name, PANELDB_NAME_LENGTH);
	entry.position = event_get_window_icon_popup_selection(edit_panel_window, EDIT_PANEL_ICON_LOCATION_MENU);
	entry.sort = atoi(icons_get_indirected_text_addr(edit_panel_window, EDIT_PANEL_ICON_ORDER));
	entry.width = atoi(icons_get_indirected_text_addr(edit_panel_window, EDIT_PANEL_ICON_WIDTH));
	entry.slab_size.x = atoi(icons_get_indirected_text_addr(edit_panel_window, EDIT_PANEL_ICON_SLAB_WIDTH));
	entry.slab_size.y = atoi(icons_get_indirected_text_addr(edit_panel_window, EDIT_PANEL_ICON_SLAB_HEIGHT));
	entry.depth = atoi(icons_get_indirected_text_addr(edit_panel_window, EDIT_PANEL_ICON_DEPTH));

	return edit_panel_callback(&entry, edit_panel_target_icon);
}
