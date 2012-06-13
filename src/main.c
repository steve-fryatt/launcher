/* Launcher - main.c
 * (c) Stephen Fryatt, 2002
 *
 * Desktop application launcher.
 */


/* ANSI C header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/os.h"
#include "oslib/wimp.h"
#include "oslib/messagetrans.h"
#include "oslib/uri.h"

/* SF-Lib header files */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/event.h"
#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/resources.h"
#include "sflib/url.h"

/* Application header files */

#include "global.h"

#include "main.h"

#include "buttons.h"
#include "init.h"

/* ================================================================================================================== */

static void	main_poll_loop(void);
static void	main_initialise(void);
static osbool	main_message_quit(wimp_message *message);
static osbool	main_message_mode_change(wimp_message *message);


/* Declare the global variables that are used. */

global_windows  windows;
global_menus    menus;

wimp_i          button_menu_icon;

/*
 * Cross file global variables
 */

wimp_t			main_task_handle;
osbool			main_quit_flag = FALSE;

/* ================================================================================================================== */



/**
 * Main code entry point.
 */

int main(void)
{
	main_initialise();

	main_poll_loop();

	msgs_terminate();
	wimp_close_down(main_task_handle);

	return 0;
}


/**
 * Wimp Poll loop.
 */

static void main_poll_loop(void)
{
	wimp_event_no		reason;
	wimp_block		blk;


	while (!main_quit_flag) {
		reason = wimp_poll(wimp_MASK_NULL, &blk, NULL);

		if (!event_process_event(reason, &blk, 0)) {

			switch (reason) {
			case wimp_OPEN_WINDOW_REQUEST:
				wimp_open_window (&(blk.open));
				break;

			case wimp_CLOSE_WINDOW_REQUEST:
				wimp_close_window (blk.close.w);
				break;

			case wimp_MOUSE_CLICK:
				mouse_click_handler (&(blk.pointer));
				break;

			case wimp_MENU_SELECTION:
				menu_selection_handler (&(blk.selection));
				break;
			}
		}
	}
}


/**
 * Application initialisation.
 */

static void main_initialise(void)
{
	static char		task_name[255];
	char			resources[RES_PATH_LEN], res_temp[RES_PATH_LEN];

	wimp_MESSAGE_LIST(4)	message_list;
	wimp_icon_create	icon_bar;
	wimp_w			window_list[10];
	wimp_menu		*menu_list[10];

	extern global_windows	windows;
	extern global_menus	menus;


	hourglass_on();

	strcpy(resources, "<Launcher$Dir>.Resources");
	resources_find_path(resources, RES_PATH_LEN);

	/* Load the messages file. */

	snprintf(res_temp, sizeof(res_temp), "%s.Messages", resources);
	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName:Launcher", "TaskSpr:!application", NULL);

	/* Initialise with the Wimp. */

	message_list.messages[0]=message_MODE_CHANGE;
	message_list.messages[1]=message_URI_RETURN_RESULT;
	message_list.messages[2]=message_ANT_OPEN_URL;
	message_list.messages[3]=message_QUIT;
	msgs_lookup("TaskName:Launcher", task_name, sizeof(task_name));
	main_task_handle = wimp_initialise(wimp_VERSION_RO3, task_name, (wimp_message_list *) &message_list, NULL);

	event_add_message_handler(message_QUIT, EVENT_MESSAGE_INCOMING, main_message_quit);
	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, main_message_mode_change);

	/* Initialise the flex heap. */

	flex_init(task_name, 0, 0);
	heap_initialise();

	/* Read the mode size and details. */

	/* read_mode_size(); */

	/* Load the configuration. */

	config_initialise(task_name, "Launcher", "<Launcher$Dir>");

	config_int_init("WindowColumns", 7);
	config_opt_init("Confirmdelete", 0);

	config_load();

	/* Load the menu structure. */


	/* Load the window templates. */

	snprintf(res_temp, sizeof(res_temp), "%s.Templates", resources);
	load_templates(res_temp, &windows);

	window_list[0] = windows.prog_info;

	snprintf(res_temp, sizeof(res_temp), "%s.Menus", resources);
	menus_load_templates(res_temp, window_list, menu_list, 10);

	menus.main = menu_list[0];

	/* Initialise the individual modules. */

	url_initialise();

	/* Load the button definitions. */

	load_buttons_file("Buttons");
	boot_buttons();

	/* Open the launch window. */

	open_launch_window(0, wimp_BOTTOM);

	/* Tidy up and finish initialisation. */

	hourglass_off();
}


/**
 * Handle incoming Message_Quit.
 */

static osbool main_message_quit(wimp_message *message)
{
	main_quit_flag = TRUE;

	return TRUE;
}


/**
 * Handle incoming Message_ModeChange.
 */

static osbool main_message_mode_change(wimp_message *message)
{
	// read_mode_size();

	return TRUE;
}




/* ------------------------------------------------------------------------------------------------------------------ */

void mouse_click_handler (wimp_pointer *pointer)
{
  /* pointer->i points to the icon, pointer->w the window handle, etc. */

  wimp_window_state   window;


  #ifdef DEBUG
  {
    char debug[256];

    sprintf (debug, "Mouse click %d", pointer->buttons);
    debug_reporter_text0 (debug);
  }
  #endif



  if (pointer-> w == windows.launch)
  {

    /* Launch Window */

    switch ((int) pointer->buttons)
    {
      case wimp_CLICK_SELECT:
      case wimp_CLICK_ADJUST:
        if (pointer->i == 0)
        {
          toggle_launch_window ();
        }
        else
        {
          press_button (pointer->i);

          if (pointer->buttons == wimp_CLICK_SELECT)
          {
            toggle_launch_window ();
          }
        }
        break;


      case wimp_CLICK_MENU:
        if (pointer->i > 0)
        {
          menus.main->entries[1].icon_flags &= ~wimp_ICON_SHADED;
        }
        else
        {
          menus.main->entries[1].icon_flags |= wimp_ICON_SHADED;
        }

        if (pointer->i == wimp_ICON_WINDOW)
        {
          menus.main->entries[2].icon_flags &= ~wimp_ICON_SHADED;
        }
        else
        {
          menus.main->entries[2].icon_flags |= wimp_ICON_SHADED;
        }

        button_menu_icon = pointer->i;
        menus.menu_up = menus_create_standard_menu (menus.main, pointer);
        break;
    }
  }
  else if (pointer->w == windows.edit)
  {

    /* Edit Window */

    switch ((int) pointer->i)
    {
      case 0: /* OK */
        read_edit_button_window (NULL);
        if (pointer->buttons == wimp_CLICK_SELECT)
        {
          close_edit_button_window ();
        }
        break;

      case 1: /* Cancel */
        if (pointer->buttons == wimp_CLICK_SELECT)
        {
          close_edit_button_window ();
        }
        else if (pointer->buttons == wimp_CLICK_ADJUST)
        {
          fill_edit_button_window ((wimp_i) -1);
          redraw_edit_button_window ();
        }
        break;
    }
  }

  else if (pointer->w == windows.prog_info)
  {

    /* Program info window */

    if (pointer->i == 8 && pointer->buttons == wimp_CLICK_SELECT) /* Website button */
    {
      char temp_buf[256];

      msgs_lookup ("SupportURL:http://www.stevefryatt.org.uk/software/utils/", temp_buf, sizeof (temp_buf));
      url_launch(temp_buf);
      wimp_create_menu ((wimp_menu *) -1, 0, 0);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void menu_selection_handler (wimp_selection *selection)
{
  wimp_pointer pointer;


  wimp_get_pointer_info (&pointer);

  if (menus.menu_up == menus.main)
  {
    if (selection->items[0] == 1) /* Button submenu */
    {
      if (selection->items[1] == 0) /* Edit button... */
      {
        fill_edit_button_window (button_menu_icon);
        open_edit_button_window (&pointer);
      }
    }
    else if (selection->items[0] == 3) /* Save buttons */
    {
      save_buttons_file ("Buttons");
    }
    else if (selection->items[0] == 5) /* Quit */
    {
      main_quit_flag = TRUE;
    }
  }

  if (pointer.buttons == wimp_CLICK_ADJUST)
  {
    wimp_create_menu (menus.menu_up, 0, 0);
  }
}

