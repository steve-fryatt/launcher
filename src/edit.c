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

static void		edit_fill_edit_window(struct button *button, os_coord *grid);
static void		edit_redraw_edit_window(void);
static struct button	*edit_read_edit_window(struct button *button);
static osbool		edit_message_data_load(wimp_message *message);
static osbool		edit_edit_keypress_handler(wimp_key *key);
static void		edit_edit_click_handler(wimp_pointer *pointer);

/* ================================================================================================================== */

/**
 * The handle of the button edit dialogue.
 */

static wimp_w		edit_window = NULL;

/**
 * The handle of the target button icon.
 */

static wimp_i		edit_target_icon = 0;



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



void edit_open_dialogue(wimp_i target, struct appdb_entry data)
{
	if (data == NULL)
		return;

	

}



/**
 * Set the contents of the button edit window.
 *
 * \param *button		The button to set data for, or NULL to use defaults.
 * \param *grid			The grid coordinates to use, or NULL to use previous.
 */

static void edit_fill_edit_window(struct button *button, os_coord *grid)
{
	struct appdb_entry entry;

	/* Initialise deafults if button data can't be found. */

	entry.x = 0;
	entry.y = 0;
	entry.local_copy = FALSE;
	entry.filer_boot = TRUE;
	*entry.name = '\0';
	*entry.sprite = '\0';
	*entry.command = '\0';

	if (button != NULL)
		appdb_get_button_info(button->key, &entry);

	if (grid != NULL) {
		entry.x = grid->x;
		entry.y = grid->y;
	}

	/* Fill the window's icons. */

	icons_strncpy(edit_window, ICON_EDIT_NAME, entry.name);
	icons_strncpy(edit_window, ICON_EDIT_SPRITE, entry.sprite);
	icons_strncpy(edit_window, ICON_EDIT_LOCATION, entry.command);

	icons_printf(edit_window, ICON_EDIT_XPOS, "%d", entry.x);
	icons_printf(edit_window, ICON_EDIT_YPOS, "%d", entry.y);

	icons_set_selected(edit_window, ICON_EDIT_KEEP_LOCAL, entry.local_copy);
	icons_set_selected(edit_window, ICON_EDIT_BOOT, entry.filer_boot);
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


/**
 * Read the contents of the button edit window, validate it and store the
 * details into a button definition.
 *
 * \param *button		The button to update; NULL to create a new entry.
 * \return			Pointer to the button containing the data.
 */

static struct button *edit_read_edit_window(struct button *button)
{
	struct appdb_entry	entry;

	entry.x = atoi(icons_get_indirected_text_addr(edit_window, ICON_EDIT_XPOS));
	entry.y = atoi(icons_get_indirected_text_addr(edit_window, ICON_EDIT_YPOS));

	if (entry.x < 0 || entry.y < 0 || entry.x >= edit_grid_columns || entry.y >= edit_grid_rows) {
		error_msgs_report_info("CoordRange");
		return NULL;
	}

	entry.local_copy = icons_get_selected(edit_window, ICON_EDIT_KEEP_LOCAL);
	entry.filer_boot = icons_get_selected(edit_window, ICON_EDIT_BOOT);

	icons_copy_text(edit_window, ICON_EDIT_NAME, entry.name, APPDB_NAME_LENGTH);
	icons_copy_text(edit_window, ICON_EDIT_SPRITE, entry.sprite, APPDB_SPRITE_LENGTH);
	icons_copy_text(edit_window, ICON_EDIT_LOCATION, entry.command, APPDB_COMMAND_LENGTH);

	if (*entry.name == '\0' || *entry.sprite == '\0' || *entry.command == '\0') {
		error_msgs_report_info("MissingText");
		return NULL;
	}

	/* If this is a new button, create its entry and get a database
	 * key for the application details.
	 */

	if (button == NULL) {
		button = heap_alloc(sizeof(struct button));

		if (button != NULL) {
			button->key = appdb_create_key();
			button->icon = -1;
			button->validation[0] = '\0';

			if (button->key != APPDB_NULL_KEY) {
				button->next = edit_list;
				edit_list = button;
			} else {
				heap_free(button);
				button = NULL;
			}
		}
	}

	/* Store the application in the database. */

	if (button != NULL) {
		entry.key = button->key;
		appdb_set_button_info(&entry);
	}

	return button;
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




/**
 * Process mouse clicks in the Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void edit_edit_click_handler(wimp_pointer *pointer)
{
	struct button		*button;

	if (pointer == NULL)
		return;

	switch (pointer->i) {
	case ICON_EDIT_OK:
		button = edit_read_edit_window(edit_edit_icon);
		if (button != NULL) {
			edit_create_icon(button);
			if (pointer->buttons == wimp_CLICK_SELECT) {
				wimp_close_window(edit_window);
				edit_edit_icon = NULL;
			}
		}
		break;

	case ICON_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			wimp_close_window(edit_window);
			edit_edit_icon = NULL;
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			edit_fill_edit_window(edit_edit_icon, NULL);
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
	struct button		*button;


	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		button = edit_read_edit_window(edit_edit_icon);
		if (button != NULL) {
			edit_create_icon(button);
			wimp_close_window(edit_window);
			edit_edit_icon = NULL;
		}
		break;

	case wimp_KEY_ESCAPE:
		wimp_close_window(edit_window);
		edit_edit_icon = NULL;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

