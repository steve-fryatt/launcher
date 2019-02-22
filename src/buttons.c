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
 * \file: buttons.c
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

#include "buttons.h"

#include "appdb.h"
#include "choices.h"
#include "edit.h"
#include "main.h"


/* Button Window */

#define BUTTONS_ICON_SIDEBAR 0
#define BUTTONS_ICON_TEMPLATE 1

/* Main Menu */

#define BUTTONS_MENU_INFO 0
#define BUTTONS_MENU_HELP 1
#define BUTTONS_MENU_BUTTON 2
#define BUTTONS_MENU_NEW_BUTTON 3
#define BUTTONS_MENU_SAVE_BUTTONS 4
#define BUTTONS_MENU_CHOICES 5
#define BUTTONS_MENU_QUIT 6

/* Button Submenu */

#define BUTTONS_MENU_BUTTON_EDIT 0
#define BUTTONS_MENU_BUTTON_MOVE 1
#define BUTTONS_MENU_BUTTON_DELETE 2

/* ================================================================================================================== */

#define BUTTONS_VALIDATION_LENGTH 40

struct button {
	unsigned	key;							/**< The database key relating to the icon.			*/

	wimp_i		icon;
	char		validation[BUTTONS_VALIDATION_LENGTH];			/**< Storage for the icon's validation string.			*/

	struct button	*next;							/**< Pointer to the next button definition.			*/
};


static struct button	*buttons_list = NULL;

static wimp_icon_create	buttons_icon_def;					/**< The definition for a button icon.				*/

static int		buttons_grid_square;					/**< The size of a grid square (in OS units).			*/
static int		buttons_grid_spacing;					/**< The spacing between grid squares (in OS units).		*/

static int		buttons_grid_columns;					/**< The number of columns in the visible grid.			*/
static int		buttons_grid_rows;					/**< The number of rows in the visible grid.			*/

static int		buttons_origin_x;					/**< The horizontal origin of the button grid (in OS units).	*/
static int		buttons_origin_y;					/**< The vertical origin of the button grid (in OS units).	*/

static int		buttons_slab_width;					/**< The width of one button slab (in OS units).		*/
static int		buttons_slab_height;					/**< The height of one button slab (in OS units).		*/

static osbool		buttons_window_is_open = FALSE;				/**< TRUE if the window is currently 'open'; else FALSE.	*/


static wimp_w		buttons_window = NULL;					/**< The handle of the buttons window.				*/

static wimp_menu	*buttons_menu = NULL;					/**< The main menu.						*/

static struct button	*buttons_menu_icon = NULL;				/**< The block for the icon over which the main menu opened.	*/
static os_coord		buttons_menu_coordinate;				/**< The grid coordinates where the main menu opened.		*/

static int		buttons_window_y0 = 0;					/**< The bottom of the buttons window.				*/
static int		buttons_window_y1 = 0;					/**< The top of the buttons window.				*/

/* Static Function Prototypes. */

static void		buttons_click_handler(wimp_pointer *pointer);
static void		buttons_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void		buttons_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static osbool		buttons_message_mode_change(wimp_message *message);

static void		buttons_toggle_window(void);
static void		buttons_reopen_window(void);
static void		buttons_open_window(wimp_open *open);
static void		buttons_update_window_position(void);

static void		buttons_update_grid_info(void);

static void		buttons_create_icon(struct button *button);
static void		buttons_delete_icon(struct button *button);
static void		buttons_press(wimp_i icon);

static void		buttons_open_edit_dialogue(wimp_pointer *pointer, struct button *button, os_coord *grid);
static osbool		buttons_process_edit_dialogue(struct appdb_entry *entry, void *data);

/* ================================================================================================================== */


/**
 * Initialise the buttons window.
 */

void buttons_initialise(void)
{
	wimp_window		*def = NULL;


	/* Initialise the menus used in the window. */

	buttons_menu = templates_get_menu("MainMenu");
	ihelp_add_menu(buttons_menu, "MainMenu");

	/* Initialise the main Buttons window. */

	def = templates_load_window("Launch");
	if (def == NULL)
		error_msgs_report_fatal("BadTemplate");

	def->icon_count = 1;
	buttons_window = wimp_create_window(def);
	ihelp_add_window(buttons_window, "Launch", NULL);
	event_add_window_open_event(buttons_window, buttons_open_window);
	event_add_window_mouse_event(buttons_window, buttons_click_handler);
	event_add_window_menu(buttons_window, buttons_menu);
	event_add_window_menu_prepare(buttons_window, buttons_menu_prepare);
	event_add_window_menu_selection(buttons_window, buttons_menu_selection);

	buttons_icon_def.icon = def->icons[1];
	buttons_icon_def.w = buttons_window;

	free(def);

	/* Watch out for Message_ModeChange. */

	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, buttons_message_mode_change);

	/* Initialise the Edit dialogue. */

	edit_initialise();

	/* Correctly size the window for the current mode. */

	buttons_update_window_position();
	buttons_update_grid_info();

	/* Open the window. */

	buttons_reopen_window();
}


/**
 * Terminate the buttons window.
 */

void buttons_terminate(void)
{
	struct button	*current;

	while (buttons_list != NULL) {
		current = buttons_list;
		buttons_list = current->next;
		heap_free(current);
	}

	edit_terminate();
}


/**
 * Inform the buttons module that the global system choices have changed,
 * and force an update of any relevant parameters.
 */

void buttons_refresh_choices(void)
{
	struct button	*button = buttons_list;

	buttons_update_grid_info();

	while (button != NULL) {
		buttons_create_icon(button);

		button = button->next;
	}

	buttons_reopen_window();
}


/**
 * Process mouse clicks in the Buttons window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void buttons_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch ((int) pointer->buttons) {
	case wimp_CLICK_SELECT:
	case wimp_CLICK_ADJUST:
		if (pointer->i == BUTTONS_ICON_SIDEBAR) {
			buttons_toggle_window();
		} else {
			buttons_press(pointer->i);

			if (pointer->buttons == wimp_CLICK_SELECT)
				buttons_toggle_window();
		}
		break;
	}
}


/**
 * Prepare the main menu for (re)-opening.
 *
 * \param  w			The handle of the menu's parent window.
 * \param  *menu		Pointer to the menu being opened.
 * \param  *pointer		Pointer to the Wimp Pointer event block.
 */

static void buttons_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	wimp_window_state	window;


	if (pointer == NULL)
		return;

	if (pointer->i == wimp_ICON_WINDOW || pointer->i == BUTTONS_ICON_SIDEBAR) {
		buttons_menu_icon = NULL;
	} else {
		buttons_menu_icon = buttons_list;

		while (buttons_menu_icon!= NULL && buttons_menu_icon->icon != pointer->i)
			buttons_menu_icon = buttons_menu_icon->next;
	}

	menus_shade_entry(buttons_menu, BUTTONS_MENU_BUTTON, (buttons_menu_icon == NULL) ? TRUE : FALSE);
	menus_shade_entry(buttons_menu, BUTTONS_MENU_NEW_BUTTON, (pointer->i == wimp_ICON_WINDOW) ? FALSE : TRUE);

	window.w = buttons_window;
	wimp_get_window_state(&window);

	buttons_menu_coordinate.x = (pointer->pos.x - window.visible.x0) + window.xscroll;
	buttons_menu_coordinate.y = (window.visible.y1 - pointer->pos.y) - window.yscroll;

	buttons_menu_coordinate.x = (buttons_origin_x - (buttons_menu_coordinate.x - (buttons_grid_spacing / 2))) / (buttons_grid_square + buttons_grid_spacing);
	buttons_menu_coordinate.y = (buttons_origin_y + (buttons_menu_coordinate.y + (buttons_grid_spacing / 2))) / (buttons_grid_square + buttons_grid_spacing);
}


/**
 * Handle selections from the main menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param  *selection		Pointer to the Wimp menu selction block.
 */

static void buttons_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	wimp_pointer	pointer;
	os_error	*error;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]) {
	case BUTTONS_MENU_HELP:
		error = xos_cli("%Filer_Run <Launcher$Dir>.!Help");
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
		break;

	case BUTTONS_MENU_BUTTON:
		if (buttons_menu_icon != NULL) {
			switch (selection->items[1]) {
			case BUTTONS_MENU_BUTTON_EDIT:
				buttons_open_edit_dialogue(&pointer, buttons_menu_icon, NULL);
				break;

			case BUTTONS_MENU_BUTTON_DELETE:
				if (!config_opt_read("ConfirmDelete") || (error_msgs_report_question("QDelete", "QDeleteB") == 3)) {
					buttons_delete_icon(buttons_menu_icon);
					buttons_menu_icon = NULL;
				}
				break;
			}
		}
		break;

	case BUTTONS_MENU_NEW_BUTTON:
		buttons_open_edit_dialogue(&pointer, buttons_menu_icon, &buttons_menu_coordinate);
		break;

	case BUTTONS_MENU_SAVE_BUTTONS:
		appdb_save_file("Buttons");
		break;

	case BUTTONS_MENU_CHOICES:
		choices_open_window(&pointer);
		break;

	case BUTTONS_MENU_QUIT:
		main_quit_flag = TRUE;
		break;
	}
}


/**
 * Handle incoming Message_ModeChange.
 *
 * \param *message		The message data block from the Wimp.
 */

static osbool buttons_message_mode_change(wimp_message *message)
{

	buttons_update_window_position();
	buttons_reopen_window();

	return TRUE;
}


/**
 * Toggle the state of the button window, open and closed.
 */

static void buttons_toggle_window(void)
{
	buttons_window_is_open = !buttons_window_is_open;

	buttons_reopen_window();
}


/**
 * Re-open the buttons window using any changed parameters which might
 * have been calculated.
 */

static void buttons_reopen_window(void)
{
	wimp_window_state	state;
	os_error		*error;

	state.w = buttons_window;
	error = xwimp_get_window_state(&state);
	if (error != NULL)
		return;

	buttons_open_window((wimp_open *) &state);
}

/**
 * Open or 'close' the buttons window to a given number of columns and
 * place it at a defined point in the window stack.
 *
 * \param *open			The Wimp_Open data for the window.
 */

static void buttons_open_window(wimp_open *open)
{
	int			sidebar_width;
	wimp_icon_state		state;
	os_error		*error;

	if (open == NULL || open->w != buttons_window)
		return;

	state.w = buttons_window;
	state.i = BUTTONS_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	sidebar_width = state.icon.extent.x1 - state.icon.extent.x0;

	open->visible.x0 = 0;
	open->visible.x1 = ((buttons_window_is_open) ? (buttons_grid_spacing + buttons_grid_columns * (buttons_grid_spacing + buttons_grid_square)) : 0) + sidebar_width;
	open->visible.y0 = buttons_window_y0;
	open->visible.y1 = buttons_window_y1;

	open->xscroll = (buttons_window_is_open) ? 0 : (buttons_grid_spacing + buttons_grid_columns * (buttons_grid_spacing + buttons_grid_square));
	open->yscroll = 0;
	open->next = (buttons_window_is_open) ? wimp_TOP : wimp_BOTTOM;

	wimp_open_window(open);
}


/**
 * Update the vertical position of the buttons window to take into account
 * a change of screen mode.
 */

static void buttons_update_window_position(void)
{
	int			mode_height, old_window_height, new_window_height;
	wimp_window_info	info;
	wimp_icon_state		state;
	os_error		*error;


	info.w = buttons_window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	state.w = buttons_window;
	state.i = BUTTONS_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	old_window_height = info.extent.y1 - info.extent.y0;

	/* Calculate the new vertical size of the window. */

	mode_height = general_mode_height();
	new_window_height = mode_height - sf_ICONBAR_HEIGHT;

	info.extent.y1 = 0;
	info.extent.y0 = info.extent.y1 - new_window_height;

	/* Extend the extent if required, but never bother to shrink it. */

	if (old_window_height < new_window_height) {
		error = xwimp_set_extent(buttons_window, &(info.extent));
		if (error != NULL)
			return;
	}

	/* Resize the icon to fit the new dimensions. */

	error = xwimp_resize_icon(buttons_window, BUTTONS_ICON_SIDEBAR,
		state.icon.extent.x0, info.extent.y0, state.icon.extent.x1, info.extent.y1);

	/* Adjust the new visible window height. */

	buttons_window_y0 = sf_ICONBAR_HEIGHT;
	buttons_window_y1 = mode_height;

	if (buttons_grid_square + buttons_grid_spacing != 0)
		buttons_grid_rows = (buttons_window_y1 - buttons_window_y0) /
				(buttons_grid_square + buttons_grid_spacing);
	else
		buttons_grid_rows = 0;
}


/**
 * Update the button window grid details to take into account new values from
 * the configuration.
 */

static void buttons_update_grid_info(void)
{
	wimp_window_info	info;
	wimp_icon_state		state;
	os_error		*error;
	int			sidebar_width;


	info.w = buttons_window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	state.w = buttons_window;
	state.i = BUTTONS_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	buttons_grid_square = config_int_read("GridSize");
	buttons_grid_spacing = config_int_read("GridSpacing");
	buttons_grid_columns = config_int_read("WindowColumns");

	if (buttons_grid_square + buttons_grid_spacing != 0)
		buttons_grid_rows = (buttons_window_y1 - buttons_window_y0) /
				(buttons_grid_square + buttons_grid_spacing);
	else
		buttons_grid_rows = 0;

	/* Origin is top-right of the grid. */

	buttons_origin_x = info.extent.x0 + buttons_grid_columns * (buttons_grid_square + buttons_grid_spacing);
	buttons_origin_y = info.extent.y1 - buttons_grid_spacing;

	/* Buttons span two grid squares, and cover the spacing in between those. */

	buttons_slab_width = 2 * buttons_grid_square + buttons_grid_spacing;
	buttons_slab_height = 2 * buttons_grid_square + buttons_grid_spacing;

	sidebar_width = state.icon.extent.x1 - state.icon.extent.x0;

	info.extent.x0 = 0;
	info.extent.x1 = info.extent.x0 + sidebar_width + buttons_grid_spacing +
			buttons_grid_columns * (buttons_grid_spacing + buttons_grid_square);

	error = xwimp_set_extent(buttons_window, &(info.extent));
	if (error != NULL)
		return;

	error = xwimp_resize_icon(buttons_window, BUTTONS_ICON_SIDEBAR,
		info.extent.x1 - sidebar_width, state.icon.extent.y0, info.extent.x1, state.icon.extent.y1);
}


/**
 * Create a full set of buttons from the contents of the application database.
 */

void buttons_create_from_db(void)
{
	unsigned	key = APPDB_NULL_KEY;
	struct button	*new;

	do {
		key = appdb_get_next_key(key);

		if (key != APPDB_NULL_KEY) {
			new = heap_alloc(sizeof(struct button));

			if (new != NULL) {
				new->next = buttons_list;
				buttons_list = new;

				new->key = key;
				new->icon = -1;
				new->validation[0] = '\0';

				buttons_create_icon(new);
			}
		}
	} while (key != APPDB_NULL_KEY);
}


/**
 * Create (or recreate) an icon for a button, based on that icon's definition
 * block.
 *
 * \param *button		The definition to create an icon for.
 */

static void buttons_create_icon(struct button *button)
{
	os_error		*error = NULL;
	struct appdb_entry	*entry = NULL;

	if (button == NULL)
		return;

	if (button->icon != -1) {
		error = xwimp_delete_icon(buttons_icon_def.w, button->icon);
		if (error != NULL)
			error_report_program(error);
		button->icon = -1;
	}

	entry = appdb_get_button_info(button->key, NULL);
	if (entry == NULL)
		return;

	buttons_icon_def.icon.extent.x1 = buttons_origin_x - entry->x * (buttons_grid_square + buttons_grid_spacing);
	buttons_icon_def.icon.extent.x0 = buttons_icon_def.icon.extent.x1 - buttons_slab_width;
	buttons_icon_def.icon.extent.y1 = buttons_origin_y - entry->y * (buttons_grid_square + buttons_grid_spacing);
	buttons_icon_def.icon.extent.y0 = buttons_icon_def.icon.extent.y1 - buttons_slab_height;

	string_printf(button->validation, BUTTONS_VALIDATION_LENGTH, "R5,1;S%s;NButton", entry->sprite);
	buttons_icon_def.icon.data.indirected_text_and_sprite.validation = button->validation;

	button->icon = wimp_create_icon(&buttons_icon_def);

	windows_redraw(buttons_window);
}


/**
 * Delete a button and all its associated information.
 *
 * \param *button		The button to be deleted.
 */

static void buttons_delete_icon(struct button *button)
{
	struct button	*parent;
	os_error	*error;

	if (button == NULL)
		return;

	/* Delete the button's icon. */

	if (button->icon != -1) {
		error = xwimp_delete_icon(buttons_icon_def.w, button->icon);
		if (error != NULL)
			error_report_program(error);
		button->icon = -1;

		windows_redraw(buttons_window);
	}

	/* Delete the application details from the database. */

	appdb_delete_key(button->key);

	/* Delink and delete the button data. */

	if (buttons_list == button) {
		buttons_list = button->next;
	} else {
		parent = buttons_list;

		while (parent != NULL && parent->next != button)
			parent = parent->next;

		if (parent != NULL)
			parent->next = button->next;
	}

	heap_free(button);
}


/**
 * Press a button in the window.
 *
 * \param icon			The handle of the icon being pressed.
 */

static void buttons_press(wimp_i icon)
{
	struct appdb_entry	entry;
	char			*buffer;
	int			length;
	os_error		*error;
	struct button		*button = buttons_list;


	while (button!= NULL && button->icon != icon)
		button = button->next;

	if (button == NULL)
		return;

	if (appdb_get_button_info(button->key, &entry) == NULL)
		return;

	length = strlen(entry.command) + 19;

	buffer = heap_alloc(length);

	if (buffer == NULL)
		return;

	string_printf(buffer, length, "%%StartDesktopTask %s", entry.command);
	error = xos_cli(buffer);
	if (error != NULL)
		error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);

	heap_free(buffer);

	return;
}


/*
 * Open an edit dialogue box for a button.
 *
 * \param *pointer	The pointer coordinates at which to open the dialogue.
 * \param *button	The button to open the dialogue for, or NULL to
 *			open a blank dialogue.
 * \param *grid		The coordinates for a blank dialogue.
 */

static void buttons_open_edit_dialogue(wimp_pointer *pointer, struct button *button, os_coord *grid)
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

	edit_open_dialogue(pointer, &entry, buttons_process_edit_dialogue, button);
}


/**
 * Handle the data returned from an Edit dialogue instance.
 *
 * \param *entry	Pointer to the data from the dialogue.
 * \param *data		Pointer to the button owning the dialogue, or NULL.
 * \return		TRUE if the data is OK; FALSE to reject it.
 */

static osbool buttons_process_edit_dialogue(struct appdb_entry *entry, void *data)
{
	struct button *button = data;

	/* Validate the button location. */

	if (entry->x < 0 || entry->y < 0 || entry->x >= buttons_grid_columns || entry->y >= buttons_grid_rows) {
		error_msgs_report_info("CoordRange");
		return FALSE;
	}


	if (*(entry->name) == '\0' || *(entry->sprite) == '\0' || *(entry->command) == '\0') {
		error_msgs_report_info("MissingText");
		return FALSE;
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
				button->next = buttons_list;
				buttons_list = button;
			} else {
				heap_free(button);
				button = NULL;
			}
		}
	}

	/* Store the application in the database. */

	if (button != NULL) {
		entry->key = button->key;
		appdb_set_button_info(entry);

		buttons_create_icon(button);

		return TRUE;
	}

	return FALSE;
}

