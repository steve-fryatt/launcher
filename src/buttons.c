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
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files. */

#include "buttons.h"

#include "ihelp.h"
#include "main.h"
#include "templates.h"


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

static void	buttons_click_handler(wimp_pointer *pointer);
static void	buttons_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void	buttons_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void	buttons_edit_click_handler(wimp_pointer *pointer);
static osbool	buttons_proginfo_web_click(wimp_pointer *pointer);


static button           *button_list = NULL, *edit_button = NULL;

static wimp_icon_create icon_definition;

static int              button_x_base,
                        button_y_base,
                        button_width,
                        button_height,

                        window_offset,
                        window_x_extent;

static bool             window_open;


static wimp_w		buttons_window = NULL;			/**< The handle of the buttons window.		*/
static wimp_w		buttons_edit_window = NULL;		/**< The handle of the button edit window.	*/
static wimp_w		buttons_info_window = NULL;		/**< The handle of the program info window.	*/

static wimp_menu	*buttons_menu = NULL;			/**< The main menu.				*/

static wimp_i		buttons_menu_icon = 0;			/**< The icon over which the main menu opened.	*/


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


	icon_definition.icon = def->icons[1];
	icon_definition.w = buttons_window;

	button_x_base = def->icons[1].extent.x0;
	button_y_base = def->icons[1].extent.y0;
	button_width = def->icons[1].extent.x1 - def->icons[1].extent.x0;
	button_height = def->icons[1].extent.y1 - def->icons[1].extent.y0;

	window_offset = def->icons[0].extent.x0;
	window_x_extent = def->icons[0].extent.x1;

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



}


/* ================================================================================================================== */

int load_buttons_file (char *leaf_name)
{
  int    result;
  char   token[1024], contents[1024], section[1024], filename[1024];
  button *current = NULL, **last;
  FILE   *file;


  last = &button_list;

  /* Find a buttons file somewhere in the usual config locations. */

  config_find_load_file (filename, sizeof(filename), leaf_name);

  if (*filename != '\0')
  {
    /* Open the file and work through it using the config file handling library. */

    file = fopen (filename, "r");

    if (file != NULL)
    {
      while ((result = config_read_token_pair(file, token, contents, section)) != sf_READ_CONFIG_EOF)
      {
        if (result == sf_READ_CONFIG_NEW_SECTION)
        {
          /* A new section of the file, so create, initialise and link in a new button object. */

          current = (button *) heap_alloc (sizeof (button));

          if (current != NULL)
          {
            strcpy (current->name, section);
            *(current->sprite) = '\0';
            *(current->command) = '\0';

            current->x = 0;
            current->y = 0;
            current->local_copy = 0;
            current->filer_boot = 1;

            current->icon = -1;
            *(current->validation) = '\0';

            current->next = NULL;
            *last = current;
            last = &(current->next);
          }
        }

        if (current != NULL)
        {
          /* If there is a current button object, add the current piece of data to it. */

          if (strcmp (token, "XPos") == 0)
          {
            current->x = atoi (contents);
          }
          else if (strcmp (token, "YPos") == 0)
          {
            current->y = atoi (contents);
          }
          else if (strcmp (token, "Sprite") == 0)
          {
            strcpy (current->sprite, contents);
          }
          else if (strcmp (token, "RunPath") == 0)
          {
            strcpy (current->command, contents);
          }
          else if (strcmp (token, "Boot") == 0)
          {
            current->filer_boot = config_read_opt_string(contents);
          }
        }
      }

      fclose (file);
    }
  }

  /* Work through the list, creating the icons in the window. */

  current = button_list;

  while (current != NULL)
  {
    create_button_icon (current);

    current = current->next;
  }

  return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */

int save_buttons_file (char *leaf_name)
{
  char   filename[1024];
  button *current = button_list;
  FILE   *file;


  /* Find a buttons file to write somewhere in the usual config locations. */

  config_find_save_file (filename, sizeof(filename), leaf_name);

  if (*filename != '\0')
  {
    /* Open the file and work through it using the config file handling library. */

    file = fopen (filename, "w");

    if (file != NULL)
    {
      fprintf (file, "# >Buttons\n#\n# Saved by Launcher.\n");

      while (current != NULL)
      {
        fprintf (file, "\n[%s]\n", current->name);
        fprintf (file, "XPos: %d\n", current->x);
        fprintf (file, "YPos: %d\n", current->y);
        fprintf (file, "Sprite: %s\n", current->sprite);
        fprintf (file, "RunPath: %s\n", current->command);
        fprintf (file, "Boot: %s\n", config_return_opt_string(current->filer_boot));

        current = current->next;
      }

      fclose (file);
    }
  }

  return 0;
}

/* ================================================================================================================== */

void boot_buttons (void)
{
  button *current = button_list;
  char     command[1024];
  os_error *error;


  while (current != NULL)
  {
    if (current->filer_boot)
    {
      sprintf (command, "Filer_Boot %s", current->command);
      error = xos_cli (command);
    }

    current = current->next;
  }
}

/* ================================================================================================================== */

int create_button_icon (button *button_def)
{
  os_error                *error;
  int                     exists = 0;


  if (button_def != NULL)
  {
    if (button_def->icon != -1)
    {
      /* Delete the icon if it already exists. */

      error = xwimp_delete_icon (icon_definition.w, button_def->icon);
      if (error != NULL)
      {
        error_report_program(error);
      }

      button_def->icon = -1;

      exists = 1;
    }

    /* Create a new icon in the correct place. */

    icon_definition.icon.extent.x0 = button_x_base - (button_def->x * (button_width / 2 + BUTTON_GUTTER));
    icon_definition.icon.extent.x1 = icon_definition.icon.extent.x0 + button_width;
    icon_definition.icon.extent.y0 = button_y_base - (button_def->y * (button_height / 2 + BUTTON_GUTTER));
    icon_definition.icon.extent.y1 = icon_definition.icon.extent.y0 + button_height;

    sprintf (button_def->validation, "R5,1;S%s", button_def->sprite);
    icon_definition.icon.data.indirected_text_and_sprite.validation = button_def->validation;

    button_def->icon = wimp_create_icon (&icon_definition);

    if (exists)
    {
      wimp_set_icon_state (icon_definition.w, button_def->icon, 0, 0);
    }
  }

  return (0);
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
  window.visible.y0 = WINDOW_YPOS;
  window.visible.y1 = WINDOW_YPOS + WINDOW_HEIGHT;

  window.xscroll = columns ? open_offset : window_offset;
  window.yscroll = 0;
  window.next = window_level;

  wimp_open_window (&window);

  window_open = (columns != 0);
}

/* ==================================================================================================================
 * Edit Button Window handling code.
 */

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

/* ------------------------------------------------------------------------------------------------------------------ */

int close_edit_button_window (void)
{


  wimp_close_window (buttons_edit_window);

  edit_button = NULL;

  return 1;
}

/* ================================================================================================================== */

int press_button (wimp_i icon)
{
  char     *command;
  button   *list = button_list;
  os_error *error;

  while (list != NULL && list->icon != icon)
  {
    list = list->next;
  }

  if (list != NULL)
  {
    command = (char *) malloc (strlen (list->command) + 19);
    sprintf (command, "%%StartDesktopTask %s", list->command);
    error = xos_cli (command);
    if (error != NULL)
    {
      error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
    }

    free (command);
  }

  return (0);
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
		if (pointer->i == 0) {
			toggle_launch_window();
		} else {
			press_button(pointer->i);

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

	menus_shade_entry(buttons_menu, MAIN_MENU_BUTTON, (pointer->i > 0) ? FALSE : TRUE);
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
			fill_edit_button_window(buttons_menu_icon);
			open_edit_button_window(&pointer);
			break;
		}
		break;

	case MAIN_MENU_SAVE_BUTTONS:
		save_buttons_file("Buttons");
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
		read_edit_button_window(NULL);
		if (pointer->buttons == wimp_CLICK_SELECT)
			close_edit_button_window();
		break;

	case ICON_EDIT_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			close_edit_button_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			fill_edit_button_window ((wimp_i) -1);
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

