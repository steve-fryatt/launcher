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
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files. */

#include "buttons.h"

#include "appdb.h"
#include "ihelp.h"
#include "main.h"
#include "templates.h"


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

static int              button_x_base,
                        button_y_base,
                        button_width,
                        button_height,

                        window_offset,
                        window_x_extent;

static osbool		buttons_window_is_open = FALSE;				/**< TRUE if the window is currently 'open'; else FALSE.	*/


static wimp_w		buttons_window = NULL;					/**< The handle of the buttons window.				*/
static wimp_w		buttons_edit_window = NULL;				/**< The handle of the button edit window.			*/
static wimp_w		buttons_info_window = NULL;				/**< The handle of the program info window.			*/

static wimp_menu	*buttons_menu = NULL;					/**< The main menu.						*/

static struct button	*buttons_menu_icon = NULL;				/**< The block for the icon over which the main menu opened.	*/
static struct button	*buttons_edit_icon = NULL;				/**< The block for the icon being edited.			*/

static int		buttons_window_y0 = 0;					/**< The bottom of the buttons window.				*/
static int		buttons_window_y1 = 0;					/**< The top of the buttons window.				*/


static void		buttons_create_icon(struct button *button);

static void		buttons_toggle_window(void);
static void		buttons_window_open(int columns, wimp_w window_level);

static void		buttons_fill_edit_window(struct button *button);
static void		buttons_redraw_edit_window(void);
static struct button	*buttons_read_edit_window(struct button *button);

static void		buttons_press(wimp_i icon);
static osbool		buttons_message_mode_change(wimp_message *message);
static void		buttons_update_window_position(void);
static void		buttons_click_handler(wimp_pointer *pointer);
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

	buttons_menu = templates_get_menu(TEMPLATES_MENU_MAIN);

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

	button_x_base = def->icons[ICON_BUTTONS_TEMPLATE].extent.x0;
	button_y_base = def->icons[ICON_BUTTONS_TEMPLATE].extent.y0;
	button_width = def->icons[ICON_BUTTONS_TEMPLATE].extent.x1 - def->icons[ICON_BUTTONS_TEMPLATE].extent.x0;
	button_height = def->icons[ICON_BUTTONS_TEMPLATE].extent.y1 - def->icons[ICON_BUTTONS_TEMPLATE].extent.y0;

	window_offset = def->icons[ICON_BUTTONS_SIDEBAR].extent.x0;
	window_x_extent = def->icons[ICON_BUTTONS_SIDEBAR].extent.x1;

	free(def);

	/* Initialise the Edit window. */

	buttons_edit_window = templates_create_window("Edit");
	ihelp_add_window(buttons_edit_window, "Edit", NULL);
	event_add_window_mouse_event(buttons_edit_window, buttons_edit_click_handler);

	/* Initialise the Program Info window. */

	buttons_info_window = templates_create_window("ProgInfo");
	ihelp_add_window(buttons_info_window, "ProgInfo", NULL);
	icons_msgs_param_lookup(buttons_info_window, ICON_PROGINFO_VERSION, "Version", BUILD_VERSION, date, NULL, NULL);
	icons_printf(buttons_info_window, ICON_PROGINFO_AUTHOR, "\xa9 Stephen Fryatt, 2003-%s", date + 7);
	event_add_window_icon_click(buttons_info_window, ICON_PROGINFO_WEBSITE, buttons_proginfo_web_click);
	templates_link_menu_dialogue("ProgInfo", buttons_info_window);

	/* Watch out for Message_ModeChange. */

	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, buttons_message_mode_change);

	/* Correctly size the window for the current mode. */

	buttons_update_window_position();

	/* Open the window. */

	buttons_window_open(0, wimp_BOTTOM);
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

	buttons_icon_def.icon.extent.x0 = button_x_base - (x_pos * (button_width / 2 + BUTTON_GUTTER));
	buttons_icon_def.icon.extent.x1 = buttons_icon_def.icon.extent.x0 + button_width;
	buttons_icon_def.icon.extent.y0 = button_y_base - (y_pos * (button_height / 2 + BUTTON_GUTTER));
	buttons_icon_def.icon.extent.y1 = buttons_icon_def.icon.extent.y0 + button_height;

	snprintf(button->validation, BUTTON_VALIDATION_LENGTH, "R5,1;S%s", sprite);
	buttons_icon_def.icon.data.indirected_text_and_sprite.validation = button->validation;

	button->icon = wimp_create_icon(&buttons_icon_def);
}


/**
 * Toggle the state of the button window, open and closed.
 */

static void buttons_toggle_window(void)
{
	if (buttons_window_is_open)
		buttons_window_open(0, wimp_BOTTOM);
	else
		buttons_window_open(config_int_read("WindowColumns"), wimp_TOP);
}


/**
 * Open or 'close' the buttons window to a given number of columns and
 * place it at a defined point in the window stack.
 *
 * \param columns		The number of columns to display, or 0 to hide.
 * \param window_level		The window to open behind.
 */

static void buttons_window_open(int columns, wimp_w window_level)
{
	int		open_offset;
 	wimp_open	window;


	open_offset = (button_x_base - 2*BUTTON_GUTTER - ((columns - 1) * (button_width/2 + BUTTON_GUTTER)));

	window.w = buttons_window;

	window.visible.x0 = 0;
	window.visible.x1 = columns ? window_x_extent - open_offset : window_x_extent - window_offset;
	window.visible.y0 = buttons_window_y0;
	window.visible.y1 = buttons_window_y1;

	window.xscroll = columns ? open_offset : window_offset;
	window.yscroll = 0;
	window.next = window_level;

	wimp_open_window(&window);

	buttons_window_is_open = (columns != 0) ? TRUE : FALSE;
}


/**
 * Set the contents of the button edit window.
 *
 * \param *button		The button to set data for, or NULL to use defaults.
 */

static void buttons_fill_edit_window(struct button *button)
{
	int		x_pos, y_pos;
	char		*name, *sprite, *command;
	osbool		local_copy, filer_boot;


	/* Initialise deafults if button data can't be found. */

	if (button == NULL || !appdb_get_button_info(button->key, &x_pos, &y_pos,
			&name, &sprite, &command, &local_copy, &filer_boot)) {
		x_pos = 0;
		y_pos = 0;
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

	local_copy = icons_get_selected(buttons_edit_window, ICON_EDIT_KEEP_LOCAL);
	filer_boot = icons_get_selected(buttons_edit_window, ICON_EDIT_BOOT);

	icons_copy_text(buttons_edit_window, ICON_EDIT_NAME, name);
	icons_copy_text(buttons_edit_window, ICON_EDIT_SPRITE, sprite);
	icons_copy_text(buttons_edit_window, ICON_EDIT_LOCATION, command);

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
 * Handle incoming Message_ModeChange.
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
	buttons_window_y0 = sf_ICONBAR_HEIGHT;
	buttons_window_y1 = general_mode_height();
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
		switch (selection->items[1]) {
		case BUTTON_MENU_EDIT:
			if (buttons_menu_icon != NULL) {
				buttons_edit_icon = buttons_menu_icon;
				buttons_fill_edit_window(buttons_edit_icon);
				windows_open_centred_at_pointer(buttons_edit_window, &pointer);
				icons_put_caret_at_end(buttons_edit_window, ICON_EDIT_NAME);
			}
			break;
		}
		break;

	case MAIN_MENU_NEW_BUTTON:
		buttons_edit_icon = NULL;
		buttons_fill_edit_window(buttons_edit_icon);
		windows_open_centred_at_pointer(buttons_edit_window, &pointer);
		icons_put_caret_at_end(buttons_edit_window, ICON_EDIT_NAME);
		break;

	case MAIN_MENU_SAVE_BUTTONS:
		appdb_save_file("Buttons");
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
			buttons_fill_edit_window(buttons_edit_icon);
			buttons_redraw_edit_window();
		}
		break;
	}
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

