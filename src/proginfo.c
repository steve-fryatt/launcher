/* Copyright 2002-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: proginfo.c
 */

/* ANSI C header files. */

/* OSLib header files. */

/* SF-Lib header files. */

#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/url.h"

/* Application header files. */

#include "proginfo.h"

/* Program Info Window Icons */

#define PROGINFO_ICON_AUTHOR  4
#define PROGINFO_ICON_VERSION 6
#define PROGINFO_ICON_WEBSITE 8

/**
 * The maximum length of a complete URL.
 */

#define PROGINFO_URL_LENGTH 256

/**
 * The handle of the program info window.
 */

static wimp_w		proginfo_window = NULL;

/* Static Function Prototypes. */

static osbool		proginfo_web_click(wimp_pointer *pointer);


/**
 * Initialise the program information window.
 */

void proginfo_initialise(void)
{
	char* date = BUILD_DATE;

	/* Initialise the Program Info window. */

	proginfo_window = templates_create_window("ProgInfo");
	ihelp_add_window(proginfo_window, "ProgInfo", NULL);
	icons_msgs_param_lookup(proginfo_window, PROGINFO_ICON_VERSION, "Version", BUILD_VERSION, date, NULL, NULL);
	icons_printf(proginfo_window, PROGINFO_ICON_AUTHOR, "\xa9 Stephen Fryatt, 2003-%s", date + 7);
	event_add_window_icon_click(proginfo_window, PROGINFO_ICON_WEBSITE, proginfo_web_click);
	templates_link_menu_dialogue("ProgInfo", proginfo_window);
}


/**
 * Handle clicks on the Website action button in the program info window.
 *
 * \param *pointer	The Wimp Event message block for the click.
 * \return		TRUE if we handle the click; else FALSE.
 */

static osbool proginfo_web_click(wimp_pointer *pointer)
{
	char		temp_buf[PROGINFO_URL_LENGTH];

	msgs_lookup("SupportURL:http://www.stevefryatt.org.uk/software/utils/", temp_buf, PROGINFO_URL_LENGTH);
	url_launch(temp_buf);

	if (pointer->buttons == wimp_CLICK_SELECT)
		wimp_create_menu((wimp_menu *) -1, 0, 0);

	return TRUE;
}

