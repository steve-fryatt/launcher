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
 * \file: edit_button.c
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

#include "edit_button.h"

#include "appdb.h"
#include "objutil.h"

/* Edit Dialogue */

#define EDIT_BUTTON_ICON_OK 0
#define EDIT_BUTTON_ICON_CANCEL 1
#define EDIT_BUTTON_ICON_NAME 3
#define EDIT_BUTTON_ICON_XPOS 5
#define EDIT_BUTTON_ICON_XPOS_DOWN 6
#define EDIT_BUTTON_ICON_XPOS_UP 7
#define EDIT_BUTTON_ICON_YPOS 9
#define EDIT_BUTTON_ICON_YPOS_DOWN 10
#define EDIT_BUTTON_ICON_YPOS_UP 11
#define EDIT_BUTTON_ICON_SPRITE 13
#define EDIT_BUTTON_ICON_LOCATION 15
#define EDIT_BUTTON_ICON_ACTION_BOOT 17
#define EDIT_BUTTON_ICON_ACTION_SPRITE 18
#define EDIT_BUTTON_ICON_ACTION_NONE 19
#define EDIT_BUTTON_ICON_REFLOW 20

/* Static Function Prototypes. */

static void		edit_button_close_window(void);
static void		edit_button_fill_window(struct appdb_entry *data);
static void		edit_button_redraw_window(void);
static osbool		edit_button_read_window(void);
static osbool		edit_button_message_data_load(wimp_message *message);
static osbool		edit_button_keypress_handler(wimp_key *key);
static void		edit_button_click_handler(wimp_pointer *pointer);

/* ================================================================================================================== */

/**
 * The handle of the button edit dialogue.
 */

static wimp_w			edit_button_window = NULL;

/**
 * The client callback function for the current dialogue.
 */

static osbool			(*edit_button_callback)(struct appdb_entry *entry, void *data) = NULL;

/**
 * The handle of the target button icon.
 */

static void			*edit_button_target_icon = NULL;

/**
 * The default starting data for the current dialogue.
 */

static struct appdb_entry	*edit_button_default_data = NULL;


/**
 * Initialise the Edit dialogue.
 */

void edit_button_initialise(void)
{
	/* Create the Edit dialogue. */

	edit_button_window = templates_create_window("EditButton");
	ihelp_add_window(edit_button_window, "EditButton", NULL);
	event_add_window_mouse_event(edit_button_window, edit_button_click_handler);
	event_add_window_key_event(edit_button_window, edit_button_keypress_handler);

	event_add_window_icon_bump(edit_button_window, EDIT_BUTTON_ICON_XPOS, EDIT_BUTTON_ICON_XPOS_UP, EDIT_BUTTON_ICON_XPOS_DOWN, 0, 999, 1);
	event_add_window_icon_bump(edit_button_window, EDIT_BUTTON_ICON_YPOS, EDIT_BUTTON_ICON_YPOS_UP, EDIT_BUTTON_ICON_YPOS_DOWN, 0, 999, 1);

	event_add_window_icon_radio(edit_button_window, EDIT_BUTTON_ICON_ACTION_BOOT, TRUE);
	event_add_window_icon_radio(edit_button_window, EDIT_BUTTON_ICON_ACTION_SPRITE, TRUE);
	event_add_window_icon_radio(edit_button_window, EDIT_BUTTON_ICON_ACTION_NONE, TRUE);

	/* Watch out for Message_DataLoad. */

	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, edit_button_message_data_load);
}


/**
 * Terminate the Edit dialogue.
 */

void edit_button_terminate(void)
{
	/* Nothing to do! */
}


/**
 * Open a Button Edit dialogue for a given target.
 *
 * \param *pointer	The pointer location at which to open the dialogue.
 * \param *data		The AppDB data to display in the dialogue.
 * \param reflow	TRUE if the reflow message should be shown; else FALSE.
 * \param *callback	A callback to receive the dialogue data.
 * \param *target	A client-specified target for the callback.
 */

void edit_button_open_dialogue(wimp_pointer *pointer, struct appdb_entry *data, osbool reflow, osbool (*callback)(struct appdb_entry *entry, void *data), void *target)
{
	if (data == NULL)
		return;

	if (windows_get_open(edit_button_window))
		wimp_close_window(edit_button_window);

	edit_button_default_data = malloc(sizeof(struct appdb_entry));
	if (edit_button_default_data == NULL)
		return;

	appdb_copy(edit_button_default_data, data);

	edit_button_callback = callback;
	edit_button_target_icon = target;

	icons_set_deleted(edit_button_window, EDIT_BUTTON_ICON_REFLOW, !reflow);
	edit_button_fill_window(edit_button_default_data);
	windows_open_centred_at_pointer(edit_button_window, pointer);
	icons_put_caret_at_end(edit_button_window, EDIT_BUTTON_ICON_NAME);
}


/**
 * Process mouse clicks in the Edit dialogue.
 *
 * \param *pointer	The mouse event block to handle.
 */

static void edit_button_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch (pointer->i) {
	case EDIT_BUTTON_ICON_OK:
		if (edit_button_read_window() && (pointer->buttons == wimp_CLICK_SELECT))
			edit_button_close_window();
		break;

	case EDIT_BUTTON_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			edit_button_close_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			edit_button_fill_window(edit_button_default_data);
			edit_button_redraw_window();
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

static osbool edit_button_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		if (edit_button_read_window())
			edit_button_close_window();
		break;

	case wimp_KEY_ESCAPE:
		edit_button_close_window();
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

static void edit_button_close_window(void)
{
	wimp_close_window(edit_button_window);

	edit_button_callback = NULL;
	edit_button_target_icon = NULL;

	if (edit_button_default_data != NULL) {
		free(edit_button_default_data);
		edit_button_default_data = NULL;
	
	}
}


/**
 * Fill the fields of the edit dialogue.
 *
 * \param *data		The data to write into the dialogue.
 */

static void edit_button_fill_window(struct appdb_entry *data)
{
	if (data == NULL)
		return;

	icons_strncpy(edit_button_window, EDIT_BUTTON_ICON_NAME, data->name);
	icons_strncpy(edit_button_window, EDIT_BUTTON_ICON_SPRITE, data->sprite);
	icons_strncpy(edit_button_window, EDIT_BUTTON_ICON_LOCATION, data->command);

	icons_printf(edit_button_window, EDIT_BUTTON_ICON_XPOS, "%d", data->position.x);
	icons_printf(edit_button_window, EDIT_BUTTON_ICON_YPOS, "%d", data->position.y);

	icons_set_selected(edit_button_window, EDIT_BUTTON_ICON_ACTION_BOOT, data->boot_action == APPDB_BOOT_ACTION_BOOT);
	icons_set_selected(edit_button_window, EDIT_BUTTON_ICON_ACTION_SPRITE, data->boot_action == APPDB_BOOT_ACTION_SPRITES);
	icons_set_selected(edit_button_window, EDIT_BUTTON_ICON_ACTION_NONE, data->boot_action == APPDB_BOOT_ACTION_NONE);
}


/**
 * Redraw the contents of the button edit window.
 */

static void edit_button_redraw_window(void)
{
	wimp_set_icon_state(edit_button_window, EDIT_BUTTON_ICON_NAME, 0, 0);
	wimp_set_icon_state(edit_button_window, EDIT_BUTTON_ICON_SPRITE, 0, 0);
	wimp_set_icon_state(edit_button_window, EDIT_BUTTON_ICON_LOCATION, 0, 0);
	wimp_set_icon_state(edit_button_window, EDIT_BUTTON_ICON_XPOS, 0, 0);
	wimp_set_icon_state(edit_button_window, EDIT_BUTTON_ICON_YPOS, 0, 0);

	icons_replace_caret_in_window(edit_button_window);
}


/**
 * Read the data from the edit window, and feed it back to the client.
 *
 * \return		TRUE if the client accepted the date; otherwise FALSE.
 */

static osbool edit_button_read_window(void)
{
	struct appdb_entry	entry;

	if (edit_button_callback == NULL)
		return TRUE;

	entry.panel = edit_button_default_data->panel;

	entry.position.x = atoi(icons_get_indirected_text_addr(edit_button_window, EDIT_BUTTON_ICON_XPOS));
	entry.position.y = atoi(icons_get_indirected_text_addr(edit_button_window, EDIT_BUTTON_ICON_YPOS));

	if (icons_get_selected(edit_button_window, EDIT_BUTTON_ICON_ACTION_BOOT))
		entry.boot_action = APPDB_BOOT_ACTION_BOOT;
	else if (icons_get_selected(edit_button_window, EDIT_BUTTON_ICON_ACTION_SPRITE))
		entry.boot_action = APPDB_BOOT_ACTION_SPRITES;
	else
		entry.boot_action = APPDB_BOOT_ACTION_NONE;

	icons_copy_text(edit_button_window, EDIT_BUTTON_ICON_NAME, entry.name, APPDB_NAME_LENGTH);
	icons_copy_text(edit_button_window, EDIT_BUTTON_ICON_SPRITE, entry.sprite, APPDB_SPRITE_LENGTH);
	icons_copy_text(edit_button_window, EDIT_BUTTON_ICON_LOCATION, entry.command, APPDB_COMMAND_LENGTH);

	return edit_button_callback(&entry, edit_button_target_icon);
}


/**
 * Handle incoming Message_DataSave to the button edit window, by using the
 * information to populate the relevant fields.
 *
 * \param *message	The message data block from the Wimp.
 */

static osbool edit_button_message_data_load(wimp_message *message)
{
	char *leafname, spritename[APPDB_SPRITE_LENGTH];

	wimp_full_message_data_xfer *data_load = (wimp_full_message_data_xfer *) message;

	if (data_load->w != edit_button_window)
		return FALSE;

	if (!windows_get_open(edit_button_window))
		return TRUE;

	leafname = string_find_leafname(data_load->file_name);

	if (!objutil_find_sprite(data_load->file_name, spritename, APPDB_SPRITE_LENGTH))
		return TRUE;

	icons_strncpy(edit_button_window, EDIT_BUTTON_ICON_NAME, leafname);
	icons_strncpy(edit_button_window, EDIT_BUTTON_ICON_SPRITE, spritename);
	icons_strncpy(edit_button_window, EDIT_BUTTON_ICON_LOCATION, data_load->file_name);

	edit_button_redraw_window();

	return TRUE;
}

