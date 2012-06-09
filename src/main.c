/* Launcher - main.c
 * (c) Stephen Fryatt, 2002
 *
 * Desktop application launcher.
 */


/* ANSI C header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/os.h"
#include "oslib/wimp.h"
#include "oslib/messagetrans.h"
#include "oslib/uri.h"

/* Application header files */

#include "global.h"
#include "main.h"
#include "buttons.h"

/* SF-Lib header files */

#include "sflib/menus.h"
#include "sflib/url.h"
#include "sflib/msgs.h"
#include "sflib/debug.h"

/* ================================================================================================================== */

/* Declare the global variables that are used. */

global_windows  windows;
global_menus    menus;

wimp_i          button_menu_icon;

/* Cross file global variables. */

wimp_t                     task_handle;
int                        quit_flag = FALSE;

/* ================================================================================================================== */

int main (void)
{
  extern wimp_t                     task_handle;


  initialise ();

  poll_loop ();

  msgs_close_file ();
  wimp_close_down (task_handle);

  return (0);
}

/* ================================================================================================================== */

int poll_loop (void)
{
  wimp_block        blk;
  int               pollword;

  extern int        quit_flag;


  while (!quit_flag)
  {
    switch (wimp_poll (wimp_MASK_NULL, &blk, &pollword))
    {
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

      case wimp_USER_MESSAGE:
      case wimp_USER_MESSAGE_RECORDED:
        user_message_handler (&(blk.message));
        break;

      case wimp_USER_MESSAGE_ACKNOWLEDGE:
        bounced_message_handler (&(blk.message));
        break;
    }
  }

  return 0;
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
        menus.menu_up = create_standard_menu (menus.main, pointer);
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
      launch_url (temp_buf);
      wimp_create_menu ((wimp_menu *) -1, 0, 0);
    }
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void menu_selection_handler (wimp_selection *selection)
{
  wimp_pointer pointer;

  extern int   quit_flag;


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
      quit_flag = TRUE;
    }
  }

  if (pointer.buttons == wimp_CLICK_ADJUST)
  {
    wimp_create_menu (menus.menu_up, 0, 0);
  }
}

/* ------------------------------------------------------------------------------------------------------------------ */

void user_message_handler (wimp_message *message)

/* User message handler.
 *
 * All messages are handled internally, except for Message_Quit which must be passed back up the chain to
 * the calling function so that it can act on it.
 */

{
  extern int quit_flag;


  switch (message->action)
  {
    case message_QUIT:
      quit_flag=TRUE;
      break;

    case message_URI_RETURN_RESULT:
      url_bounce (message);
      break;

    case message_MODE_CHANGE:
      /* read_mode_size (); */
      break;
  }
}
/* ------------------------------------------------------------------------------------------------------------------ */

void bounced_message_handler (wimp_message *message)

/* User message handler.
 *
 * All messages are handled internally, except for Message_Quit which must be passed back up the chain to
 * the calling function so that it can act on it.
 */

{
  switch (message->action)
  {
    case message_ANT_OPEN_URL:
      url_bounce (message);
      break;
  }
}

