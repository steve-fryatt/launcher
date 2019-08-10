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

/* OSLib header files. */

#include "oslib/wimp.h"
#include "oslib/os.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files. */

#include "buttons.h"

#include "appdb.h"
#include "choices.h"
#include "edit.h"
#include "filing.h"
#include "icondb.h"
#include "main.h"
#include "paneldb.h"


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

/**
 * The possible buttons window positions.
 */

enum buttons_position {
	BUTTONS_POSITION_NONE = 0,
	BUTTONS_POSITION_LEFT = 1,
	BUTTONS_POSITION_RIGHT = 2,
	BUTTONS_POSITION_VERTICAL = 3,
	BUTTONS_POSITION_TOP = 4,
	BUTTONS_POSITION_BOTTOM = 8,
	BUTTONS_POSITION_HORIZONTAL = 12
};

/**
 * The buttion instance data block.
 */

struct buttons_block {
	/**
	 * The panel ID, as used in the configuration.
	 */

	unsigned panel_id;

	/**
	 * The location of the panel.
	 */

	enum buttons_position location;

	/**
	 * The sort position of the panel.
	 */

	int sort;

	/**
	 * The width weight of the panel.
	 */

	int width;

	/**
	 * The number of columns in the visible grid.
	 */

	int grid_columns;

	/**
	 * The number of rows in the visible grid.
	 */

	int grid_rows;

	/**
	 * The horizontal origin of the button grid (in OS units).
	 */

	int origin_x;

	/**
	 * The vertical origin of the button grid (in OS units).
	 */

	int origin_y;

	/**
	 * Indicate whether the window is currently "open" (TRUE) or "closed" (FALSE).
	 */

	osbool panel_is_open;

	/**
	 * The handle of the buttons window.
	 */

	wimp_w window;

	/**
	 * The lower (bottom) coordinate of the buttons window.
	 */

	int min_extent;

	/**
	 * The upper (top) coordinate of the buttons window.
	 */

	int max_extent;

	/**
	 * The next instance in the list, or NULL.
	 */

	struct buttons_block *next;
};

/* Global Variables. */

/**
 * The list of buttons windows.
 */

static struct buttons_block *buttons_list = NULL;

/**
 * The Wimp window definition for a button window.
 */

static wimp_window *buttons_window_def;

/**
 * The Wimp icon definition for a button icon.
 */

static wimp_icon_create buttons_icon_def;


/**
 * The width of the current mode, in OS Units.
 */

static int buttons_mode_width;

/**
 * The height of the current mode, in OS Units.
 */

static int buttons_mode_height;

/**
 * The height of the iconbar, in OS Units.
 */

static int buttons_iconbar_height;

/**
 * The size of a grid square (in OS units).
 */

static int buttons_grid_square;

/**
 * The spacing between grid squares (in OS units).
 */

static int buttons_grid_spacing;


/**
 * The width of one button slab (in OS units).
 */

static int buttons_slab_width;

/**
 * The height of one button slab (in OS units).
 */

static int buttons_slab_height;

/**
 * The handle of the main menu.
 */

static wimp_menu *buttons_menu = NULL;

/**
 * The block for the icon over which the main menu last opened.
 */

static struct icondb_button *buttons_menu_icon = NULL;

/**
 * The grid coordinates where the main menu last opened.
 */

static os_coord buttons_menu_coordinate;

/* Static Function Prototypes. */

static void buttons_create_instance(unsigned key);

static void buttons_click_handler(wimp_pointer *pointer);
static void buttons_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void buttons_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static osbool buttons_message_mode_change(wimp_message *message);

static void buttons_update_mode_details(void);
static void buttons_update_positions(void);

static void buttons_toggle_window(struct buttons_block *windat);
static void buttons_reopen_window(struct buttons_block *windat);
static void buttons_open_window(wimp_open *open);
static void buttons_update_window_position(struct buttons_block *windat, os_box positions);

static void buttons_update_grid_info(struct buttons_block *windat);
static void buttons_rebuild_window(struct buttons_block *windat);

static void buttons_create_icon(struct buttons_block *windat, struct icondb_button *button);
static void buttons_delete_icon(struct buttons_block *windat, struct icondb_button *button);
static void buttons_press(struct buttons_block *windat, wimp_i icon);

static void buttons_open_edit_dialogue(wimp_pointer *pointer, struct icondb_button *button, os_coord *grid);
static osbool buttons_process_edit_dialogue(struct appdb_entry *entry, void *data);

static struct buttons_block *buttons_find_id(unsigned id);


/**
 * Initialise the buttons window.
 */

void buttons_initialise(void)
{
	/* Initialise the menus used in the window. */

	buttons_menu = templates_get_menu("MainMenu");
	ihelp_add_menu(buttons_menu, "MainMenu");

	/* Initialise the main Buttons window template. */

	buttons_window_def = templates_load_window("Launch");
	if (buttons_window_def == NULL)
		error_msgs_report_fatal("BadTemplate");

	buttons_window_def->icon_count = 1;

	buttons_icon_def.icon = buttons_window_def->icons[1];

	/* Watch out for Message_ModeChange. */

	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, buttons_message_mode_change);

	/* Initialise the Edit dialogue. */

	edit_initialise();

	/* Correctly size the window for the current mode. */

	buttons_update_mode_details();
	buttons_refresh_choices();
}


/**
 * Terminate the buttons window.
 */

void buttons_terminate(void)
{
	icondb_terminate();
	edit_terminate();
}


static void buttons_create_instance(unsigned key)
{
	struct paneldb_entry panel;
	struct buttons_block *new;

	if (paneldb_get_panel_info(key, &panel) == NULL)
		return;

	new = heap_alloc(sizeof(struct buttons_block));
	if (new == NULL)
		return;

	new->panel_id = key;
	new->grid_columns = 0;
	new->grid_rows = 0;
	new->origin_x = 0;
	new->origin_y = 0;
	new->panel_is_open = FALSE;
	new->min_extent = 0;
	new->max_extent = 0;

	switch (panel.position) {
	case PANELDB_POSITION_LEFT:
		new->location = BUTTONS_POSITION_LEFT;
		break;
	case PANELDB_POSITION_RIGHT:
		new->location = BUTTONS_POSITION_RIGHT;
		break;
	case PANELDB_POSITION_TOP:
		new->location = BUTTONS_POSITION_TOP;
		break;
	case PANELDB_POSITION_BOTTOM:
		new->location = BUTTONS_POSITION_BOTTOM;
		break;
	default:
		new->location = BUTTONS_POSITION_LEFT;
		break;
	}

	new->window = wimp_create_window(buttons_window_def);
	ihelp_add_window(new->window, "Launch", NULL);
	event_add_window_user_data(new->window, new);
	event_add_window_open_event(new->window, buttons_open_window);
	event_add_window_mouse_event(new->window, buttons_click_handler);
	event_add_window_menu(new->window, buttons_menu);
	event_add_window_menu_prepare(new->window, buttons_menu_prepare);
	event_add_window_menu_selection(new->window, buttons_menu_selection);

	/* Link the window in to the data structure. */

	new->next = buttons_list;
	buttons_list = new;

	/* Open the window. */

	buttons_update_grid_info(new);
	buttons_rebuild_window(new);
//	buttons_update_window_position(new);
//	buttons_reopen_window(new);
}


/**
 * Inform the buttons module that the global system choices have changed,
 * and force an update of any relevant parameters.
 */

void buttons_refresh_choices(void)
{
	struct buttons_block *windat = buttons_list;

	buttons_grid_square = config_int_read("GridSize");
	buttons_grid_spacing = config_int_read("GridSpacing");

	/* Buttons span two grid squares, and cover the spacing in between those. */

	buttons_slab_width = 2 * buttons_grid_square + buttons_grid_spacing;
	buttons_slab_height = 2 * buttons_grid_square + buttons_grid_spacing;

	while (windat != NULL) {
		buttons_update_grid_info(windat);
		buttons_rebuild_window(windat);

		windat = windat->next;
	}
}


/**
 * Process mouse clicks in the Buttons window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void buttons_click_handler(wimp_pointer *pointer)
{
	struct buttons_block *windat;

	if (pointer == NULL)
		return;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL)
		return;

	switch ((int) pointer->buttons) {
	case wimp_CLICK_SELECT:
	case wimp_CLICK_ADJUST:
		if (pointer->i == BUTTONS_ICON_SIDEBAR) {
			buttons_toggle_window(windat);
		} else {
			buttons_press(windat, pointer->i);

			if (pointer->buttons == wimp_CLICK_SELECT)
				buttons_toggle_window(windat);
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
	struct buttons_block	*windat;


	if (pointer == NULL)
		return;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (pointer->i == wimp_ICON_WINDOW || pointer->i == BUTTONS_ICON_SIDEBAR)
		buttons_menu_icon = NULL;
	else
		buttons_menu_icon = icondb_find_icon(pointer->w, pointer->i);

	menus_shade_entry(buttons_menu, BUTTONS_MENU_BUTTON, (buttons_menu_icon == NULL) ? TRUE : FALSE);
	menus_shade_entry(buttons_menu, BUTTONS_MENU_NEW_BUTTON, (pointer->i == wimp_ICON_WINDOW) ? FALSE : TRUE);

	window.w = w;
	wimp_get_window_state(&window);

	buttons_menu_coordinate.x = (pointer->pos.x - window.visible.x0) + window.xscroll;
	buttons_menu_coordinate.y = (window.visible.y1 - pointer->pos.y) - window.yscroll;

	buttons_menu_coordinate.x = (windat->origin_x - (buttons_menu_coordinate.x - (buttons_grid_spacing / 2))) / (buttons_grid_square + buttons_grid_spacing);
	buttons_menu_coordinate.y = (windat->origin_y + (buttons_menu_coordinate.y + (buttons_grid_spacing / 2))) / (buttons_grid_square + buttons_grid_spacing);
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
	wimp_pointer		pointer;
	os_error		*error;
	struct buttons_block	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

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
					buttons_delete_icon(windat, buttons_menu_icon);
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
		filing_save("Buttons");
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
	buttons_update_mode_details();
	return TRUE;
}


/**
 * Update the details of the current screen mode.
 */

static void buttons_update_mode_details(void)
{
	wimp_window_state	state;
	os_error		*error;

	/* Get the screen mode dimensions. */

	buttons_mode_width = general_mode_width();
	buttons_mode_height = general_mode_height();

	/* Get the iconbar height. */

	state.w = wimp_ICON_BAR;
	error = xwimp_get_window_state(&state);

	buttons_iconbar_height = (error == NULL) ? state.visible.y1 : sf_ICONBAR_HEIGHT;

	/* Update the bars. */

	buttons_update_positions();
}


/**
 * Update the positions of all of the bars on the screen.
 */

static void buttons_update_positions(void)
{
	struct buttons_block	*windat = NULL;
	os_box			locations;

	/* Locate the positions of all of the panels.*/

	locations.x0 = 0;
	locations.x1 = 0;
	locations.y0 = 0;
	locations.y1 = 0;

	windat = buttons_list;

	while (windat != NULL) {
		switch (windat->location) {
		case BUTTONS_POSITION_LEFT:
			locations.x0++;
			break;
		case BUTTONS_POSITION_RIGHT:
			locations.x1++;
			break;
		case BUTTONS_POSITION_TOP:
			locations.y1++;
			break;
		case BUTTONS_POSITION_BOTTOM:
			locations.y0++;
			break;
		case BUTTONS_POSITION_VERTICAL:
		case BUTTONS_POSITION_HORIZONTAL:
		case BUTTONS_POSITION_NONE:
			break;
		}
		windat = windat->next;
	}

	/* Update all of the panel positions. */

	windat = buttons_list;

	while (windat != NULL) {
		buttons_update_window_position(windat, locations);
		buttons_reopen_window(windat);

		windat = windat->next;
	}

}


/**
 * Toggle the state of the button window, open and closed.
 *
 * \param *windat		The window to be updated.
 */

static void buttons_toggle_window(struct buttons_block *windat)
{
	if (windat == NULL)
		return;

	windat->panel_is_open = !windat->panel_is_open;

	buttons_reopen_window(windat);
}


/**
 * Re-open the buttons window using any changed parameters which might
 * have been calculated.
 *
 * \param *windat		The window to be reopened.
 */

static void buttons_reopen_window(struct buttons_block *windat)
{
	wimp_window_state	state;
	os_error		*error;

	if (windat == NULL)
		return;

	state.w = windat->window;
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
	struct buttons_block	*windat;
	int			sidebar_size, grid_size;
	wimp_icon_state		state;
	os_error		*error;
	wimp_window_info	info;

	if (open == NULL)
		return;

	windat = event_get_window_user_data(open->w);
	if (windat == NULL)
		return;

	state.w = windat->window;
	state.i = BUTTONS_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	info.w = windat->window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	grid_size = buttons_grid_spacing + (windat->grid_columns * (buttons_grid_spacing + buttons_grid_square));

	switch (windat->location) {
	case BUTTONS_POSITION_LEFT:
		sidebar_size = state.icon.extent.x1 - state.icon.extent.x0;

		open->visible.x0 = 0;
		open->visible.x1 = ((windat->panel_is_open) ? grid_size : 0) + sidebar_size;
		open->visible.y0 = windat->min_extent;
		open->visible.y1 = windat->max_extent;

		open->xscroll = (windat->panel_is_open) ? 0 : grid_size;
		open->yscroll = 0;
		break;

	case BUTTONS_POSITION_RIGHT:
		sidebar_size = state.icon.extent.x1 - state.icon.extent.x0;

		open->visible.x0 = buttons_mode_width - (((windat->panel_is_open) ? grid_size : 0) + sidebar_size);
		open->visible.x1 = buttons_mode_width;
		open->visible.y0 = windat->min_extent;
		open->visible.y1 = windat->max_extent;

		open->xscroll = 0;
		open->yscroll = 0;
		break;

	case BUTTONS_POSITION_TOP:
		sidebar_size = state.icon.extent.y1 - state.icon.extent.y0;

		open->visible.x0 = windat->min_extent;
		open->visible.x1 = windat->max_extent;
		open->visible.y0 = buttons_mode_height - (((windat->panel_is_open) ? grid_size : 0) + sidebar_size);
		open->visible.y1 = buttons_mode_height;

		open->xscroll = 0;
		open->yscroll = (windat->panel_is_open) ? 0 : -grid_size;
		break;

	case BUTTONS_POSITION_BOTTOM:
		sidebar_size = state.icon.extent.y1 - state.icon.extent.y0;

		open->visible.x0 = windat->min_extent;
		open->visible.x1 = windat->max_extent;
		open->visible.y0 = buttons_iconbar_height;
		open->visible.y1 = buttons_iconbar_height + (((windat->panel_is_open) ? grid_size : 0) + sidebar_size);

		open->xscroll = 0;
		open->yscroll = 0;
		break;

	case BUTTONS_POSITION_HORIZONTAL:
	case BUTTONS_POSITION_VERTICAL:
	case BUTTONS_POSITION_NONE:
		break;
	}

	open->next = (windat->panel_is_open) ? wimp_TOP : wimp_BOTTOM;

	wimp_open_window(open);
}


/**
 * Update the vertical position of the buttons window to take into account
 * a change of screen mode.
 *
 * \param *windat		The window to be reopened.
 * \param positions		The numbers of bars on each screen edge.
 */

static void buttons_update_window_position(struct buttons_block *windat, os_box positions)
{
	int			old_window_size, new_window_size, bar_size;
	wimp_window_info	info;
	wimp_icon_state		state;
	os_error		*error;

	if (windat == NULL)
		return;

	info.w = windat->window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	state.w = windat->window;
	state.i = BUTTONS_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	if (windat->location & BUTTONS_POSITION_VERTICAL) {
		old_window_size = info.extent.y1 - info.extent.y0;
		bar_size = state.icon.extent.x1 - state.icon.extent.x0; // \TODO -- Assumes square icon!

		/* Calculate the new vertical size of the window. */

		new_window_size = buttons_mode_height - buttons_iconbar_height;

		if (positions.y1 > 0)
			new_window_size -= bar_size;

		if (positions.y0 > 0)
			new_window_size -= bar_size;

		info.extent.y1 = 0;
		info.extent.y0 = info.extent.y1 - new_window_size;

		/* Extend the extent if required, but never bother to shrink it. */

		if (old_window_size < new_window_size) {
			error = xwimp_set_extent(windat->window, &(info.extent));
			if (error != NULL)
				return;
		}

		/* Resize the icon to fit the new dimensions. */

		error = xwimp_resize_icon(windat->window, BUTTONS_ICON_SIDEBAR,
			state.icon.extent.x0, info.extent.y0, state.icon.extent.x1, info.extent.y1);

		/* Adjust the new visible window height. */

		windat->min_extent = buttons_iconbar_height;
		if (positions.y0 > 0)
			windat->min_extent += bar_size;

		windat->max_extent = buttons_mode_height;
		if (positions.y1 > 0)
			windat->max_extent -= bar_size;

		if (buttons_grid_square + buttons_grid_spacing != 0)
			windat->grid_rows = (windat->max_extent - windat->min_extent) /
					(buttons_grid_square + buttons_grid_spacing);
		else
			windat->grid_rows = 0;
	} else if (windat->location & BUTTONS_POSITION_HORIZONTAL) {
		old_window_size = info.extent.x1 - info.extent.x0;
		bar_size = state.icon.extent.y1 - state.icon.extent.y0; // \TODO -- Assumes square icon!

		/* Calculate the new horizontal size of the window. */

		new_window_size = buttons_mode_width;

		if (positions.x0 > 0)
			new_window_size -= bar_size;

		if (positions.x1 > 0)
			new_window_size -= bar_size;

		info.extent.x0 = 0;
		info.extent.x1 = info.extent.x0 + new_window_size;

		/* Extend the extent if required, but never bother to shrink it. */

		if (old_window_size < new_window_size) {
			error = xwimp_set_extent(windat->window, &(info.extent));
			if (error != NULL)
				return;
		}

		/* Resize the icon to fit the new dimensions. */

		error = xwimp_resize_icon(windat->window, BUTTONS_ICON_SIDEBAR,
			info.extent.x0, state.icon.extent.y0, info.extent.x1, state.icon.extent.y1);

		/* Adjust the new visible window width. */

		windat->min_extent = 0;
		if (positions.x0 > 0)
			windat->min_extent += bar_size;

		windat->max_extent = buttons_mode_width;
		if (positions.x1 > 0)
			windat->max_extent -= bar_size;

		if (buttons_grid_square + buttons_grid_spacing != 0)
			windat->grid_rows = (windat->max_extent - windat->min_extent) /
					(buttons_grid_square + buttons_grid_spacing);
		else
			windat->grid_rows = 0;
	}
}


/**
 * Update the button window grid details to take into account new values from
 * the configuration.
 *
 * \param *windat		The window to be updated.
  */

static void buttons_update_grid_info(struct buttons_block *windat)
{
	wimp_window_info	info;
	wimp_icon_state		state;
	os_error		*error;
	int			sidebar_width = 0;

	if (windat == NULL)
		return;

	info.w = windat->window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	state.w = windat->window;
	state.i = BUTTONS_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	windat->grid_columns = config_int_read("WindowColumns");

	if (buttons_grid_square + buttons_grid_spacing != 0)
		windat->grid_rows = (windat->max_extent - windat->min_extent) /
				(buttons_grid_square + buttons_grid_spacing);
	else
		windat->grid_rows = 0;

	if (windat->location & BUTTONS_POSITION_VERTICAL) {
		sidebar_width = state.icon.extent.x1 - state.icon.extent.x0;

		info.extent.x0 = 0;
		info.extent.x1 = info.extent.x0 + sidebar_width + buttons_grid_spacing +
				windat->grid_columns * (buttons_grid_spacing + buttons_grid_square);
	} else if (windat->location & BUTTONS_POSITION_HORIZONTAL) {
		sidebar_width = state.icon.extent.y1 - state.icon.extent.y0;

		info.extent.y1 = 0;
		info.extent.y0 = info.extent.y1 - sidebar_width - buttons_grid_spacing -
				windat->grid_columns * (buttons_grid_spacing + buttons_grid_square);
	}

	error = xwimp_set_extent(windat->window, &(info.extent));
	if (error != NULL)
		return;

	/* Origin is top-right of the grid. */

	switch (windat->location) {
	case BUTTONS_POSITION_LEFT:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (buttons_grid_square + buttons_grid_spacing);
		windat->origin_y = info.extent.y1 - buttons_grid_spacing;

		xwimp_resize_icon(windat->window, BUTTONS_ICON_SIDEBAR,
			info.extent.x1 - sidebar_width, state.icon.extent.y0, info.extent.x1, state.icon.extent.y1);
		break;

	case BUTTONS_POSITION_RIGHT:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (buttons_grid_square + buttons_grid_spacing) + sidebar_width;
		windat->origin_y = info.extent.y1 - buttons_grid_spacing;

		xwimp_resize_icon(windat->window, BUTTONS_ICON_SIDEBAR,
			info.extent.x0, state.icon.extent.y0, info.extent.x0 + sidebar_width, state.icon.extent.y1);
		break;

	case BUTTONS_POSITION_TOP:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (buttons_grid_square + buttons_grid_spacing);
		windat->origin_y = info.extent.y1 - buttons_grid_spacing;

		xwimp_resize_icon(windat->window, BUTTONS_ICON_SIDEBAR,
			state.icon.extent.x0, info.extent.y0, state.icon.extent.x1, info.extent.y0 + sidebar_width);
		break;

	case BUTTONS_POSITION_BOTTOM:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (buttons_grid_square + buttons_grid_spacing);
		windat->origin_y = info.extent.y1 - buttons_grid_spacing;

		xwimp_resize_icon(windat->window, BUTTONS_ICON_SIDEBAR,
			state.icon.extent.x0, info.extent.y1 - sidebar_width, state.icon.extent.x1, info.extent.y1);
		break;

	case BUTTONS_POSITION_HORIZONTAL:
	case BUTTONS_POSITION_VERTICAL:
	case BUTTONS_POSITION_NONE:
		break;
	}
}


/**
 * Create a full set of buttons from the contents of the application database.
 */

void buttons_create_from_db(void)
{
	unsigned		key, panel;
	struct icondb_button	*new;
	struct buttons_block	*windat;

	/* Create the panels defined in the database. */

	key = PANELDB_NULL_KEY;

	do {
		key = paneldb_get_next_key(key);

		if (key != PANELDB_NULL_KEY)
			buttons_create_instance(key);
	} while (key != PANELDB_NULL_KEY);

	/* Add the buttons defined in the database. */

	key = APPDB_NULL_KEY;

	do {
		key = appdb_get_next_key(key);

		if (key != APPDB_NULL_KEY) {
			panel = appdb_get_panel(key);
			if (panel == APPDB_NULL_PANEL)
				continue;

			windat = buttons_find_id(panel);
			if (windat == NULL)
				continue;

			new = icondb_create_icon(key);

			if (new != NULL)
				buttons_create_icon(windat, new);
		}
	} while (key != APPDB_NULL_KEY);

	buttons_update_positions();
}


/**
 * Rebuild the contents of a buttons window.
 *
 * \param *windat		The window to be rebuilt.
 */

static void buttons_rebuild_window(struct buttons_block *windat)
{
	struct icondb_button *button = icondb_get_list();

	while (button != NULL) {
		buttons_create_icon(windat, button);

		button = button->next;
	}

	buttons_reopen_window(windat);
}


/**
 * Create (or recreate) an icon for a button, based on that icon's definition
 * block.
 *
 * \param *windat		The window to create the icon in.
 * \param *button		The definition to create an icon for.
 */

static void buttons_create_icon(struct buttons_block *windat, struct icondb_button *button)
{
	os_error		*error = NULL;
	struct appdb_entry	*entry = NULL;

	if (windat == NULL || button == NULL)
		return;

	if (button->icon != -1) {
		error = xwimp_delete_icon(windat->window, button->icon);
		if (error != NULL)
			error_report_program(error);
		button->icon = -1;
	}

	entry = appdb_get_button_info(button->key, NULL);
	if (entry == NULL)
		return;

	buttons_icon_def.w = windat->window;

	buttons_icon_def.icon.extent.x1 = windat->origin_x - entry->x * (buttons_grid_square + buttons_grid_spacing);
	buttons_icon_def.icon.extent.x0 = buttons_icon_def.icon.extent.x1 - buttons_slab_width;
	buttons_icon_def.icon.extent.y1 = windat->origin_y - entry->y * (buttons_grid_square + buttons_grid_spacing);
	buttons_icon_def.icon.extent.y0 = buttons_icon_def.icon.extent.y1 - buttons_slab_height;

	string_printf(button->validation, ICONDB_VALIDATION_LENGTH, "R5,1;S%s;NButton", entry->sprite);
	buttons_icon_def.icon.data.indirected_text_and_sprite.validation = button->validation;

	button->window = windat->window;
	button->icon = wimp_create_icon(&buttons_icon_def);

	windows_redraw(windat->window);
}


/**
 * Delete a button and all its associated information.
 *
 * \param *windat		The window to delete the icon from.
 * \param *button		The button to be deleted.
 */

static void buttons_delete_icon(struct buttons_block *windat, struct icondb_button *button)
{
	os_error	*error;

	if (windat == NULL || button == NULL)
		return;

	/* Delete the button's icon. */

	if (button->icon != -1) {
		error = xwimp_delete_icon(windat->window, button->icon);
		if (error != NULL)
			error_report_program(error);
		button->icon = -1;

		windows_redraw(windat->window);
	}

	/* Delete the application and button details from the databases. */

	appdb_delete_key(button->key);
	icondb_delete_icon(button);
}


/**
 * Press a button in the window.
 *
 * \param *windat		The window containing the icon.
 * \param icon			The handle of the icon being pressed.
 */

static void buttons_press(struct buttons_block *windat, wimp_i icon)
{
	struct appdb_entry	entry;
	char			*buffer;
	int			length;
	os_error		*error;
	struct icondb_button	*button = NULL;

	if (windat == NULL)
		return;

	button = icondb_find_icon(windat->window, icon);

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

static void buttons_open_edit_dialogue(wimp_pointer *pointer, struct icondb_button *button, os_coord *grid)
{
	struct appdb_entry entry;

	/* Initialise deafults if button data can't be found. */

	entry.panel = 0;
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
	struct icondb_button *button = data;
	struct buttons_block *windat = NULL;

	if (entry == NULL)
		return FALSE;

	windat = buttons_find_id(entry->panel);
	if (windat == NULL) {
		error_msgs_report_error("BadPanel");
		return FALSE;
	}

	/* Validate the button location. */

	if (entry->x < 0 || entry->y < 0 || entry->x >= windat->grid_columns || entry->y >= windat->grid_rows) {
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

	if (button == NULL)
		button = icondb_create_icon(appdb_create_key());

	if (button == NULL) {
		error_msgs_report_error("NoMemNewButton");
		return FALSE;
	}

	/* Store the application in the database. */

	entry->key = button->key;
	appdb_set_button_info(entry);

	buttons_create_icon(windat, button);

	return TRUE;
}


/**
 * Given a panel id number, return the associated panel data block.
 *
 * \param id		The Panel Id to locate.
 * \return		Pointer to the associated block, or NULL.
 */

static struct buttons_block *buttons_find_id(unsigned id)
{
	struct buttons_block *windat = buttons_list;

	while (windat != NULL && windat->panel_id != id)
		windat = windat->next;

	return windat;
}

