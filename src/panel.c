/* Copyright 2002-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: panel.c
 */

/* ANSI C header files. */

#include <string.h>

/* OSLib header files. */

#include "oslib/wimp.h"
#include "oslib/os.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
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

#include "panel.h"

#include "appdb.h"
#include "choices.h"
#include "edit.h"
#include "filing.h"
#include "icondb.h"
#include "main.h"
#include "paneldb.h"


/* Button Window */

#define PANEL_ICON_SIDEBAR 0
#define PANEL_ICON_TEMPLATE 1

/* Main Menu */

#define PANEL_MENU_INFO 0
#define PANEL_MENU_HELP 1
#define PANEL_MENU_BUTTON 2
#define PANEL_MENU_NEW_BUTTON 3
#define PANEL_MENU_SAVE_BUTTONS 4
#define PANEL_MENU_CHOICES 5
#define PANEL_MENU_QUIT 6

/* Button Submenu */

#define PANEL_MENU_BUTTON_EDIT 0
#define PANEL_MENU_BUTTON_MOVE 1
#define PANEL_MENU_BUTTON_DELETE 2

/**
 * The possible buttons window positions.
 */

enum panel_position {
	PANEL_POSITION_NONE = 0,
	PANEL_POSITION_LEFT = 1,
	PANEL_POSITION_RIGHT = 2,
	PANEL_POSITION_VERTICAL = 3,
	PANEL_POSITION_TOP = 4,
	PANEL_POSITION_BOTTOM = 8,
	PANEL_POSITION_HORIZONTAL = 12
};

/**
 * The buttion instance data block.
 */

struct panel_block {
	/**
	 * The panel ID, as used in the configuration.
	 */

	unsigned panel_id;

	/**
	 * The location of the panel.
	 */

	enum panel_position location;

	/**
	 * The icon database to use for the panel.
	 */

	struct icondb_block *icondb;

	/**
	 * The sort position of the panel.
	 */

	int sort;

	/**
	 * The longitude weight of the panel.
	 */

	int longitude_weight;

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

	int min_longitude;

	/**
	 * The upper (top) coordinate of the buttons window.
	 */

	int max_longitude;

	/**
	 * The next instance in the list, or NULL.
	 */

	struct panel_block *next;
};

/* Global Variables. */

/**
 * The list of buttons windows.
 */

static struct panel_block *panel_list = NULL;

/**
 * The Wimp window definition for a button window.
 */

static wimp_window *panel_window_def;

/**
 * The Wimp icon definition for a button icon.
 */

static wimp_icon_create panel_icon_def;

/**
 * The width of the current mode, in OS Units.
 */

static int panel_mode_width;

/**
 * The height of the current mode, in OS Units.
 */

static int panel_mode_height;

/**
 * The height of the iconbar, in OS Units.
 */

static int panel_iconbar_height;

/**
 * The number of OS units per pixel horizontally in the current mode.
 */

static int panel_mode_x_units_per_pixel;

/**
 * The number of OS units per pixel vertically in the current mode.
 */

static int panel_mode_y_units_per_pixel;

/**
 * The size of a grid square (in OS units).
 */

static int panel_grid_square;

/**
 * The spacing between grid squares (in OS units).
 */

static int panel_grid_spacing;


/**
 * The width of one button slab (in OS units).
 */

static int panel_slab_width;

/**
 * The height of one button slab (in OS units).
 */

static int panel_slab_height;

/**
 * The handle of the main menu.
 */

static wimp_menu *panel_menu = NULL;

/**
 * The block for the icon over which the main menu last opened.
 */

static struct icondb_button *panel_menu_icon = NULL;

/**
 * The grid coordinates where the main menu last opened.
 */

static os_coord panel_menu_coordinate;

/* Static Function Prototypes. */

static void panel_create_instance(unsigned key);

static void panel_click_handler(wimp_pointer *pointer);
static void panel_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void panel_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static osbool panel_message_mode_change(wimp_message *message);

static void panel_update_mode_details(void);

static void panel_toggle_window(struct panel_block *windat);
static void panel_reopen_window(struct panel_block *windat);
static void panel_open_window(wimp_open *open);
static void panel_update_positions(void);
static int compare_panels(const void *p, const void *q);
static void panel_update_window_extent(struct panel_block *windat);

static void panel_update_grid_info(struct panel_block *windat);
static void panel_rebuild_window(struct panel_block *windat);

static void panel_create_icon(struct panel_block *windat, struct icondb_button *button);
static void panel_delete_icon(struct panel_block *windat, struct icondb_button *button);
static void panel_press(struct panel_block *windat, wimp_i icon);

static void panel_open_edit_dialogue(wimp_pointer *pointer, struct icondb_button *button, os_coord *grid);
static osbool panel_process_edit_dialogue(struct appdb_entry *entry, void *data);

static struct panel_block *panel_find_id(unsigned id);


/**
 * Initialise the buttons window.
 */

void panel_initialise(void)
{
	/* Initialise the menus used in the window. */

	panel_menu = templates_get_menu("MainMenu");
	ihelp_add_menu(panel_menu, "MainMenu");

	/* Initialise the main Buttons window template. */

	panel_window_def = templates_load_window("Launch");
	if (panel_window_def == NULL)
		error_msgs_report_fatal("BadTemplate");

	panel_window_def->icon_count = 1;

	panel_icon_def.icon = panel_window_def->icons[PANEL_ICON_TEMPLATE];

	/* Watch out for Message_ModeChange. */

	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, panel_message_mode_change);

	/* Initialise the Edit dialogue. */

	edit_initialise();

	/* Correctly size the window for the current mode. */

	panel_update_mode_details();
	panel_refresh_choices();
}


/**
 * Terminate the buttons window.
 */

void panel_terminate(void)
{
	icondb_terminate();
	edit_terminate();
}

/**
 * Create a new button bar instance, using a given database entry.
 *
 * \param key			The database key to use.
 */

static void panel_create_instance(unsigned key)
{
	struct paneldb_entry panel;
	struct panel_block *new;

	if (paneldb_get_panel_info(key, &panel) == NULL)
		return;

	new = heap_alloc(sizeof(struct panel_block));
	if (new == NULL)
		return;

	debug_printf("\\GNew Buttons Instance Created: 0x%x", new);

	new->panel_id = key;
	new->grid_columns = 0;
	new->grid_rows = 0;
	new->origin_x = 0;
	new->origin_y = 0;
	new->panel_is_open = FALSE;
	new->min_longitude = 0;
	new->max_longitude = 0;

	switch (panel.position) {
	case PANELDB_POSITION_LEFT:
		new->location = PANEL_POSITION_LEFT;
		break;
	case PANELDB_POSITION_RIGHT:
		new->location = PANEL_POSITION_RIGHT;
		break;
	case PANELDB_POSITION_TOP:
		new->location = PANEL_POSITION_TOP;
		break;
	case PANELDB_POSITION_BOTTOM:
		new->location = PANEL_POSITION_BOTTOM;
		break;
	default:
		new->location = PANEL_POSITION_LEFT;
		break;
	}

	new->icondb = icondb_create_instance();

	new->longitude_weight = panel.width;
	new->sort = panel.sort;

	new->window = wimp_create_window(panel_window_def);
	ihelp_add_window(new->window, "Launch", NULL);
	event_add_window_user_data(new->window, new);
	event_add_window_open_event(new->window, panel_open_window);
	event_add_window_mouse_event(new->window, panel_click_handler);
	event_add_window_menu(new->window, panel_menu);
	event_add_window_menu_prepare(new->window, panel_menu_prepare);
	event_add_window_menu_selection(new->window, panel_menu_selection);

	/* Link the window in to the data structure. */

	new->next = panel_list;
	panel_list = new;

	/* Open the window. */

	panel_update_grid_info(new);
	panel_rebuild_window(new);
}


/**
 * Inform the buttons module that the global system choices have changed,
 * and force an update of any relevant parameters.
 */

void panel_refresh_choices(void)
{
	struct panel_block *windat = panel_list;

	panel_grid_square = config_int_read("GridSize");
	panel_grid_spacing = config_int_read("GridSpacing");

	/* Buttons span two grid squares, and cover the spacing in between those. */

	panel_slab_width = 2 * panel_grid_square + panel_grid_spacing;
	panel_slab_height = 2 * panel_grid_square + panel_grid_spacing;

	while (windat != NULL) {
		panel_update_grid_info(windat);
		panel_rebuild_window(windat);

		windat = windat->next;
	}
}


/**
 * Process mouse clicks in the Buttons window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void panel_click_handler(wimp_pointer *pointer)
{
	struct panel_block *windat;

	if (pointer == NULL)
		return;

	windat = event_get_window_user_data(pointer->w);
	if (windat == NULL)
		return;

	switch ((int) pointer->buttons) {
	case wimp_CLICK_SELECT:
	case wimp_CLICK_ADJUST:
		if (pointer->i == PANEL_ICON_SIDEBAR) {
			panel_toggle_window(windat);
		} else {
			panel_press(windat, pointer->i);

			if (pointer->buttons == wimp_CLICK_SELECT)
				panel_toggle_window(windat);
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

static void panel_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	wimp_window_state	window;
	struct panel_block	*windat;


	if (pointer == NULL)
		return;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	if (pointer->i == wimp_ICON_WINDOW || pointer->i == PANEL_ICON_SIDEBAR)
		panel_menu_icon = NULL;
	else
		panel_menu_icon = icondb_find_icon(windat->icondb, pointer->w, pointer->i);

	menus_shade_entry(panel_menu, PANEL_MENU_BUTTON, (panel_menu_icon == NULL) ? TRUE : FALSE);
	menus_shade_entry(panel_menu, PANEL_MENU_NEW_BUTTON, (pointer->i == wimp_ICON_WINDOW) ? FALSE : TRUE);

	window.w = w;
	wimp_get_window_state(&window);

	panel_menu_coordinate.x = (pointer->pos.x - window.visible.x0) + window.xscroll;
	panel_menu_coordinate.y = (window.visible.y1 - pointer->pos.y) - window.yscroll;

	panel_menu_coordinate.x = (windat->origin_x - (panel_menu_coordinate.x - (panel_grid_spacing / 2))) / (panel_grid_square + panel_grid_spacing);
	panel_menu_coordinate.y = (windat->origin_y + (panel_menu_coordinate.y + (panel_grid_spacing / 2))) / (panel_grid_square + panel_grid_spacing);
}


/**
 * Handle selections from the main menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param  *selection		Pointer to the Wimp menu selction block.
 */

static void panel_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	wimp_pointer		pointer;
	os_error		*error;
	struct panel_block	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]) {
	case PANEL_MENU_HELP:
		error = xos_cli("%Filer_Run <Launcher$Dir>.!Help");
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
		break;

	case PANEL_MENU_BUTTON:
		if (panel_menu_icon != NULL) {
			switch (selection->items[1]) {
			case PANEL_MENU_BUTTON_EDIT:
				panel_open_edit_dialogue(&pointer, panel_menu_icon, NULL);
				break;

			case PANEL_MENU_BUTTON_DELETE:
				if (!config_opt_read("ConfirmDelete") || (error_msgs_report_question("QDelete", "QDeleteB") == 3)) {
					panel_delete_icon(windat, panel_menu_icon);
					panel_menu_icon = NULL;
				}
				break;
			}
		}
		break;

	case PANEL_MENU_NEW_BUTTON:
		panel_open_edit_dialogue(&pointer, panel_menu_icon, &panel_menu_coordinate);
		break;

	case PANEL_MENU_SAVE_BUTTONS:
		filing_save("Buttons");
		break;

	case PANEL_MENU_CHOICES:
		choices_open_window(&pointer);
		break;

	case PANEL_MENU_QUIT:
		main_quit_flag = TRUE;
		break;
	}
}


/**
 * Handle incoming Message_ModeChange.
 *
 * \param *message		The message data block from the Wimp.
 */

static osbool panel_message_mode_change(wimp_message *message)
{
	panel_update_mode_details();
	return TRUE;
}


/**
 * Update the details of the current screen mode.
 */

static void panel_update_mode_details(void)
{
	wimp_window_state	state;
	os_error		*error;
	int			dimension, shift;

	/* Get the screen mode dimensions. */

	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_XWIND_LIMIT, &dimension);
	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_XEIG_FACTOR, &shift);

	panel_mode_width = ((dimension + 1) << shift);
	panel_mode_x_units_per_pixel = 1 << shift;

	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_YWIND_LIMIT, &dimension);
	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_YEIG_FACTOR, &shift);

	panel_mode_height = ((dimension + 1) << shift);
	panel_mode_y_units_per_pixel = 1 << shift;

	/* Get the iconbar height. */

	state.w = wimp_ICON_BAR;
	error = xwimp_get_window_state(&state);

	panel_iconbar_height = (error == NULL) ? state.visible.y1 : sf_ICONBAR_HEIGHT;

	/* Update the bars. */

	panel_update_positions();
}


/**
 * Toggle the state of the button window, open and closed.
 *
 * \param *windat		The window to be updated.
 */

static void panel_toggle_window(struct panel_block *windat)
{
	if (windat == NULL)
		return;

	windat->panel_is_open = !windat->panel_is_open;

	panel_reopen_window(windat);
}


/**
 * Re-open the buttons window using any changed parameters which might
 * have been calculated.
 *
 * \param *windat		The window to be reopened.
 */

static void panel_reopen_window(struct panel_block *windat)
{
	wimp_window_state	state;
	os_error		*error;

	if (windat == NULL)
		return;

	state.w = windat->window;
	error = xwimp_get_window_state(&state);
	if (error != NULL)
		return;

	panel_open_window((wimp_open *) &state);
}

/**
 * Open or 'close' the buttons window to a given number of columns and
 * place it at a defined point in the window stack.
 *
 * \param *open			The Wimp_Open data for the window.
 */

static void panel_open_window(wimp_open *open)
{
	struct panel_block	*windat;
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
	state.i = PANEL_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	info.w = windat->window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	grid_size = panel_grid_spacing + (windat->grid_columns * (panel_grid_spacing + panel_grid_square));

	switch (windat->location) {
	case PANEL_POSITION_LEFT:
		sidebar_size = state.icon.extent.x1 - state.icon.extent.x0;

		open->visible.x0 = 0;
		open->visible.x1 = ((windat->panel_is_open) ? grid_size : 0) + sidebar_size;
		open->visible.y0 = windat->min_longitude;
		open->visible.y1 = windat->max_longitude;

		open->xscroll = (windat->panel_is_open) ? 0 : grid_size;
		open->yscroll = 0;
		break;

	case PANEL_POSITION_RIGHT:
		sidebar_size = state.icon.extent.x1 - state.icon.extent.x0;

		open->visible.x0 = panel_mode_width - (((windat->panel_is_open) ? grid_size : 0) + sidebar_size);
		open->visible.x1 = panel_mode_width;
		open->visible.y0 = windat->min_longitude;
		open->visible.y1 = windat->max_longitude;

		open->xscroll = 0;
		open->yscroll = 0;
		break;

	case PANEL_POSITION_TOP:
		sidebar_size = state.icon.extent.y1 - state.icon.extent.y0;

		open->visible.x0 = windat->min_longitude;
		open->visible.x1 = windat->max_longitude;
		open->visible.y0 = panel_mode_height - (((windat->panel_is_open) ? grid_size : 0) + sidebar_size);
		open->visible.y1 = panel_mode_height;

		open->xscroll = 0;
		open->yscroll = (windat->panel_is_open) ? 0 : -grid_size;
		break;

	case PANEL_POSITION_BOTTOM:
		sidebar_size = state.icon.extent.y1 - state.icon.extent.y0;

		open->visible.x0 = windat->min_longitude;
		open->visible.x1 = windat->max_longitude;
		open->visible.y0 = panel_iconbar_height;
		open->visible.y1 = panel_iconbar_height + (((windat->panel_is_open) ? grid_size : 0) + sidebar_size);

		open->xscroll = 0;
		open->yscroll = 0;
		break;

	case PANEL_POSITION_HORIZONTAL:
	case PANEL_POSITION_VERTICAL:
	case PANEL_POSITION_NONE:
		break;
	}

	open->next = (windat->panel_is_open) ? wimp_TOP : wimp_BOTTOM;

	wimp_open_window(open);
}

/**
 * Update the positions of all of the bars on the screen.
 */

static void panel_update_positions(void)
{
	struct panel_block	*windat = NULL;
	struct panel_block	**panels;
	size_t			panel_count = 0;
	os_box			locations, start_pos, next_pos, max_width, units, count;
	int			i, sidebar_width, sidebar_height;

	debug_printf("\\YUpdating the panel positions...");

	/* Count the number of panels on each side of the screen.*/

	locations.x0 = 0;
	locations.x1 = 0;
	locations.y0 = 0;
	locations.y1 = 0;

	/* Total the number of width units on each side of the screen. */

	units.x0 = 0;
	units.x1 = 0;
	units.y0 = 0;
	units.y1 = 0;

	/* Count through the panels. */

	panel_count = 0;

	windat = panel_list;

	while (windat != NULL) {
		switch (windat->location) {
		case PANEL_POSITION_LEFT:
			locations.x0++;
			units.x0 += windat->longitude_weight;
			break;
		case PANEL_POSITION_RIGHT:
			locations.x1++;
			units.x1 += windat->longitude_weight;
			break;
		case PANEL_POSITION_TOP:
			locations.y1++;
			units.y1 += windat->longitude_weight;
			break;
		case PANEL_POSITION_BOTTOM:
			locations.y0++;
			units.y0 += windat->longitude_weight;
			break;
		case PANEL_POSITION_VERTICAL:
		case PANEL_POSITION_HORIZONTAL:
		case PANEL_POSITION_NONE:
			break;
		}

		panel_count++;

		windat = windat->next;
	}

	/* Create a sorted array of panels. */

	panels = heap_alloc(sizeof(struct panel_block *) * panel_count);
	if (panels == NULL)
		return;

	windat = panel_list;
	i = 0;

	while ((windat != NULL) && (i < panel_count)) {
		panels[i++] = windat;

		windat = windat->next;
	}

	qsort(panels, panel_count, sizeof(struct panel_block *), &compare_panels);

	/* Work out the size of the sidebar icon. */

	sidebar_width = panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.x1 -
			panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.x0;

	sidebar_height = panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.y1 -
			panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.y0;

	/* The lowest position of the first bar on each side of the screen. */

	start_pos.x0 = panel_iconbar_height;
	start_pos.x1 = panel_iconbar_height;
	start_pos.y0 = 0;
	start_pos.y1 = 0;

	/* The end-to-end distance for all the bars on each side of the screen. */

	max_width.x0 = panel_mode_height - panel_iconbar_height;
	max_width.x1 = panel_mode_height - panel_iconbar_height;
	max_width.y0 = panel_mode_width;
	max_width.y1 = panel_mode_width;

	/* If there's a bar on the left, push the top and bottom bars in. */

	if (locations.x0 > 0) {
		start_pos.y0 += sidebar_width;
		start_pos.y1 += sidebar_width;

		max_width.y0 -= sidebar_width;
		max_width.y1 -= sidebar_width;
	}

	/* If there's a bar on the right, pull the top and bottom bars back. */

	if (locations.x1 > 0) {
		max_width.y0 -= sidebar_width;
		max_width.y1 -= sidebar_width;
	}

	/* If there's a bar at the bottum, push the left and right bars up. */

	if (locations.y0 > 0) {
		start_pos.x0 += sidebar_height;
		start_pos.x1 += sidebar_height;

		max_width.x0 -= sidebar_height;
		max_width.x1 -= sidebar_height;
	}

	/* If there's a bar at the top, pull the left and right bars down. */

	if (locations.y1 > 0) {
		max_width.x0 -= sidebar_height;
		max_width.x1 -= sidebar_height;
	}

	/* Track the start of the next bar on each side of the screen. */

	next_pos.x0 = start_pos.x0;
	next_pos.x1 = start_pos.x1;
	next_pos.y0 = start_pos.y0;
	next_pos.y1 = start_pos.y1;

	/* Count the number of width weight units seen so far. */

	count.x0 = 0;
	count.x1 = 0;
	count.y0 = 0;
	count.y1 = 0;

	/* Update the min and max extent for each bar, then reopen it. */

	for (i = 0; i < panel_count; i++) {
		switch (panels[i]->location) {
		case PANEL_POSITION_LEFT:
			count.x0 += panels[i]->longitude_weight;
			panels[i]->min_longitude = next_pos.x0;

			panels[i]->max_longitude = ((count.x0 * max_width.x0) / units.x0) + start_pos.x0;
			next_pos.x0 = panels[i]->max_longitude + panel_mode_y_units_per_pixel;
			break;
		case PANEL_POSITION_RIGHT:
			count.x1 += panels[i]->longitude_weight;
			panels[i]->min_longitude = next_pos.x1;

			panels[i]->max_longitude = ((count.x1 * max_width.x1) / units.x1) + start_pos.x1;
			next_pos.x1 = panels[i]->max_longitude + panel_mode_y_units_per_pixel;
			break;
		case PANEL_POSITION_TOP:
			count.y1 += panels[i]->longitude_weight;
			panels[i]->min_longitude = next_pos.y1;

			panels[i]->max_longitude = ((count.y1 * max_width.y1) / units.y1) + start_pos.y1;
			next_pos.y1 = panels[i]->max_longitude + panel_mode_x_units_per_pixel;
			break;
		case PANEL_POSITION_BOTTOM:
			count.y0 += panels[i]->longitude_weight;
			panels[i]->min_longitude = next_pos.y0;

			panels[i]->max_longitude = ((count.y0 * max_width.y0) / units.y0) + start_pos.y0;
			next_pos.y0 = panels[i]->max_longitude + panel_mode_x_units_per_pixel;
			break;
		case PANEL_POSITION_VERTICAL:
		case PANEL_POSITION_HORIZONTAL:
		case PANEL_POSITION_NONE:
			break;
		}

		panel_update_window_extent(panels[i]);
		panel_reopen_window(panels[i]);
	}

	/* Free the memory allocation. */

	heap_free(panels);
}


/**
 * Compare two entries in an array of pointers to panel instances.
 * Used as a callback function for qsort().
 *
 * \param *p			Pointer to the first array entry.
 * \param *q			Pointer to the second array entry.
 * \return			-1, 0 or +1 depending on sort order.
 */

static int compare_panels(const void *p, const void *q)
{
	const struct panel_block * const *a = p;
	const struct panel_block * const *b = q;

	if (a == NULL || *a == NULL || b == NULL || *b == NULL)
		return 0;

	return ((*a)->sort > (*b)->sort) - ((*a)->sort < (*b)->sort);
}


/**
 * Update the button window grid details to take into account new values from
 * the configuration.
 *
 * \param *windat		The window to be updated.
  */

static void panel_update_grid_info(struct panel_block *windat)
{
	if (windat == NULL)
		return;


	windat->grid_columns = config_int_read("WindowColumns");

	if (panel_grid_square + panel_grid_spacing != 0)
		windat->grid_rows = (windat->max_longitude - windat->min_longitude) /
				(panel_grid_square + panel_grid_spacing);
	else
		windat->grid_rows = 0;

	panel_update_window_extent(windat);
}

/**
 * Update the vertical position of the buttons window to take into account
 * a change of screen mode.
 *
 * \param *windat		The window to be reopened.
 */

static void panel_update_window_extent(struct panel_block *windat)
{
	int			old_window_size = 0, new_window_size, sidebar_width = 0;
	wimp_window_info	info;
	wimp_icon_state		state;
	os_error		*error;

	if (windat == NULL)
		return;

	debug_printf("Updating extent for 0x%x", windat);

	info.w = windat->window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	state.w = windat->window;
	state.i = PANEL_ICON_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	/* Update the window extent. */

	new_window_size = windat->max_longitude - windat->min_longitude;

	if (windat->location & PANEL_POSITION_VERTICAL) {
		old_window_size = info.extent.y1 - info.extent.y0;

		/* Calculate the new vertical size of the window. */

		info.extent.y1 = 0;
		info.extent.y0 = info.extent.y1 - new_window_size;

		/* Calculate the sidebar width. */

		sidebar_width = state.icon.extent.x1 - state.icon.extent.x0;

		/* Calculate the new horizontal size of the window. */

		info.extent.x0 = 0;
		info.extent.x1 = info.extent.x0 + sidebar_width + panel_grid_spacing +
				windat->grid_columns * (panel_grid_spacing + panel_grid_square);

		/* Extend the extent if required, but never bother to shrink it. */

		if (old_window_size < new_window_size) {
			error = xwimp_set_extent(windat->window, &(info.extent));
			if (error != NULL)
				return;
		}
	} else if (windat->location & PANEL_POSITION_HORIZONTAL) {
		old_window_size = info.extent.x1 - info.extent.x0;

		/* Calculate the new horizontal size of the window. */

		info.extent.x0 = 0;
		info.extent.x1 = info.extent.x0 + new_window_size;

		/* Calculate the sidebar width (height). */

		sidebar_width = state.icon.extent.y1 - state.icon.extent.y0;

		/* Calculate the new vertical size of the window. */

		info.extent.y1 = 0;
		info.extent.y0 = info.extent.y1 - sidebar_width - panel_grid_spacing -
				windat->grid_columns * (panel_grid_spacing + panel_grid_square);

		/* Extend the extent if required, but never bother to shrink it. */

		if (old_window_size < new_window_size) {
			error = xwimp_set_extent(windat->window, &(info.extent));
			if (error != NULL)
				return;
		}
	}

	/* Move the sidebar icon into its new location. */

	switch (windat->location) {
	case PANEL_POSITION_LEFT:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (panel_grid_square + panel_grid_spacing);
		windat->origin_y = info.extent.y1 - panel_grid_spacing;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x1 - sidebar_width, info.extent.y0, info.extent.x1, info.extent.y1);
		break;

	case PANEL_POSITION_RIGHT:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (panel_grid_square + panel_grid_spacing) + sidebar_width;
		windat->origin_y = info.extent.y1 - panel_grid_spacing;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x0, info.extent.y0, info.extent.x0 + sidebar_width, info.extent.y1);
		break;

	case PANEL_POSITION_TOP:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (panel_grid_square + panel_grid_spacing);
		windat->origin_y = info.extent.y1 - panel_grid_spacing;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x0, info.extent.y0, info.extent.x1, info.extent.y0 + sidebar_width);
		break;

	case PANEL_POSITION_BOTTOM:
		windat->origin_x = info.extent.x0 + windat->grid_columns * (panel_grid_square + panel_grid_spacing);
		windat->origin_y = info.extent.y1 - panel_grid_spacing;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x0, info.extent.y1 - sidebar_width, info.extent.x1, info.extent.y1);
		break;

	case PANEL_POSITION_HORIZONTAL:
	case PANEL_POSITION_VERTICAL:
	case PANEL_POSITION_NONE:
		break;
	}
}


/**
 * Create a full set of buttons from the contents of the application database.
 */

void panel_create_from_db(void)
{
	unsigned		key, panel;
	struct icondb_button	*new;
	struct panel_block	*windat;

	/* Create the panels defined in the database. */

	key = PANELDB_NULL_KEY;

	do {
		key = paneldb_get_next_key(key);

		if (key != PANELDB_NULL_KEY)
			panel_create_instance(key);
	} while (key != PANELDB_NULL_KEY);

	/* Add the buttons defined in the database. */

	key = APPDB_NULL_KEY;

	do {
		key = appdb_get_next_key(key);

		if (key != APPDB_NULL_KEY) {
			panel = appdb_get_panel(key);
			if (panel == APPDB_NULL_PANEL)
				continue;

			windat = panel_find_id(panel);
			if (windat == NULL)
				continue;

			new = icondb_create_icon(windat->icondb, key);

			if (new != NULL)
				panel_create_icon(windat, new);
		}
	} while (key != APPDB_NULL_KEY);

	panel_update_positions();
}


/**
 * Rebuild the contents of a buttons window.
 *
 * \param *windat		The window to be rebuilt.
 */

static void panel_rebuild_window(struct panel_block *windat)
{
	struct icondb_button *button = NULL;
	
	if (windat == NULL)
		return;

	button = icondb_get_list(windat->icondb);

	debug_printf("\\RRebuilding panel 0x%x", windat);

	while (button != NULL) {
		debug_printf("Recreating icon key=%u, window=0x%x, icon=%d", button->key, button->window, button->icon);
		panel_create_icon(windat, button);

		button = button->next;
	}

	panel_reopen_window(windat);
}


/**
 * Create (or recreate) an icon for a button, based on that icon's definition
 * block.
 *
 * \param *windat		The window to create the icon in.
 * \param *button		The definition to create an icon for.
 */

static void panel_create_icon(struct panel_block *windat, struct icondb_button *button)
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

	panel_icon_def.w = windat->window;

	panel_icon_def.icon.extent.x1 = windat->origin_x - entry->position.x * (panel_grid_square + panel_grid_spacing);
	panel_icon_def.icon.extent.x0 = panel_icon_def.icon.extent.x1 - panel_slab_width;
	panel_icon_def.icon.extent.y1 = windat->origin_y - entry->position.y * (panel_grid_square + panel_grid_spacing);
	panel_icon_def.icon.extent.y0 = panel_icon_def.icon.extent.y1 - panel_slab_height;

	string_printf(button->validation, ICONDB_VALIDATION_LENGTH, "R5,1;S%s;NButton", entry->sprite);
	panel_icon_def.icon.data.indirected_text_and_sprite.validation = button->validation;

	button->window = windat->window;
	button->icon = wimp_create_icon(&panel_icon_def);

	windows_redraw(windat->window);
}


/**
 * Delete a button and all its associated information.
 *
 * \param *windat		The window to delete the icon from.
 * \param *button		The button to be deleted.
 */

static void panel_delete_icon(struct panel_block *windat, struct icondb_button *button)
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
	icondb_delete_icon(windat->icondb, button);
}


/**
 * Press a button in the window.
 *
 * \param *windat		The window containing the icon.
 * \param icon			The handle of the icon being pressed.
 */

static void panel_press(struct panel_block *windat, wimp_i icon)
{
	struct appdb_entry	entry;
	char			*buffer;
	int			length;
	os_error		*error;
	struct icondb_button	*button = NULL;

	if (windat == NULL)
		return;

	button = icondb_find_icon(windat->icondb, windat->window, icon);

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

static void panel_open_edit_dialogue(wimp_pointer *pointer, struct icondb_button *button, os_coord *grid)
{
	struct appdb_entry entry;

	/* Initialise deafults if button data can't be found. */

	entry.panel = 0;
	entry.position.x = 0;
	entry.position.y = 0;
	entry.local_copy = FALSE;
	entry.filer_boot = TRUE;
	*entry.name = '\0';
	*entry.sprite = '\0';
	*entry.command = '\0';

	if (button != NULL)
		appdb_get_button_info(button->key, &entry);

	if (grid != NULL) {
		entry.position.x = grid->x;
		entry.position.y = grid->y;
	}

	edit_open_dialogue(pointer, &entry, panel_process_edit_dialogue, button);
}


/**
 * Handle the data returned from an Edit dialogue instance.
 *
 * \param *entry	Pointer to the data from the dialogue.
 * \param *data		Pointer to the button owning the dialogue, or NULL.
 * \return		TRUE if the data is OK; FALSE to reject it.
 */

static osbool panel_process_edit_dialogue(struct appdb_entry *entry, void *data)
{
	struct icondb_button *button = data;
	struct panel_block *windat = NULL;

	if (entry == NULL)
		return FALSE;

	windat = panel_find_id(entry->panel);
	if (windat == NULL) {
		error_msgs_report_error("BadPanel");
		return FALSE;
	}

	/* Validate the button location. */

	if (entry->position.x < 0 || entry->position.y < 0 || entry->position.x >= windat->grid_columns || entry->position.y >= windat->grid_rows) {
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
		button = icondb_create_icon(windat->icondb, appdb_create_key());

	if (button == NULL) {
		error_msgs_report_error("NoMemNewButton");
		return FALSE;
	}

	/* Store the application in the database. */

	appdb_set_button_info(button->key, entry);

	panel_create_icon(windat, button);

	return TRUE;
}


/**
 * Given a panel id number, return the associated panel data block.
 *
 * \param id		The Panel Id to locate.
 * \return		Pointer to the associated block, or NULL.
 */

static struct panel_block *panel_find_id(unsigned id)
{
	struct panel_block *windat = panel_list;

	while (windat != NULL && windat->panel_id != id)
		windat = windat->next;

	return windat;
}

