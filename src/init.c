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
