/* Copyright 2002-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Launcher:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
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
 * \file: edit.c
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
#include "sflib/debug.h"
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

#include "edit.h"

#include "appdb.h"

/* Edit Dialogue */

#define ICON_EDIT_OK 0
#define ICON_EDIT_CANCEL 1
#define ICON_EDIT_NAME 2
#define ICON_EDIT_XPOS 5
#define ICON_EDIT_YPOS 7
#define ICON_EDIT_SPRITE 8
#define ICON_EDIT_KEEP_LOCAL 10
#define ICON_EDIT_LOCATION 11
#define ICON_EDIT_BOOT 13

/* Static Function Prototypes. */

static void		edit_close_window(void);
static void		edit_fill_edit_window(struct appdb_entry *data);
static void		edit_redraw_edit_window(void);
static osbool		edit_read_edit_window(void);
static osbool		edit_message_data_load(wimp_message *message);
static osbool		edit_edit_keypress_handler(wimp_key *key);
static void		edit_edit_click_handler(wimp_pointer *pointer);

/* ================================================================================================================== */

/**
 * The handle of the button edit dialogue.
 */

static wimp_w			edit_window = NULL;

/**
 * The handle of the target button icon.
 */

static void			*edit_target_icon = NULL;

/**
 * The default starting data for the current dialogue.
 */

static struct appdb_entry	*edit_default_data = NULL;


/**
 * Initialise the Edit dialogue.
 */

void edit_initialise(void)
{
	/* Create the Edit dialogue. */

	edit_window = templates_create_window("Edit");
	ihelp_add_window(edit_window, "Edit", NULL);
	event_add_window_mouse_event(edit_window, edit_edit_click_handler);
	event_add_window_key_event(edit_window, edit_edit_keypress_handler);

	/* Watch out for Message_ModeChange. */

	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, edit_message_data_load);
}


/**
 * Terminate the Edit dialogue.
 */

void edit_terminate(void)
{
	/* Nothing to do! */
}


/**
 * Open a Button Edit dialogue for a given target.
 *
 * \param *pointer	The pointer location at which to open the dialogue.
 * \param *target	A client-specified target for the dialogue.
 * \param *data		The AppDB data to display in the dialogue.
 */

void edit_open_dialogue(wimp_pointer *pointer, void *target, struct appdb_entry *data)
{
	if (data == NULL)
		return;

	edit_default_data = malloc(sizeof(struct appdb_entry));
	if (edit_default_data == NULL)
		return;

	appdb_copy(edit_default_data, data);

	edit_target_icon = target;

	edit_fill_edit_window(edit_default_data);
	windows_open_centred_at_pointer(edit_window, pointer);
	icons_put_caret_at_end(edit_window, ICON_EDIT_NAME);
}


/**
 * Process mouse clicks in the Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void edit_edit_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch (pointer->i) {
	case ICON_EDIT_OK:
		if (edit_read_edit_window() && (pointer->buttons == wimp_CLICK_SELECT))
			edit_close_window();
		break;

	case ICON_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			edit_close_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			edit_fill_edit_window(edit_default_data);
			edit_redraw_edit_window();
		}
		break;
	}
}


/**
 * Process keypresses in the Edit dialogue.
 *
 * \param *key			The keypress event block to handle.
 * \return			TRUE if the event was handled; else FALSE.
 */

static osbool edit_edit_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		if (edit_read_edit_window())
			edit_close_window();
		break;

	case wimp_KEY_ESCAPE:
		edit_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}



static void edit_close_window(void)
{
	wimp_close_window(edit_window);

	edit_target_icon = NULL;

	if (edit_default_data != NULL) {
		free(edit_default_data);
		edit_default_data = NULL;
	
	}
}


static void edit_fill_edit_window(struct appdb_entry *data)
{
	if (data == NULL)
		return;

	icons_strncpy(edit_window, ICON_EDIT_NAME, data->name);
	icons_strncpy(edit_window, ICON_EDIT_SPRITE, data->sprite);
	icons_strncpy(edit_window, ICON_EDIT_LOCATION, data->command);

	icons_printf(edit_window, ICON_EDIT_XPOS, "%d", data->x);
	icons_printf(edit_window, ICON_EDIT_YPOS, "%d", data->y);

	icons_set_selected(edit_window, ICON_EDIT_KEEP_LOCAL, data->local_copy);
	icons_set_selected(edit_window, ICON_EDIT_BOOT, data->filer_boot);
}


/**
 * Redraw the contents of the button edit window.
 */

static void edit_redraw_edit_window(void)
{
	wimp_set_icon_state(edit_window, ICON_EDIT_NAME, 0, 0);
	wimp_set_icon_state(edit_window, ICON_EDIT_SPRITE, 0, 0);
	wimp_set_icon_state(edit_window, ICON_EDIT_LOCATION, 0, 0);
	wimp_set_icon_state(edit_window, ICON_EDIT_XPOS, 0, 0);
	wimp_set_icon_state(edit_window, ICON_EDIT_YPOS, 0, 0);

	icons_replace_caret_in_window(edit_window);
}




static osbool edit_read_edit_window(void)
{
	struct appdb_entry	entry;

	entry.x = atoi(icons_get_indirected_text_addr(edit_window, ICON_EDIT_XPOS));
	entry.y = atoi(icons_get_indirected_text_addr(edit_window, ICON_EDIT_YPOS));

	entry.local_copy = icons_get_selected(edit_window, ICON_EDIT_KEEP_LOCAL);
	entry.filer_boot = icons_get_selected(edit_window, ICON_EDIT_BOOT);

	icons_copy_text(edit_window, ICON_EDIT_NAME, entry.name, APPDB_NAME_LENGTH);
	icons_copy_text(edit_window, ICON_EDIT_SPRITE, entry.sprite, APPDB_SPRITE_LENGTH);
	icons_copy_text(edit_window, ICON_EDIT_LOCATION, entry.command, APPDB_COMMAND_LENGTH);

	return FALSE;
}


/**
 * Handle incoming Message_DataSave to the button edit window, by using the
 * information to populate the relevant fields.
 *
 * \param *message		The message data block from the Wimp.
 */

static osbool edit_message_data_load(wimp_message *message)
{
	char *leafname, spritename[APPDB_SPRITE_LENGTH];

	wimp_full_message_data_xfer *data_load = (wimp_full_message_data_xfer *) message;

	if (data_load->w != edit_window)
		return FALSE;

	if (!windows_get_open(edit_window))
		return TRUE;

	leafname = string_find_leafname(data_load->file_name);

	if (leafname[0] == '!') {
		strncpy(spritename, leafname, APPDB_SPRITE_LENGTH);
		string_tolower(spritename);

		/* \TODO -- If the sprite doesn't exist, use application? */

		leafname++;
	} else {
		strncpy(spritename, "file_xxx", APPDB_SPRITE_LENGTH);

		/* \TODO -- This should sort out filetype sprites! */
	}

	icons_strncpy(edit_window, ICON_EDIT_NAME, leafname);
	icons_strncpy(edit_window, ICON_EDIT_SPRITE, spritename);
	icons_strncpy(edit_window, ICON_EDIT_LOCATION, data_load->file_name);

	edit_redraw_edit_window();

	return TRUE;
}

