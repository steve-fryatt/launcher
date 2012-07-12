/* Launcher - buttons.c
 * (c) Stephen Fryatt, 2002
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
#define MAIN_MENU_BUTTON 1
#define MAIN_MENU_NEW_BUTTON 2
#define MAIN_MENU_SAVE_BUTTONS 3
#define MAIN_MENU_CHOICES 4
#define MAIN_MENU_QUIT 5

/* Button Submenu */

#define BUTTON_MENU_EDIT 0
#define BUTTON_MENU_MOVE 1
#define BUTTON_MENU_DELETE 2

/* ================================================================================================================== */

#define BUTTON_VALIDATION_LENGTH 40

struct button
{
	unsigned	key;							/**< The database key relating to the icon.	*/

	wimp_i		icon;
	char		validation[BUTTON_VALIDATION_LENGTH];			/**< Storage for the icon's validation string.	*/

	struct button	*next;							/**< Pointer to the next button definition.	*/
};


static struct button	*buttons_list = NULL;

//static button           *button_list = NULL, *edit_button = NULL;

static wimp_icon_create	buttons_icon_def;					/**< The definition for a button icon.		*/

static int              button_x_base,
                        button_y_base,
                        button_width,
                        button_height,

                        window_offset,
                        window_x_extent;

static bool             window_open;


static wimp_w		buttons_window = NULL;					/**< The handle of the buttons window.		*/
static wimp_w		buttons_edit_window = NULL;				/**< The handle of the button edit window.	*/
static wimp_w		buttons_info_window = NULL;				/**< The handle of the program info window.	*/

static wimp_menu	*buttons_menu = NULL;					/**< The main menu.				*/

static wimp_i		buttons_menu_icon = 0;					/**< The icon over which the main menu opened.	*/

static int		buttons_window_y0 = 0;					/**< The bottom of the buttons window.		*/
static int		buttons_window_y1 = 0;					/**< The top of the buttons window.		*/


static void	buttons_create_icon(struct button *button);

static void	buttons_press(wimp_i icon);
static osbool	buttons_message_mode_change(wimp_message *message);
static void	buttons_update_window_position(void);
static void	buttons_click_handler(wimp_pointer *pointer);
static void	buttons_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void	buttons_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void	buttons_edit_click_handler(wimp_pointer *pointer);
static osbool	buttons_proginfo_web_click(wimp_pointer *pointer);

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

	if (!appdb_get_button_info(button->key, &x_pos, &y_pos, &sprite))
		return;

	buttons_icon_def.icon.extent.x0 = button_x_base - (x_pos * (button_width / 2 + BUTTON_GUTTER));
	buttons_icon_def.icon.extent.x1 = buttons_icon_def.icon.extent.x0 + button_width;
	buttons_icon_def.icon.extent.y0 = button_y_base - (y_pos * (button_height / 2 + BUTTON_GUTTER));
	buttons_icon_def.icon.extent.y1 = buttons_icon_def.icon.extent.y0 + button_height;

	snprintf(button->validation, BUTTON_VALIDATION_LENGTH, "R5,1;S%s", sprite);
	buttons_icon_def.icon.data.indirected_text_and_sprite.validation = button->validation;

	button->icon = wimp_create_icon(&buttons_icon_def);
}




/* ==================================================================================================================
 * Launch Window handling code.
 */

void toggle_launch_window (void)
{
  if (window_open)
  {
    open_launch_window(0, wimp_BOTTOM);
  }
  else
  {
    open_launch_window(config_int_read("WindowColumns"), wimp_TOP);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void open_launch_window (int columns, wimp_w window_level)
{
  int open_offset;
  wimp_open window;


  open_offset = (button_x_base - 2*BUTTON_GUTTER - ((columns - 1) * (button_width/2 + BUTTON_GUTTER)));

  window.w = buttons_window;

  window.visible.x0 = 0;
  window.visible.x1 = columns ? window_x_extent - open_offset : window_x_extent - window_offset;
  window.visible.y0 = buttons_window_y0;
  window.visible.y1 = buttons_window_y1;

  window.xscroll = columns ? open_offset : window_offset;
  window.yscroll = 0;
  window.next = window_level;

  wimp_open_window (&window);

  window_open = (columns != 0);
}

/* ==================================================================================================================
 * Edit Button Window handling code.
 */
#if 0
int fill_edit_button_window (wimp_i icon)
{
  /* Set the icons in the edit button window.
   *
   * If icon >= 0, the data for that button is used, otherwise the data for the current window
   * is used (to allow for resets, etc.).
   */

  button                *list = button_list;

  if (icon != -1)
  {
    while (list != NULL && list->icon != icon)
    {
      list = list->next;
    }
  }
  else
  {
    list = edit_button;
  }

  if (list != NULL)
  {
    icons_strncpy(buttons_edit_window, 2, list->name);
    icons_strncpy(buttons_edit_window, 8, list->sprite);
    icons_strncpy(buttons_edit_window, 11, list->command);

    icons_printf(buttons_edit_window, 5, "%d", list->x);
    icons_printf(buttons_edit_window, 7, "%d", list->y);

    icons_set_selected(buttons_edit_window, 10, list->local_copy);
    icons_set_selected(buttons_edit_window, 13, list->filer_boot);
  }

  edit_button = list;

  return (0);
}
#endif
/* ------------------------------------------------------------------------------------------------------------------ */

int open_edit_button_window (wimp_pointer *pointer)
{
  windows_open_centred_at_pointer(buttons_edit_window, pointer);
  icons_put_caret_at_end(buttons_edit_window, 2);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int redraw_edit_button_window (void)
{
  wimp_set_icon_state(buttons_edit_window, 2, 0, 0);
  wimp_set_icon_state(buttons_edit_window, 8, 0, 0);
  wimp_set_icon_state(buttons_edit_window, 11, 0, 0);
  wimp_set_icon_state(buttons_edit_window, 5, 0, 0);
  wimp_set_icon_state(buttons_edit_window, 7, 0, 0);

  icons_replace_caret_in_window(buttons_edit_window);

  return 1;
}

/* ------------------------------------------------------------------------------------------------------------------ */
#if 0
int read_edit_button_window (button *button_def)
{
  /* Read the contents of the Edit button window into a button block.
   *
   * If button_def != NULL, that definition is used (to allow for creating new buttons); otherwise
   * the button for whom the window was opened is updated.
   */



  if (button_def == NULL)
  {
    button_def = edit_button;
  }

  if (button_def != NULL)
  {
    icons_copy_text(buttons_edit_window, 2, button_def->name);
    icons_copy_text(buttons_edit_window, 8, button_def->sprite);
    icons_copy_text(buttons_edit_window, 11, button_def->command);

    button_def->x = atoi(icons_get_indirected_text_addr(buttons_edit_window, 5));
    button_def->y = atoi(icons_get_indirected_text_addr(buttons_edit_window, 7));

    button_def->local_copy = icons_get_selected(buttons_edit_window, 10);
    button_def->filer_boot = icons_get_selected(buttons_edit_window, 13);
  }

  create_button_icon (button_def);

  return 1;
}
#endif
/* ------------------------------------------------------------------------------------------------------------------ */

int close_edit_button_window (void)
{


  wimp_close_window (buttons_edit_window);

  //edit_button = NULL;

  return 1;
}

/* ================================================================================================================== */





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

	command = appdb_get_command(button->key);

	if (command == NULL)
		return;

	length = strlen(command) + 19;

	buffer = heap_alloc(length);

	if (buffer == NULL)
		return;

	/* Refetch the command data, as the heap_alloc() could have moved the
	 * flex heap and hence invalidated the command pointer.
	 */

	command = appdb_get_command(button->key);

	if (command != NULL) {
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
			toggle_launch_window();
		} else {
			buttons_press(pointer->i);

			if (pointer->buttons == wimp_CLICK_SELECT)
				toggle_launch_window();
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

	menus_shade_entry(buttons_menu, MAIN_MENU_BUTTON, (pointer->i > ICON_BUTTONS_SIDEBAR) ? FALSE : TRUE);
	menus_shade_entry(buttons_menu, MAIN_MENU_NEW_BUTTON, (pointer->i == wimp_ICON_WINDOW) ? FALSE : TRUE);

        buttons_menu_icon = pointer->i;
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

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]) {
	/*case MAIN_MENU_HELP:
		os_cli("%Filer_Run <Locate$Dir>.!Help");
		break; */

	case MAIN_MENU_BUTTON:
		switch (selection->items[1]) {
		case BUTTON_MENU_EDIT:
			//fill_edit_button_window(buttons_menu_icon);
			open_edit_button_window(&pointer);
			break;
		}
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
	if (pointer == NULL)
		return;

	switch (pointer->i) {
	case ICON_EDIT_OK:
		//read_edit_button_window(NULL);
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_edit_button_window();
		break;

	case ICON_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_edit_button_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			//fill_edit_button_window ((wimp_i) -1);
			redraw_edit_button_window ();
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

