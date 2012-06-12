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
#include "sflib/icons.h"
#include "sflib/windows.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files. */

#include "global.h"
#include "buttons.h"

/* ================================================================================================================== */

static button           *button_list = NULL, *edit_button = NULL;

static wimp_icon_create icon_definition;

static int              button_x_base,
                        button_y_base,
                        button_width,
                        button_height,

                        window_offset,
                        window_x_extent;

static bool             window_open;

/* ================================================================================================================== */

void initialise_buttons_window (wimp_w window, wimp_icon sample_icon, wimp_icon edge_icon)
{
  icon_definition.icon = sample_icon;
  icon_definition.w = window;

  button_x_base = sample_icon.extent.x0;
  button_y_base = sample_icon.extent.y0;
  button_width = sample_icon.extent.x1 - sample_icon.extent.x0;
  button_height = sample_icon.extent.y1 - sample_icon.extent.y0;

  window_offset = edge_icon.extent.x0;
  window_x_extent = edge_icon.extent.x1;
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

  extern global_windows windows;


  open_offset = (button_x_base - 2*BUTTON_GUTTER - ((columns - 1) * (button_width/2 + BUTTON_GUTTER)));

  window.w = windows.launch;

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

  extern global_windows windows;

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
    icons_strncpy(windows.edit, 2, list->name);
    icons_strncpy(windows.edit, 8, list->sprite);
    icons_strncpy(windows.edit, 11, list->command);

    icons_printf(windows.edit, 5, "%d", list->x);
    icons_printf(windows.edit, 7, "%d", list->y);

    icons_set_selected(windows.edit, 10, list->local_copy);
    icons_set_selected(windows.edit, 13, list->filer_boot);
  }

  edit_button = list;

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int open_edit_button_window (wimp_pointer *pointer)
{
  extern global_windows windows;


  windows_open_centred_at_pointer(windows.edit, pointer);
  icons_put_caret_at_end(windows.edit, 2);

  return (0);
}

/* ------------------------------------------------------------------------------------------------------------------ */

int redraw_edit_button_window (void)
{
  extern global_windows windows;


  wimp_set_icon_state(windows.edit, 2, 0, 0);
  wimp_set_icon_state(windows.edit, 8, 0, 0);
  wimp_set_icon_state(windows.edit, 11, 0, 0);
  wimp_set_icon_state(windows.edit, 5, 0, 0);
  wimp_set_icon_state(windows.edit, 7, 0, 0);

  icons_replace_caret_in_window(windows.edit);

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

  extern global_windows windows;


  if (button_def == NULL)
  {
    button_def = edit_button;
  }

  if (button_def != NULL)
  {
    icons_copy_text(windows.edit, 2, button_def->name);
    icons_copy_text(windows.edit, 8, button_def->sprite);
    icons_copy_text(windows.edit, 11, button_def->command);

    button_def->x = atoi(icons_get_indirected_text_addr(windows.edit, 5));
    button_def->y = atoi(icons_get_indirected_text_addr(windows.edit, 7));

    button_def->local_copy = icons_get_selected(windows.edit, 10);
    button_def->filer_boot = icons_get_selected(windows.edit, 13);
  }

  create_button_icon (button_def);

  return 1;
}

/* ------------------------------------------------------------------------------------------------------------------ */

int close_edit_button_window (void)
{
  extern global_windows windows;


  wimp_close_window (windows.edit);

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
