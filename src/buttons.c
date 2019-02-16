/* Launcher - buttons.c
 * (c) Stephen Fryatt, 2002-2012
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
#include "main.h"


/* Button Window */

#define ICON_BUTTONS_SIDEBAR 0
#define ICON_BUTTONS_TEMPLATE 1

/* Program Info Window */

#define ICON_PROGINFO_AUTHOR  4
#define ICON_PROGINFO_VERSION 6
#define ICON_PROGINFO_WEBSITE 8

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

/* Main Menu */

#define MAIN_MENU_INFO 0
#define MAIN_MENU_HELP 1
#define MAIN_MENU_BUTTON 2
#define MAIN_MENU_NEW_BUTTON 3
#define MAIN_MENU_SAVE_BUTTONS 4
#define MAIN_MENU_CHOICES 5
#define MAIN_MENU_QUIT 6

/* Button Submenu */

#define BUTTON_MENU_EDIT 0
#define BUTTON_MENU_MOVE 1
#define BUTTON_MENU_DELETE 2

/* ================================================================================================================== */

#define BUTTON_VALIDATION_LENGTH 40

struct button
{
	unsigned	key;							/**< The database key relating to the icon.			*/

	wimp_i		icon;
	char		validation[BUTTON_VALIDATION_LENGTH];			/**< Storage for the icon's validation string.			*/

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
static wimp_w		buttons_edit_window = NULL;				/**< The handle of the button edit window.			*/
static wimp_w		buttons_info_window = NULL;				/**< The handle of the program info window.			*/

static wimp_menu	*buttons_menu = NULL;					/**< The main menu.						*/

static struct button	*buttons_menu_icon = NULL;				/**< The block for the icon over which the main menu opened.	*/
static struct button	*buttons_edit_icon = NULL;				/**< The block for the icon being edited.			*/
static os_coord		buttons_menu_coordinate;				/**< The grid coordinates where the main menu opened.		*/

static int		buttons_window_y0 = 0;					/**< The bottom of the buttons window.				*/
static int		buttons_window_y1 = 0;					/**< The top of the buttons window.				*/


static void		buttons_create_icon(struct button *button);
static void		buttons_delete_icon(struct button *button);

static void		buttons_toggle_window(void);
static void		buttons_window_open(int columns, wimp_w window_level, osbool use_level);

static void		buttons_fill_edit_window(struct button *button, os_coord *grid);
static void		buttons_redraw_edit_window(void);
static struct button	*buttons_read_edit_window(struct button *button);

static void		buttons_press(wimp_i icon);
static osbool		buttons_message_data_load(wimp_message *message);
static osbool		buttons_message_mode_change(wimp_message *message);
static void		buttons_update_window_position(void);
static void		buttons_update_grid_info(void);
static void		buttons_click_handler(wimp_pointer *pointer);
static osbool		buttons_edit_keypress_handler(wimp_key *key);
static void		buttons_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void		buttons_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void		buttons_edit_click_handler(wimp_pointer *pointer);
static osbool		buttons_proginfo_web_click(wimp_pointer *pointer);

/* ================================================================================================================== */


/**
 * Initialise the buttons window.
 */

void buttons_initialise(void)
{
	char*			date = BUILD_DATE;
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
	event_add_window_mouse_event(buttons_window, buttons_click_handler);
	event_add_window_menu(buttons_window, buttons_menu);
	event_add_window_menu_prepare(buttons_window, buttons_menu_prepare);
	event_add_window_menu_selection(buttons_window, buttons_menu_selection);

	buttons_icon_def.icon = def->icons[1];
	buttons_icon_def.w = buttons_window;

	free(def);

	/* Initialise the Edit window. */

	buttons_edit_window = templates_create_window("Edit");
	ihelp_add_window(buttons_edit_window, "Edit", NULL);
	event_add_window_mouse_event(buttons_edit_window, buttons_edit_click_handler);
	event_add_window_key_event(buttons_edit_window, buttons_edit_keypress_handler);

	/* Initialise the Program Info window. */

	buttons_info_window = templates_create_window("ProgInfo");
	ihelp_add_window(buttons_info_window, "ProgInfo", NULL);
	icons_msgs_param_lookup(buttons_info_window, ICON_PROGINFO_VERSION, "Version", BUILD_VERSION, date, NULL, NULL);
	icons_printf(buttons_info_window, ICON_PROGINFO_AUTHOR, "\xa9 Stephen Fryatt, 2003-%s", date + 7);
	event_add_window_icon_click(buttons_info_window, ICON_PROGINFO_WEBSITE, buttons_proginfo_web_click);
	templates_link_menu_dialogue("ProgInfo", buttons_info_window);

	/* Watch out for Message_ModeChange. */

	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, buttons_message_mode_change);
	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, buttons_message_data_load);

	/* Correctly size the window for the current mode. */

	buttons_update_window_position();
	buttons_update_grid_info();

	/* Open the window. */

	buttons_window_open(0, wimp_BOTTOM, TRUE);
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
 * Inform tha buttons module that the glocal system choices have changed,
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

	buttons_window_open((buttons_window_is_open) ? buttons_grid_columns : 0, wimp_TOP, FALSE);
}


/**
 * Create (or recreate) an icon for a button, based on that icon's definition
 * block.
 *
 * \param *button		The definition to create an icon for.
 */

static void buttons_create_icon(struct button *button)
{
	os_error	*error;
	int		x_pos, y_pos;
	char		*sprite;

	if (button == NULL)
		return;

	if (button->icon != -1) {
		error = xwimp_delete_icon(buttons_icon_def.w, button->icon);
		if (error != NULL)
			error_report_program(error);
		button->icon = -1;
	}

	if (!appdb_get_button_info(button->key, &x_pos, &y_pos, NULL, &sprite, NULL, NULL, NULL))
		return;

	buttons_icon_def.icon.extent.x1 = buttons_origin_x - x_pos * (buttons_grid_square + buttons_grid_spacing);
	buttons_icon_def.icon.extent.x0 = buttons_icon_def.icon.extent.x1 - buttons_slab_width;
	buttons_icon_def.icon.extent.y1 = buttons_origin_y - y_pos * (buttons_grid_square + buttons_grid_spacing);
	buttons_icon_def.icon.extent.y0 = buttons_icon_def.icon.extent.y1 - buttons_slab_height;

	snprintf(button->validation, BUTTON_VALIDATION_LENGTH, "R5,1;S%s;NButton", sprite);
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
 * Toggle the state of the button window, open and closed.
 */

static void buttons_toggle_window(void)
{
	if (buttons_window_is_open)
		buttons_window_open(0, wimp_BOTTOM, TRUE);
	else
		buttons_window_open(buttons_grid_columns, wimp_TOP, TRUE);
}


/**
 * Open or 'close' the buttons window to a given number of columns and
 * place it at a defined point in the window stack.
 *
 * \param columns		The number of columns to display, or 0 to hide.
 * \param window_level		The window to open behind.
 * \param use_level		TRUE to use the window_level; false to leave in situ.
 */

static void buttons_window_open(int columns, wimp_w window_level, osbool use_level)
{
	int			sidebar_width;
	wimp_open		window;
	wimp_icon_state		state;
	os_error		*error;

	state.w = buttons_window;
	state.i = ICON_BUTTONS_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	sidebar_width = state.icon.extent.x1 - state.icon.extent.x0;

	window.w = buttons_window;

	window.visible.x0 = 0;
	window.visible.x1 = ((columns > 0) ? (buttons_grid_spacing + columns * (buttons_grid_spacing + buttons_grid_square)) : 0) + sidebar_width;
	window.visible.y0 = buttons_window_y0;
	window.visible.y1 = buttons_window_y1;

	window.xscroll = (columns == buttons_grid_columns) ? 0 : (buttons_grid_spacing + (buttons_grid_columns - columns) * (buttons_grid_spacing + buttons_grid_square));
	window.yscroll = 0;
	window.next = window_level;

	wimp_open_window(&window);

	buttons_window_is_open = (columns != 0) ? TRUE : FALSE;
}


/**
 * Set the contents of the button edit window.
 *
 * \param *button		The button to set data for, or NULL to use defaults.
 * \param *grid			The grid coordinates to use, or NULL to use previous.
 */

static void buttons_fill_edit_window(struct button *button, os_coord *grid)
{
	static int	x_pos = 0, y_pos = 0;
	char		*name, *sprite, *command;
	osbool		local_copy, filer_boot;


	/* Initialise deafults if button data can't be found. */

	if (button == NULL || !appdb_get_button_info(button->key, &x_pos, &y_pos,
			&name, &sprite, &command, &local_copy, &filer_boot)) {
		if (grid != NULL) {
			x_pos = grid->x;
			y_pos = grid->y;
		}
		name = "";
		sprite = "";
		command = "";
		local_copy = FALSE;
		filer_boot = TRUE;
	}

	/* Fill the window's icons. */

	icons_strncpy(buttons_edit_window, ICON_EDIT_NAME, name);
	icons_strncpy(buttons_edit_window, ICON_EDIT_SPRITE, sprite);
	icons_strncpy(buttons_edit_window, ICON_EDIT_LOCATION, command);

	icons_printf(buttons_edit_window, ICON_EDIT_XPOS, "%d", x_pos);
	icons_printf(buttons_edit_window, ICON_EDIT_YPOS, "%d", y_pos);

	icons_set_selected(buttons_edit_window, ICON_EDIT_KEEP_LOCAL, local_copy);
	icons_set_selected(buttons_edit_window, ICON_EDIT_BOOT, filer_boot);
}


/**
 * Redraw the contents of the button edit window.
 */

static void buttons_redraw_edit_window(void)
{
	wimp_set_icon_state(buttons_edit_window, ICON_EDIT_NAME, 0, 0);
	wimp_set_icon_state(buttons_edit_window, ICON_EDIT_SPRITE, 0, 0);
	wimp_set_icon_state(buttons_edit_window, ICON_EDIT_LOCATION, 0, 0);
	wimp_set_icon_state(buttons_edit_window, ICON_EDIT_XPOS, 0, 0);
	wimp_set_icon_state(buttons_edit_window, ICON_EDIT_YPOS, 0, 0);

	icons_replace_caret_in_window(buttons_edit_window);
}


/**
 * Read the contents of the button edit window, validate it and store the
 * details into a button definition.
 *
 * \param *button		The button to update; NULL to create a new entry.
 * \return			Pointer to the button containing the data.
 */

static struct button *buttons_read_edit_window(struct button *button)
{
	char		name[APPDB_NAME_LENGTH], sprite[APPDB_SPRITE_LENGTH], command[APPDB_COMMAND_LENGTH];
	int		x_pos, y_pos;
	osbool		local_copy, filer_boot;


	x_pos = atoi(icons_get_indirected_text_addr(buttons_edit_window, ICON_EDIT_XPOS));
	y_pos = atoi(icons_get_indirected_text_addr(buttons_edit_window, ICON_EDIT_YPOS));

	if (x_pos < 0 || y_pos < 0 || x_pos >= buttons_grid_columns || y_pos >= buttons_grid_rows) {
		error_msgs_report_info("CoordRange");
		return NULL;
	}

	local_copy = icons_get_selected(buttons_edit_window, ICON_EDIT_KEEP_LOCAL);
	filer_boot = icons_get_selected(buttons_edit_window, ICON_EDIT_BOOT);

	icons_copy_text(buttons_edit_window, ICON_EDIT_NAME, name, APPDB_NAME_LENGTH);
	icons_copy_text(buttons_edit_window, ICON_EDIT_SPRITE, sprite, APPDB_SPRITE_LENGTH);
	icons_copy_text(buttons_edit_window, ICON_EDIT_LOCATION, command, APPDB_COMMAND_LENGTH);

	if (*name == '\0' || *sprite == '\0' || *command == '\0') {
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
				button->next = buttons_list;
				buttons_list = button;
			} else {
				heap_free(button);
				button = NULL;
			}
		}
	}

	if (button != NULL)
		appdb_set_button_info(button->key, x_pos, y_pos, name, sprite, command, local_copy, filer_boot);

	return button;
}



/**
 * Press a button in the window.
 *
 * \param icon			The handle of the icon being pressed.
 */

static void buttons_press(wimp_i icon)
{
	char		*command, *buffer;
	int		length;
	os_error	*error;
	struct button	*button = buttons_list;


	while (button!= NULL && button->icon != icon)
		button = button->next;

	if (button == NULL)
		return;

	if (!appdb_get_button_info(button->key, NULL, NULL, NULL, NULL, &command, NULL, NULL))
		return;

	if (command == NULL)
		return;

	length = strlen(command) + 19;

	buffer = heap_alloc(length);

	if (buffer == NULL)
		return;

	/* Refetch the command data, as the heap_alloc() could have moved the
	 * flex heap and hence invalidated the command pointer.
	 */

	if (appdb_get_button_info(button->key, NULL, NULL, NULL, NULL, &command, NULL, NULL) &&
			command != NULL) {
		snprintf(buffer, length, "%%StartDesktopTask %s", command);
		error = xos_cli(buffer);
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
	}

	heap_free(buffer);

	return;
}


/**
 * Handle incoming Message_DataSave to the button edit window, by using the
 * information to populate the relevant fields.
 *
 * \param *message		The message data block from the Wimp.
 */

static osbool buttons_message_data_load(wimp_message *message)
{
	char *leafname, spritename[APPDB_SPRITE_LENGTH];

	wimp_full_message_data_xfer *data_load = (wimp_full_message_data_xfer *) message;

	if (data_load->w != buttons_edit_window)
		return FALSE;

	if (!windows_get_open(buttons_edit_window))
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

	icons_strncpy(buttons_edit_window, ICON_EDIT_NAME, leafname);
	icons_strncpy(buttons_edit_window, ICON_EDIT_SPRITE, spritename);
	icons_strncpy(buttons_edit_window, ICON_EDIT_LOCATION, data_load->file_name);

	buttons_redraw_edit_window();

	return TRUE;
}


/**
 * Handle incoming Message_ModeChange.
 *
 * \param *message		The message data block from the Wimp.
 */

static osbool buttons_message_mode_change(wimp_message *message)
{
	buttons_update_window_position();

	return TRUE;
}


/**
 * Update the vertical position of the buttons window to take into account
 * a change of screen mode.
 */

static void buttons_update_window_position(void)
{
	int			mode_height, window_height;
	wimp_window_info	info;
	wimp_icon_state		state;
	os_error		*error;


	info.w = buttons_window;
	error = xwimp_get_window_info_header_only(&info);
	if (error != NULL)
		return;

	state.w = buttons_window;
	state.i = ICON_BUTTONS_SIDEBAR;
	error = xwimp_get_icon_state(&state);
	if (error != NULL)
		return;

	mode_height = general_mode_height();
	window_height = mode_height - sf_ICONBAR_HEIGHT;

	if ((info.extent.y1 - info.extent.y0) != window_height) {
		info.extent.y1 = 0;
		info.extent.y0 = info.extent.y1 - window_height;

		error = xwimp_set_extent(buttons_window, &(info.extent));
		if (error != NULL)
			return;

		error = xwimp_resize_icon(buttons_window, ICON_BUTTONS_SIDEBAR,
			state.icon.extent.x0, info.extent.y0, state.icon.extent.x1, info.extent.y1);
	}

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
	state.i = ICON_BUTTONS_SIDEBAR;
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

	error = xwimp_resize_icon(buttons_window, ICON_BUTTONS_SIDEBAR,
		info.extent.x1 - sidebar_width, state.icon.extent.y0, info.extent.x1, state.icon.extent.y1);
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
		if (pointer->i == ICON_BUTTONS_SIDEBAR) {
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

	if (pointer->i == wimp_ICON_WINDOW || pointer->i == ICON_BUTTONS_SIDEBAR) {
		buttons_menu_icon = NULL;
	} else {
		buttons_menu_icon = buttons_list;

		while (buttons_menu_icon!= NULL && buttons_menu_icon->icon != pointer->i)
			buttons_menu_icon = buttons_menu_icon->next;
	}

	menus_shade_entry(buttons_menu, MAIN_MENU_BUTTON, (buttons_menu_icon == NULL) ? TRUE : FALSE);
	menus_shade_entry(buttons_menu, MAIN_MENU_NEW_BUTTON, (pointer->i == wimp_ICON_WINDOW) ? FALSE : TRUE);

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
	case MAIN_MENU_HELP:
		error = xos_cli("%Filer_Run <Launcher$Dir>.!Help");
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
		break;

	case MAIN_MENU_BUTTON:
		if (buttons_menu_icon != NULL) {
			switch (selection->items[1]) {
			case BUTTON_MENU_EDIT:
				buttons_edit_icon = buttons_menu_icon;
				buttons_fill_edit_window(buttons_edit_icon, NULL);
				windows_open_centred_at_pointer(buttons_edit_window, &pointer);
				icons_put_caret_at_end(buttons_edit_window, ICON_EDIT_NAME);
				break;

			case BUTTON_MENU_DELETE:
				if (!config_opt_read("ConfirmDelete") || (error_msgs_report_question("QDelete", "QDeleteB") == 3)) {
					buttons_delete_icon(buttons_menu_icon);
					buttons_menu_icon = NULL;
				}
				break;
			}
		}
		break;

	case MAIN_MENU_NEW_BUTTON:
		buttons_edit_icon = NULL;
		buttons_fill_edit_window(buttons_edit_icon, &buttons_menu_coordinate);
		windows_open_centred_at_pointer(buttons_edit_window, &pointer);
		icons_put_caret_at_end(buttons_edit_window, ICON_EDIT_NAME);
		break;

	case MAIN_MENU_SAVE_BUTTONS:
		appdb_save_file("Buttons");
		break;

	case MAIN_MENU_CHOICES:
		choices_open_window(&pointer);
		break;

	case MAIN_MENU_QUIT:
		main_quit_flag = TRUE;
		break;
	}
}


/**
 * Process mouse clicks in the Edit dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void buttons_edit_click_handler(wimp_pointer *pointer)
{
	struct button		*button;

	if (pointer == NULL)
		return;

	switch (pointer->i) {
	case ICON_EDIT_OK:
		button = buttons_read_edit_window(buttons_edit_icon);
		if (button != NULL) {
			buttons_create_icon(button);
			if (pointer->buttons == wimp_CLICK_SELECT) {
				wimp_close_window(buttons_edit_window);
				buttons_edit_icon = NULL;
			}
		}
		break;

	case ICON_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			wimp_close_window(buttons_edit_window);
			buttons_edit_icon = NULL;
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			buttons_fill_edit_window(buttons_edit_icon, NULL);
			buttons_redraw_edit_window();
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

static osbool buttons_edit_keypress_handler(wimp_key *key)
{
	struct button		*button;


	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		button = buttons_read_edit_window(buttons_edit_icon);
		if (button != NULL) {
			buttons_create_icon(button);
			wimp_close_window(buttons_edit_window);
			buttons_edit_icon = NULL;
		}
		break;

	case wimp_KEY_ESCAPE:
		wimp_close_window(buttons_edit_window);
		buttons_edit_icon = NULL;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Handle clicks on the Website action button in the program info window.
 *
 * \param *pointer	The Wimp Event message block for the click.
 * \return		TRUE if we handle the click; else FALSE.
 */

static osbool buttons_proginfo_web_click(wimp_pointer *pointer)
{
	char		temp_buf[256];

	msgs_lookup("SupportURL:http://www.stevefryatt.org.uk/software/utils/", temp_buf, sizeof(temp_buf));
	url_launch(temp_buf);

	if (pointer->buttons == wimp_CLICK_SELECT)
		wimp_create_menu((wimp_menu *) -1, 0, 0);

	return TRUE;
}

