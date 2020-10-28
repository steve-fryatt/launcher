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
 * \file: panel.c
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

#include "panel.h"

#include "appdb.h"
#include "choices.h"
#include "edit_button.h"
#include "edit_panel.h"
#include "filing.h"
#include "icondb.h"
#include "main.h"
#include "objutil.h"
#include "paneldb.h"


/* Button Window */

#define PANEL_ICON_SIDEBAR 0
#define PANEL_ICON_TEMPLATE 1

/* Main Menu */

#define PANEL_MENU_INFO 0
#define PANEL_MENU_HELP 1
#define PANEL_MENU_BUTTON 2
#define PANEL_MENU_NEW_BUTTON 3
#define PANEL_MENU_PANEL 4
#define PANEL_MENU_NEW_PANEL 5
#define PANEL_MENU_SAVE_LAYOUT 6
#define PANEL_MENU_CHOICES 7
#define PANEL_MENU_QUIT 8

/* Button Submenu */

#define PANEL_MENU_BUTTON_EDIT 0
#define PANEL_MENU_BUTTON_DELETE 1

/* Panel Submenu */

#define PANEL_MENU_PANEL_EDIT 0
#define PANEL_MENU_PANEL_DELETE 1

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
 * The reasons that a panel might be on display.
 */

enum panel_status {
	PANEL_STATUS_CLOSED = 0,
	PANEL_STATUS_POINTER_OVER = 1,
	PANEL_STATUS_MENU_OPEN = 2,
	PANEL_STATUS_PANEL_DLOG_OPEN = 4,
	PANEL_STATUS_BUTTON_DLOG_OPEN = 8
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

	os_coord grid_dimensions;

	/**
	 * The origin of the button grid (in OS units).
	 */

	os_coord origin;

	/**
	 * Indicate whether the window is currently "open" (TRUE) or "closed" (FALSE).
	 */

	osbool panel_is_open;

	/**
	 * Track reasons why a panel might be open.
	 */

	enum panel_status open_status;

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
	 * The size of a grid square (in OS units).
	 */

	int grid_square;

	/**
	 * The spacing between grid squares (in OS units).
	 */
	int grid_spacing;

	/**
	 * The dimensions of one button slab (in grid squares).
	 */

	os_coord slab_grid_dimensions;

	/**
	 * The dimensions of one button slab (in OS units).
	 */

	os_coord slab_os_dimensions;

	/**
	 * Does the panel respond to mouseover events.
	 */

	osbool auto_mouseover;

	/**
	 * The time, in centiseconds, before an auto-open occurs.
	 */

	os_t auto_open_delay;

	/**
	 * The time, in centiseconds, before an auto-close occurs.
	 */

	os_t auto_close_delay;

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
 * The width of the panel sidebar on vertical panels.
 */
static int panel_sidebar_width;

/**
 * The height of the panel sidebar on horizontal panels.
 */
static int panel_sidebar_height;

/**
 * The handle of the main menu.
 */

static wimp_menu *panel_menu = NULL;

static wimp_menu *panel_sub_menu = NULL;

/**
 * The block for the icon over which the main menu last opened.
 */

static struct icondb_button *panel_menu_icon = NULL;

/**
 * The grid coordinates where the main menu last opened.
 */

static os_coord panel_menu_coordinate;

/* Static Function Prototypes. */

static struct panel_block *panel_create_instance(unsigned key);
static void panel_delete_instance(struct panel_block *windat);
static void panel_update_instance_from_db(struct panel_block *windat);

static void panel_click_handler(wimp_pointer *pointer);
static void panel_pointer_entering_handler(wimp_entering *entering);
static osbool panel_pointer_entering_callback(os_t time, void *data);
static void panel_pointer_leaving_handler(wimp_leaving *leaving);
static osbool panel_pointer_leaving_callback(os_t time, void *data);
static void panel_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void panel_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void panel_menu_close(wimp_w w, wimp_menu *menu);
static osbool panel_message_mode_change(wimp_message *message);

static void panel_update_mode_details(void);

static void panel_toggle_window(struct panel_block *windat);
static void panel_reopen_window(struct panel_block *windat);
static void panel_open_window(wimp_open *open);
static void panel_update_positions(void);
static int compare_panels(const void *p, const void *q);
static void panel_update_window_extent(struct panel_block *windat);

static void panel_update_grid_info(struct panel_block *windat);

static void panel_add_buttons_from_db(struct panel_block *windat);
static void panel_reflow_buttons(struct panel_block *windat);
static void panel_rebuild_window(struct panel_block *windat);
static void panel_empty_window(struct panel_block *windat);

static void panel_create_icon(struct panel_block *windat, struct icondb_button *button);
static void panel_press(struct panel_block *windat, wimp_i icon);

static void panel_open_panel_dialogue(wimp_pointer *pointer, struct panel_block *windat);
static osbool panel_process_panel_dialogue(struct paneldb_entry *app, void *data);
static osbool panel_delete_panel(struct panel_block *windat);

static void panel_open_button_dialogue(wimp_pointer *pointer, struct panel_block *windat, struct icondb_button *button, os_coord *grid);
static osbool panel_process_button_dialogue(struct appdb_entry *entry, void *data);
static osbool panel_delete_button(struct panel_block *windat, struct icondb_button *button);

static struct panel_block *panel_find_id(unsigned id);


/**
 * Initialise the buttons window.
 */

void panel_initialise(void)
{
	/* Initialise the menus used in the window. */

	panel_menu = templates_get_menu("MainMenu");
	ihelp_add_menu(panel_menu, "MainMenu");

	panel_sub_menu = templates_get_menu("PanelMenu");

	/* Initialise the main Buttons window template. */

	panel_window_def = templates_load_window("Launch");
	if (panel_window_def == NULL)
		error_msgs_report_fatal("BadTemplate");

	panel_window_def->icon_count = 1;

	panel_icon_def.icon = panel_window_def->icons[PANEL_ICON_TEMPLATE];

	/* Work out the size of the sidebar icon. */

	panel_sidebar_width = panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.x1 -
			panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.x0;

	panel_sidebar_height = panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.y1 -
			panel_window_def->icons[PANEL_ICON_SIDEBAR].extent.y0;

	/* Watch out for Message_ModeChange. */

	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, panel_message_mode_change);

	/* Initialise the Edit dialogues. */

	edit_panel_initialise();
	edit_button_initialise();

	/* Correctly size the window for the current mode. */

	panel_update_mode_details();
	panel_refresh_choices();
}


/**
 * Terminate the buttons window.
 */

void panel_terminate(void)
{
	while (panel_list != NULL)
		panel_delete_instance(panel_list);

	icondb_terminate();
	edit_panel_terminate();
	edit_button_terminate();
}


/**
 * Create a new panel instance, using a given database entry.
 * This does not open the panel.
 *
 * \param key			The database key to use.
 * \return			The new instance, or NULL.
 */

static struct panel_block *panel_create_instance(unsigned key)
{
	struct panel_block *new;

	new = heap_alloc(sizeof(struct panel_block));
	if (new == NULL)
		return NULL;

	new->panel_id = key;
	new->grid_columns = 0;
	new->grid_dimensions.x = 0;
	new->grid_dimensions.y = 0;
	new->origin.x = 0;
	new->origin.y = 0;
	new->panel_is_open = FALSE;
	new->open_status = PANEL_STATUS_CLOSED;
	new->min_longitude = 0;
	new->max_longitude = 0;
	new->auto_mouseover = config_opt_read("MouseOver");
	new->auto_open_delay = config_int_read("OpenDelay");
	new->auto_close_delay = 10;

	new->icondb = icondb_create_instance();

	new->window = wimp_create_window(panel_window_def);
	ihelp_add_window(new->window, "Launch", NULL);
	event_add_window_user_data(new->window, new);
	event_add_window_open_event(new->window, panel_open_window);
	event_add_window_mouse_event(new->window, panel_click_handler);
	event_add_window_pointer_entering_event(new->window, panel_pointer_entering_handler);
	event_add_window_pointer_leaving_event(new->window, panel_pointer_leaving_handler);
	event_add_window_menu(new->window, panel_menu);
	event_add_window_menu_prepare(new->window, panel_menu_prepare);
	event_add_window_menu_selection(new->window, panel_menu_selection);
	event_add_window_menu_close(new->window, panel_menu_close);

	/* Link the window in to the data structure. */

	new->next = panel_list;
	panel_list = new;

	return new;
}


/**
 * Delete a panel instance.
 * 
 * \param *windat		The panel instance to delete.
 */

static void panel_delete_instance(struct panel_block *windat)
{
	unsigned		key, last_key;
	struct panel_block	*parent;
	struct appdb_entry	*app;

	if (windat == NULL)
		return;

	event_delete_window(windat->window);
	ihelp_remove_window(windat->window);
	wimp_delete_window(windat->window);

	/* Delete the applications from the database. */

	key = appdb_get_next_key(APPDB_NULL_KEY);

	while (key != APPDB_NULL_KEY) {
		last_key = key;

		key = appdb_get_next_key(key);

		app = appdb_get_button_info(last_key, NULL);
		if (app != NULL && app->panel == windat->panel_id)
			appdb_delete_key(last_key);
	}

	/* Delete the panel from the database. */

	paneldb_delete_key(windat->panel_id);

	/* Delete the icon databse instance. */

	icondb_destroy_instance(windat->icondb);

	/* Delink the panel from the list. */

	if (panel_list == windat) {
		panel_list = windat->next;
	} else {
		for (parent = panel_list; parent != NULL && parent->next != windat; parent = parent->next);

		if (parent != NULL && parent->next == windat)
			parent->next = windat->next;
	}

	/* Free the memory. */

	heap_free(windat);
}


/**
 * Update a panel instance from the panel database.
 * 
 * \param *windat		The panel instance to update.
 */

static void panel_update_instance_from_db(struct panel_block *windat)
{
	struct paneldb_entry *panel;

	if (windat == NULL)
		return;

	panel = paneldb_get_panel_info(windat->panel_id, NULL);
	if (panel == NULL)
		return;

	switch (panel->position) {
	case PANELDB_POSITION_LEFT:
		windat->location = PANEL_POSITION_LEFT;
		break;
	case PANELDB_POSITION_RIGHT:
		windat->location = PANEL_POSITION_RIGHT;
		break;
	case PANELDB_POSITION_TOP:
		windat->location = PANEL_POSITION_TOP;
		break;
	case PANELDB_POSITION_BOTTOM:
		windat->location = PANEL_POSITION_BOTTOM;
		break;
	default:
		windat->location = PANEL_POSITION_LEFT;
		break;
	}

	windat->longitude_weight = panel->width;
	windat->sort = panel->sort;
}


/**
 * Inform the buttons module that the global system choices have changed,
 * and force an update of any relevant parameters.
 */

void panel_refresh_choices(void)
{
	struct panel_block *windat = panel_list;

	while (windat != NULL) {
		panel_update_grid_info(windat);
		panel_reflow_buttons(windat);
		panel_update_window_extent(windat);
		panel_rebuild_window(windat);

		windat->auto_mouseover = config_opt_read("MouseOver");
		windat->auto_open_delay = config_int_read("OpenDelay");

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
 * Process pointer entering events for a panel.
 *
 * \param *entering		The pointer entering event block.
 */

static void panel_pointer_entering_handler(wimp_entering *entering)
{
	struct panel_block *windat;

	if (entering == NULL)
		return;

	windat = event_get_window_user_data(entering->w);
	if (windat == NULL)
		return;

	windat->open_status |= PANEL_STATUS_POINTER_OVER;

	if (windat->auto_mouseover && windat->open_status == PANEL_STATUS_POINTER_OVER)
		event_add_single_callback(entering->w, windat->auto_open_delay, panel_pointer_entering_callback, windat);
}


/**
 * Callback to check the position of the pointer after
 * the auto-open delay has elapsed.
 *
 * \param time			The time that the callback occurred.
 * \param *data			The window instance of interest.
 * \return			TRUE if the callback was complete.
 */

static osbool panel_pointer_entering_callback(os_t time, void *data)
{
	struct panel_block *windat = data;

	if (windat == NULL || !windat->auto_mouseover || windat->open_status != PANEL_STATUS_POINTER_OVER)
		return TRUE;

	windat->panel_is_open = TRUE;

	panel_reopen_window(windat);

	return TRUE;
}


/**
 * Process pointer leaving events for a panel.
 *
 * \param *leaving		The pointer leaving event block.
 */

static void panel_pointer_leaving_handler(wimp_leaving *leaving)
{
	struct panel_block *windat;

	if (leaving == NULL)
		return;

	windat = event_get_window_user_data(leaving->w);
	if (windat == NULL)
		return;

	windat->open_status &= ~PANEL_STATUS_POINTER_OVER;

	if (windat->auto_mouseover)
		event_add_single_callback(leaving->w, windat->auto_close_delay, panel_pointer_leaving_callback, windat);
}


/**
 * Callback to check the position of the pointer after
 * the auto-close delay has elapsed.
 *
 * \param time			The time that the callback occurred.
 * \param *data			The window instance of interest.
 * \return			TRUE if the callback was complete.
 */

static osbool panel_pointer_leaving_callback(os_t time, void *data)
{
	struct panel_block *windat = data;

	if (!windat->auto_mouseover || !windat->auto_mouseover || windat->open_status != PANEL_STATUS_CLOSED)
		return TRUE;

	windat->panel_is_open = FALSE;

	panel_reopen_window(windat);

	return TRUE;
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
	os_coord		click;


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
	menus_shade_entry(panel_sub_menu, PANEL_MENU_PANEL_DELETE, (panel_list == NULL || panel_list->next == NULL) ? TRUE : FALSE);

	window.w = w;
	wimp_get_window_state(&window);

	/* Find the click position in work area coordinates. */

	click.x = (pointer->pos.x - window.visible.x0) + window.xscroll;
	click.y = (pointer->pos.y - window.visible.y1) + window.yscroll;

	/* Adjust to OS units in the grid orientation. */

	switch(windat->location) {
	case PANEL_POSITION_LEFT:
		panel_menu_coordinate.x = windat->origin.x - click.x;
		panel_menu_coordinate.y = windat->origin.y - click.y;
		break;

	case PANEL_POSITION_RIGHT:
		panel_menu_coordinate.x = click.x - windat->origin.x;
		panel_menu_coordinate.y = windat->origin.y - click.y;
		break;

	case PANEL_POSITION_TOP:
		panel_menu_coordinate.x = click.y - windat->origin.y;
		panel_menu_coordinate.y = click.x - windat->origin.x;
		break;

	case PANEL_POSITION_BOTTOM:
		panel_menu_coordinate.x = windat->origin.y - click.y;
		panel_menu_coordinate.y = click.x - windat->origin.x;
		break;

	case PANEL_POSITION_HORIZONTAL:
	case PANEL_POSITION_VERTICAL:
	case PANEL_POSITION_NONE:
		break;
	}

	/* Convert to grid squares. */

	panel_menu_coordinate.x = panel_menu_coordinate.x / (windat->grid_square + windat->grid_spacing);
	panel_menu_coordinate.y = panel_menu_coordinate.y / (windat->grid_square + windat->grid_spacing);

	/* Track that the menu is open. */

	windat->open_status |= PANEL_STATUS_MENU_OPEN;
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
				panel_open_button_dialogue(&pointer, windat, panel_menu_icon, NULL);
				break;

			case PANEL_MENU_BUTTON_DELETE:
				if (panel_delete_button(windat, panel_menu_icon))
					panel_menu_icon = NULL;
				break;
			}
		}
		break;

	case PANEL_MENU_NEW_BUTTON:
		panel_open_button_dialogue(&pointer, windat, panel_menu_icon, &panel_menu_coordinate);
		break;

	case PANEL_MENU_PANEL:
		switch (selection->items[1]) {
		case PANEL_MENU_PANEL_EDIT:
			panel_open_panel_dialogue(&pointer, windat);
			break;

		case PANEL_MENU_PANEL_DELETE:
			panel_delete_panel(windat);
			break;
		}
		break;

	case PANEL_MENU_NEW_PANEL:
		panel_open_panel_dialogue(&pointer, NULL);
		break;

	case PANEL_MENU_SAVE_LAYOUT:
		filing_save("Buttons");
		break;

	case PANEL_MENU_CHOICES:
		choices_open_window(&pointer);
		break;

	case PANEL_MENU_QUIT:
		if (!main_check_for_unsaved_data())
			main_quit_flag = TRUE;
		break;
	}
}


/**
 * Handles the menu tree closing.
 */

static void panel_menu_close(wimp_w w, wimp_menu *menu)
{
	struct panel_block	*windat;

	windat = event_get_window_user_data(w);
	if (windat == NULL)
		return;

	windat->open_status &= ~PANEL_STATUS_MENU_OPEN;

	if (windat->auto_mouseover)
		event_add_single_callback(w, windat->auto_close_delay, panel_pointer_leaving_callback, windat);
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

	grid_size = windat->grid_spacing + (windat->grid_dimensions.x * (windat->grid_spacing + windat->grid_square));

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
	int			i;

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
		start_pos.y0 += panel_sidebar_width;
		start_pos.y1 += panel_sidebar_width;

		max_width.y0 -= panel_sidebar_width;
		max_width.y1 -= panel_sidebar_width;
	}

	/* If there's a bar on the right, pull the top and bottom bars back. */

	if (locations.x1 > 0) {
		max_width.y0 -= panel_sidebar_width;
		max_width.y1 -= panel_sidebar_width;
	}

	/* If there's a bar at the bottum, push the left and right bars up. */

	if (locations.y0 > 0) {
		start_pos.x0 += panel_sidebar_height;
		start_pos.x1 += panel_sidebar_height;

		max_width.x0 -= panel_sidebar_height;
		max_width.x1 -= panel_sidebar_height;
	}

	/* If there's a bar at the top, pull the left and right bars down. */

	if (locations.y1 > 0) {
		max_width.x0 -= panel_sidebar_height;
		max_width.x1 -= panel_sidebar_height;
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

		panel_update_grid_info(panels[i]);
		panel_reflow_buttons(panels[i]);
		panel_update_window_extent(panels[i]);
		panel_rebuild_window(panels[i]);
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

	/* Update the user choices. */

	windat->grid_square = config_int_read("GridSize");
	windat->grid_spacing = config_int_read("GridSpacing");
	windat->grid_columns = config_int_read("WindowColumns");
	windat->slab_grid_dimensions.x = config_int_read("SlabXSize");
	windat->slab_grid_dimensions.y = config_int_read("SlabYSize");

	/* Update the slab size in OS units. */

	windat->slab_os_dimensions.x = (windat->slab_grid_dimensions.x * windat->grid_square) + windat->grid_spacing;
	windat->slab_os_dimensions.y = (windat->slab_grid_dimensions.y * windat->grid_square) + windat->grid_spacing;

	/* Calculate the number of rows in the panel at the current slab size. */

	windat->grid_dimensions.x = windat->grid_columns;

	if (windat->grid_square + windat->grid_spacing != 0)
		windat->grid_dimensions.y = (windat->max_longitude - windat->min_longitude) /
				(windat->grid_square + windat->grid_spacing);
	else
		windat->grid_dimensions.y = 0;
}

/**
 * Update the vertical position of the buttons window to take into account
 * a change of screen mode.
 *
 * \param *windat		The window to be reopened.
 */

static void panel_update_window_extent(struct panel_block *windat)
{
	int			new_window_size;
	wimp_window_info	info;
	os_error		*error;

	if (windat == NULL)
		return;

	info.w = windat->window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	/* Update the window extent. */

	new_window_size = windat->max_longitude - windat->min_longitude;

	if (windat->location & PANEL_POSITION_VERTICAL) {

		/* Calculate the new vertical size of the window. */

		info.extent.y1 = 0;
		info.extent.y0 = info.extent.y1 - new_window_size;

		/* Calculate the new horizontal size of the window. */

		info.extent.x0 = 0;
		info.extent.x1 = info.extent.x0 + panel_sidebar_width + windat->grid_spacing +
				windat->grid_dimensions.x * (windat->grid_spacing + windat->grid_square);
	} else if (windat->location & PANEL_POSITION_HORIZONTAL) {

		/* Calculate the new horizontal size of the window. */

		info.extent.x0 = 0;
		info.extent.x1 = info.extent.x0 + new_window_size;

		/* Calculate the new vertical size of the window. */

		info.extent.y1 = 0;
		info.extent.y0 = info.extent.y1 - panel_sidebar_height - windat->grid_spacing -
				windat->grid_dimensions.x * (windat->grid_spacing + windat->grid_square);
	}

	/* Update the extent. */

	error = xwimp_set_extent(windat->window, &(info.extent));
	if (error != NULL)
		return;

	/* Move the sidebar icon into its new location. */

	switch (windat->location) {
	case PANEL_POSITION_LEFT:
		windat->origin.x = info.extent.x1 - windat->grid_spacing - panel_sidebar_width;
		windat->origin.y = info.extent.y1 - windat->grid_spacing;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x1 - panel_sidebar_width, info.extent.y0, info.extent.x1, info.extent.y1);
		break;

	case PANEL_POSITION_RIGHT:
		windat->origin.x = info.extent.x0 + windat->grid_spacing + panel_sidebar_width;
		windat->origin.y = info.extent.y1 - windat->grid_spacing;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x0, info.extent.y0, info.extent.x0 + panel_sidebar_width, info.extent.y1);
		break;

	case PANEL_POSITION_TOP:
		windat->origin.x = info.extent.x0 + windat->grid_spacing;
		windat->origin.y = info.extent.y0 + windat->grid_spacing + panel_sidebar_height;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x0, info.extent.y0, info.extent.x1, info.extent.y0 + panel_sidebar_height);
		break;

	case PANEL_POSITION_BOTTOM:
		windat->origin.x = info.extent.x0 + windat->grid_spacing;
		windat->origin.y = info.extent.y1 - windat->grid_spacing - panel_sidebar_height;

		xwimp_resize_icon(windat->window, PANEL_ICON_SIDEBAR,
			info.extent.x0, info.extent.y1 - panel_sidebar_height, info.extent.x1, info.extent.y1);
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
	unsigned		key;
	struct panel_block	*windat = NULL;

	/* Create the panels defined in the database. */

	key = PANELDB_NULL_KEY;

	do {
		key = paneldb_get_next_key(key);

		if (key == PANELDB_NULL_KEY)
			continue;

		windat = panel_find_id(key);

		if (windat == NULL)
			windat = panel_create_instance(key);

		if (windat == NULL)
			continue;

		panel_update_instance_from_db(windat);
		panel_add_buttons_from_db(windat);
	} while (key != PANELDB_NULL_KEY);


	panel_update_positions();
}

/**
 * Add the buttons to a panel's icon database from the main
 * application database.
 * 
 * \param *windat		The panel to be updated.
 */

static void panel_add_buttons_from_db(struct panel_block *windat)
{
	unsigned		key, panel;
	struct appdb_entry	app;

	panel_empty_window(windat);
	icondb_reset_instance(windat->icondb);

	/* Add the icons to the database one by one. */

	key = APPDB_NULL_KEY;

	do {
		key = appdb_get_next_key(key);

		if (key != APPDB_NULL_KEY) {
			panel = appdb_get_panel(key);
			if (panel == APPDB_NULL_PANEL)
				continue;

			if (windat->panel_id != panel)
				continue;

			if (appdb_get_button_info(key, &app) == NULL)
				continue;

			icondb_create_icon(windat->icondb, key, &(app.position));
		}
	} while (key != APPDB_NULL_KEY);
}


/**
 * Reflow the buttons in a panel, to reflect the available space
 * on the grid.
 * 
 * \param *windat		The panel to be reflowed.
 */

static void panel_reflow_buttons(struct panel_block *windat)
{
	struct icondb_button	*button = NULL, *previous = NULL;
	struct appdb_entry	*app = NULL;
	os_coord		overflow;
	osbool			clash;
	
	if (windat == NULL)
		return;

	/* Start the grid at the configured width. */

	windat->grid_dimensions.x = windat->grid_columns;

	/* Start to place overflow buttons top-left. */

	overflow.x = 0;
	overflow.y = 0;

	/* Process the icons. */

	button = icondb_get_list(windat->icondb);

	while (button != NULL) {
		app = appdb_get_button_info(button->key, NULL);
		if (app != NULL) {
			button->position.x = app->position.x;
			button->position.y = app->position.y;

			/* Do a bounds check on all the buttons above and to the left. */

			previous = icondb_get_list(windat->icondb);

			while (previous != NULL && previous != button) {
				if ((button->position.x > previous->position.x) &&
						(button->position.x < (previous->position.x + windat->slab_grid_dimensions.x)) &&
						(button->position.y >= previous->position.y) &&
						(button->position.y < (previous->position.y + windat->slab_grid_dimensions.y))) {
					button->position.x = previous->position.x + windat->slab_grid_dimensions.x;
				}

				if ((button->position.y >= previous->position.y) &&
						(button->position.y < (previous->position.y + windat->slab_grid_dimensions.y)) &&
						(button->position.x > (previous->position.x - windat->slab_grid_dimensions.x)) &&
						(button->position.x <= previous->position.x)) {
					button->position.y = previous->position.y + windat->slab_grid_dimensions.y;
				}

				previous = previous->next;
			}

			/* Does the button fall outside the configured rows?
			 *
			 * We know by now that we've reached the bottom of the grid in all
			 * columns, due to the sort order, so we're just looking for spaces
			 * in the layout working right in columns from top to bottom.
			 */

			if ((button->position.y + windat->slab_grid_dimensions.y) > windat->grid_dimensions.y) {
				do {
					clash = FALSE;

					previous = icondb_get_list(windat->icondb);

					while (previous != NULL && previous != button) {
						if ((overflow.x > (previous->position.x - windat->slab_grid_dimensions.x)) &&
								(overflow.x < (previous->position.x + windat->slab_grid_dimensions.x)) &&
								(overflow.y > (previous->position.y - windat->slab_grid_dimensions.y)) &&
								(overflow.y < (previous->position.y + windat->slab_grid_dimensions.y)))
							clash = TRUE;	

						previous = previous->next;
					}

					if (clash) {
						overflow.y++;

						if ((overflow.y + windat->slab_grid_dimensions.y) > windat->grid_dimensions.y) {
							overflow.x++;
							overflow.y = 0;
						}
					}
				} while (clash);
	
				button->position.x = overflow.x;
				button->position.y = overflow.y;
			}

			/* Does the button fall outside the colfigured columns? */

			if ((button->position.x + windat->slab_grid_dimensions.x) > windat->grid_dimensions.x)
				windat->grid_dimensions.x = button->position.x + windat->slab_grid_dimensions.x;
		}

		button = button->next;
	}
}

/**
 * Rebuild the contents of a panel. This should be done after updating the
 * panel's extent, so that icon origins are correct.
 *
 * \param *windat		The window to be rebuilt.
 */

static void panel_rebuild_window(struct panel_block *windat)
{
	struct icondb_button *button = NULL;
	
	if (windat == NULL)
		return;

	button = icondb_get_list(windat->icondb);

	while (button != NULL) {
		panel_create_icon(windat, button);

		button = button->next;
	}

	panel_reopen_window(windat);

	windows_redraw(windat->window);
}

/**
 * Remove the buttons from within a panel.
 * 
 * \param *windat		The panel to be emptied.
 */

static void panel_empty_window(struct panel_block *windat)
{
	struct icondb_button	*button = NULL;
	os_error		*error = NULL;
	
	if (windat == NULL)
		return;

	button = icondb_get_list(windat->icondb);

	while (button != NULL) {
		if (button->icon != wimp_ICON_WINDOW) {
			error = xwimp_delete_icon(windat->window, button->icon);
			if (error != NULL)
				error_report_program(error);
		}

		button = button->next;
	}
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
	struct appdb_entry	*app = NULL;

	if (windat == NULL || button == NULL)
		return;

	if (button->icon != -1) {
		error = xwimp_delete_icon(windat->window, button->icon);
		if (error != NULL)
			error_report_program(error);
		button->icon = -1;
	}

	app = appdb_get_button_info(button->key, NULL);
	if (app == NULL)
		return;

	panel_icon_def.w = windat->window;

	switch (windat->location) {
	case PANEL_POSITION_LEFT:
		panel_icon_def.icon.extent.x1 = windat->origin.x - button->position.x * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.y1 = windat->origin.y - button->position.y * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.x0 = panel_icon_def.icon.extent.x1 - windat->slab_os_dimensions.x;
		panel_icon_def.icon.extent.y0 = panel_icon_def.icon.extent.y1 - windat->slab_os_dimensions.y;
		break;

	case PANEL_POSITION_RIGHT:
		panel_icon_def.icon.extent.x0 = windat->origin.x + button->position.x * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.y1 = windat->origin.y - button->position.y * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.x1 = panel_icon_def.icon.extent.x0 + windat->slab_os_dimensions.x;
		panel_icon_def.icon.extent.y0 = panel_icon_def.icon.extent.y1 - windat->slab_os_dimensions.y;
		break;

	case PANEL_POSITION_TOP:
		panel_icon_def.icon.extent.x0 = windat->origin.x + button->position.y * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.y0 = windat->origin.y + button->position.x * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.x1 = panel_icon_def.icon.extent.x0 + windat->slab_os_dimensions.x;
		panel_icon_def.icon.extent.y1 = panel_icon_def.icon.extent.y0 + windat->slab_os_dimensions.y;
		break;

	case PANEL_POSITION_BOTTOM:
		panel_icon_def.icon.extent.x0 = windat->origin.x + button->position.y * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.y1 = windat->origin.y - button->position.x * (windat->grid_square + windat->grid_spacing);
		panel_icon_def.icon.extent.x1 = panel_icon_def.icon.extent.x0 + windat->slab_os_dimensions.x;
		panel_icon_def.icon.extent.y0 = panel_icon_def.icon.extent.y1 - windat->slab_os_dimensions.y;
		break;

	case PANEL_POSITION_HORIZONTAL:
	case PANEL_POSITION_VERTICAL:
	case PANEL_POSITION_NONE:
		break;
	}

	string_printf(button->validation, ICONDB_VALIDATION_LENGTH, "R5,1;S%s;NButton", app->sprite);
	panel_icon_def.icon.data.indirected_text_and_sprite.validation = button->validation;

	button->window = windat->window;
	button->icon = wimp_create_icon(&panel_icon_def);
}


/**
 * Press a button in the window.
 *
 * \param *windat		The window containing the icon.
 * \param icon			The handle of the icon being pressed.
 */

static void panel_press(struct panel_block *windat, wimp_i icon)
{
	struct appdb_entry	app;
	struct icondb_button	*button = NULL;

	if (windat == NULL)
		return;

	button = icondb_find_icon(windat->icondb, windat->window, icon);

	if (button == NULL)
		return;

	if (appdb_get_button_info(button->key, &app) == NULL)
		return;

	objutil_launch(app.command);

	return;
}

/*
 * Open an edit dialogue box for a panel.
 *
 * \param *pointer	The pointer coordinates at which to open the dialogue.
 * \param *windat	The panel to open the dialogue for, or NULL to
 *			open a blank dialogue.
 */

static void panel_open_panel_dialogue(wimp_pointer *pointer, struct panel_block *windat)
{
	struct paneldb_entry panel;

	paneldb_set_defaults(&panel);

	if (windat != NULL)
		paneldb_get_panel_info(windat->panel_id, &panel);

	edit_panel_open_dialogue(pointer, &panel, panel_process_panel_dialogue, windat);
}


/**
 * Handle the data returned from an edit panel dialogue instance.
 *
 * \param *panel	Pointer to the data from the dialogue.
 * \param *data		Pointer to the panel owning the dialogue, or NULL.
 * \return		TRUE if the data is OK; FALSE to reject it.
 */

static osbool panel_process_panel_dialogue(struct paneldb_entry *panel, void *data)
{
	struct panel_block	*windat = data;
	unsigned		key = PANELDB_NULL_KEY;


	/* Validate the panel name. */

	if (*(panel->name) == '\0') {
		error_msgs_report_info("MissingName");
		return FALSE;
	}

	key = paneldb_key_from_name(panel->name);

	if (((windat == NULL) || (key != windat->panel_id)) && (key != PANELDB_NULL_KEY)) {
		error_msgs_param_report_info("DuplicateName", panel->name, NULL, NULL, NULL);
		return FALSE;

	}

	if (panel->sort < 1 || panel->sort > 9999) {
		error_msgs_report_info("SortRange");
		return FALSE;
	}

	if (panel->width < 1 || panel->width > 9999) {
		error_msgs_report_info("WidthRange");
		return FALSE;
	}

	/* If this is a new panel, create its entry and get a database key.
	 */

	if (windat == NULL) {
		key = paneldb_create_key();
		if (key == PANELDB_NULL_KEY) {
			error_msgs_report_error("NoMemNewPanel");
			return FALSE;
		}
	} else {
		key = windat->panel_id;
	}

	/* Store the panel in the database. */

	paneldb_set_panel_info(key, panel);

	panel_create_from_db();

	return TRUE;
}


/**
 * Handle a request to delete a panel.
 *
 * \param *windat	The panel instance to be deleted.
 * \return		TRUE if the panel was deleted; else FALSE.
 */

static osbool panel_delete_panel(struct panel_block *windat)
{
	if (panel_list == NULL || panel_list->next == NULL) {
		error_msgs_report_info("LastPanelDelete");
		return FALSE;
	}

	panel_delete_instance(windat);

	panel_create_from_db();

	return TRUE;
}


/*
 * Open an edit dialogue box for a button.
 *
 * \param *pointer	The pointer coordinates at which to open the dialogue.
 * \param *windat	The panel to open the dialogue for.
 * \param *button	The button to open the dialogue for, or NULL to
 *			open a blank dialogue.
 * \param *grid		The coordinates for a blank dialogue.
 */

static void panel_open_button_dialogue(wimp_pointer *pointer, struct panel_block *windat, struct icondb_button *button, os_coord *grid)
{
	struct appdb_entry app;
	osbool reflowed = FALSE;

	/* Initialise deafults if button data can't be found. */

	appdb_set_defaults(&app);

	app.panel = windat->panel_id;

	/* Bring in the settings for the target button. */

	if (button != NULL) {
		appdb_get_button_info(button->key, &app);

		reflowed = (app.position.x != button->position.x) || (app.position.y != button->position.y);
	}

	/* Apply the local grid override for new buttons. */

	if (grid != NULL) {
		app.position.x = grid->x;
		app.position.y = grid->y;
	}

	edit_button_open_dialogue(pointer, &app, reflowed, panel_process_button_dialogue, button);
}


/**
 * Handle the data returned from an edit button dialogue instance.
 *
 * \param *app		Pointer to the data from the dialogue.
 * \param *data		Pointer to the button owning the dialogue, or NULL.
 * \return		TRUE if the data is OK; FALSE to reject it.
 */

static osbool panel_process_button_dialogue(struct appdb_entry *app, void *data)
{
	struct icondb_button	*button = data;
	struct panel_block	*windat = NULL;
	unsigned		key = APPDB_NULL_KEY;

	if (app == NULL)
		return FALSE;

	windat = panel_find_id(app->panel);
	if (windat == NULL) {
		error_msgs_report_error("BadPanel");
		return FALSE;
	}

	/* Validate the button location. */

	if (app->position.x < 0 || app->position.y < 0 || app->position.x >= windat->grid_columns || app->position.y >= windat->grid_dimensions.y) {
		error_msgs_report_info("CoordRange");
		return FALSE;
	}


	if (*(app->name) == '\0' || *(app->sprite) == '\0' || *(app->command) == '\0') {
		error_msgs_report_info("MissingText");
		return FALSE;
	}


	/* If this is a new button, create its entry and get a database
	 * key for the application details.
	 */

	if (button == NULL) {
		key = appdb_create_key();
		if (key == APPDB_NULL_KEY) {
			error_msgs_report_error("NoMemNewButton");
			return FALSE;
		}
	} else {
		key = button->key;
	}

	/* Store the application in the database. */

	appdb_set_button_info(key, app);

	panel_add_buttons_from_db(windat);
	panel_reflow_buttons(windat);
	panel_update_window_extent(windat);
	panel_rebuild_window(windat);

	return TRUE;
}


/**
 * Handle a request to delete a button from a panel.
 *
 * \param *windat	The panel instance holding the button.
 * \param *button	The button to be deleted.
 * \return		TRUE if the button was deleted; else FALSE.
 */

static osbool panel_delete_button(struct panel_block *windat, struct icondb_button *button)
{
	if (button == NULL)
		return FALSE;

	if (config_opt_read("ConfirmDelete") && (error_msgs_report_question("QDelete", "QDeleteB") != 3))
		return FALSE;

	appdb_delete_key(button->key);

	panel_add_buttons_from_db(windat);
	panel_reflow_buttons(windat);
	panel_update_window_extent(windat);
	panel_rebuild_window(windat);

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

