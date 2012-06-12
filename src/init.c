/* Launcher - init.c
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
 */

/* ANSI C header files. */

#include <string.h>
#include <stdio.h>

/* Acorn C header files. */

#include "flex.h"

/* OSLib header files. */

#include "oslib/wimp.h"
#include "oslib/osspriteop.h"
#include "oslib/uri.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/menus.h"
#include "sflib/resources.h"
#include "sflib/errors.h"
#include "sflib/url.h"
#include "sflib/heap.h"

/* Application header files. */

#include "global.h"
#include "init.h"
#include "buttons.h"

/* ================================================================================================================== */

int initialise (void)
{
  static char                       task_name[255];
  char                              resources[RES_PATH_LEN], res_temp[RES_PATH_LEN];

  wimp_MESSAGE_LIST(4)              message_list;
  wimp_icon_create                  icon_bar;
  wimp_w                            window_list[10];
  wimp_menu                         *menu_list[10];

  extern global_windows             windows;
  extern global_menus               menus;

  extern wimp_t                     task_handle;


  hourglass_on ();

  strcpy(resources, "<Launcher$Dir>.Resources");
  resources_find_path(resources, RES_PATH_LEN);

  /* Load the messages file. */

  strcpy (res_temp, resources);
  msgs_initialise(strcat (res_temp, ".Messages"));

  /* Initialise the error message system. */

  error_initialise ("TaskName:Launcher", "TaskSpr:!application", NULL);

  /* Initialise with the Wimp. */

  message_list.messages[0]=message_MODE_CHANGE;
  message_list.messages[1]=message_URI_RETURN_RESULT;
  message_list.messages[2]=message_ANT_OPEN_URL;
  message_list.messages[3]=message_QUIT;
  msgs_lookup ("TaskName:Launcher", task_name, sizeof (task_name));
  task_handle = wimp_initialise (wimp_VERSION_RO3, task_name, (wimp_message_list *) &message_list, NULL);

  /* Initialise the flex heap. */

  flex_init(task_name, 0, 0);
  heap_initialise();

  /* Read the mode size and details. */

  /* read_mode_size (); */

  /* Load the configuration. */

  config_initialise(task_name, "Launcher", "<Launcher$Dir>");

  config_int_init("WindowColumns", 7);
  config_opt_init("Confirmdelete", 0);

  config_load();

  /* Load the window templates and menu structure. */

  strcpy (res_temp, resources);
  load_templates(strcat (res_temp, ".Templates"), &windows);

  window_list[0] = windows.prog_info;

  strcpy(res_temp, resources);
  menus_load_templates(strcat(res_temp, ".Menus"), window_list, menu_list, 10);

  menus.main = menu_list[0];

  /* Load the button definitions. */

  load_buttons_file ("Buttons");
  boot_buttons ();

  /* Open the launch window. */

  open_launch_window (0, wimp_BOTTOM);

  /* Tidy up and finish initialisation. */

  hourglass_off ();

  return (0);
}

/* ================================================================================================================== */

void load_templates (char *template_file, global_windows *windows)
{
  int                               context = 0, def_size, ind_size;
	char*			date = BUILD_DATE;


  byte                              *ind_data;
  const char                        *ind_data_end;
  wimp_window                       *window_def;


  ind_data = (byte *) malloc (IND_DATA_SIZE);
  ind_data_end = ind_data + IND_DATA_SIZE;

  wimp_open_template (template_file);

  /* Launch Window. */

  wimp_load_template (wimp_GET_SIZE, ind_data, ind_data_end, wimp_NO_FONTS, "Launch", context, &def_size, &ind_size);
  window_def = (wimp_window *) malloc (def_size);
  wimp_load_template (window_def, ind_data, ind_data_end, wimp_NO_FONTS, "Launch", context, &def_size, &ind_size);

  /* Remove the sample button icon, create the window and store the icon definition and position away for
   * future use by the button handling code.
   */

  window_def->icon_count = 1;
  windows->launch = wimp_create_window (window_def);
  initialise_buttons_window (windows->launch, window_def->icons[1], window_def->icons[0]);

  free (window_def);
  ind_data = (byte *) ind_size;

  /* Edit Window. */

  wimp_load_template (wimp_GET_SIZE, ind_data, ind_data_end, wimp_NO_FONTS, "Edit", context, &def_size, &ind_size);
  window_def = (wimp_window *) malloc (def_size);
  wimp_load_template (window_def, ind_data, ind_data_end, wimp_NO_FONTS, "Edit", context, &def_size, &ind_size);
  windows->edit = wimp_create_window (window_def);
  free (window_def);
  ind_data = (byte *) ind_size;

  /* Program Info Window. */

  wimp_load_template (wimp_GET_SIZE, ind_data, ind_data_end, wimp_NO_FONTS, "ProgInfo", context, &def_size, &ind_size);
  window_def = (wimp_window *) malloc (def_size);
  wimp_load_template (window_def, ind_data, ind_data_end, wimp_NO_FONTS, "ProgInfo", context, &def_size, &ind_size);
  windows->prog_info = wimp_create_window (window_def);
  icons_msgs_param_lookup(windows->prog_info, 6, "Version", BUILD_VERSION, date, NULL, NULL);
  icons_printf(windows->prog_info, 8, "\xa9 Stephen Fryatt, 2003-%s", date + 7);

  free (window_def);
  ind_data = (byte *) ind_size;

  wimp_close_template ();
}
