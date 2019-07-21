/* Copyright 2002-2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Launcher:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
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
 * \file: main.c
 */

/* ANSI C header files */

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/wimp.h"

/* SF-Lib header files */

#include "sflib/config.h"
#include "sflib/event.h"
#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/resources.h"
#include "sflib/string.h"
#include "sflib/tasks.h"
#include "sflib/templates.h"
#include "sflib/url.h"

/* Application header files */

#include "main.h"

#include "appdb.h"
#include "buttons.h"
#include "choices.h"
#include "filing.h"
#include "paneldb.h"
#include "proginfo.h"

/**
 * The size of buffer allocated to resource filename processing.
 */

#define MAIN_FILENAME_BUFFER_LEN 1024

/**
 * The size of buffer allocated to the task name.
 */

#define MAIN_TASKNAME_BUFFER_LEN 64


/* Cross file global variables */

/**
 * The Wimp task handle for the Launcher application.
 */

wimp_t			main_task_handle;

/**
 * Set TRUE for the application to quit at the next oppoprtunity.
 */

osbool			main_quit_flag = FALSE;


/* Static Function Prototypes */

static void	main_poll_loop(void);
static void	main_initialise(void);
static osbool	main_message_quit(wimp_message *message);


/**
 * Main code entry point.
 */

int main(void)
{
	main_initialise();

	main_poll_loop();

	msgs_terminate();
	buttons_terminate();
	appdb_terminate();
	paneldb_terminate();

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
			}
		}
	}
}


/**
 * Application initialisation.
 */

static void main_initialise(void)
{
	static char		task_name[MAIN_TASKNAME_BUFFER_LEN];
	char			resources[MAIN_FILENAME_BUFFER_LEN], res_temp[MAIN_FILENAME_BUFFER_LEN];


	hourglass_on();

	string_copy(resources, "<Launcher$Dir>.Resources", MAIN_FILENAME_BUFFER_LEN);
	resources_find_path(resources, MAIN_FILENAME_BUFFER_LEN);

	/* Load the messages file. */

	string_printf(res_temp, MAIN_FILENAME_BUFFER_LEN, "%s.Messages", resources);
	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName:Launcher", "TaskSpr:!application", NULL);

	/* Initialise with the Wimp. */

	msgs_lookup("TaskName:Launcher", task_name, MAIN_TASKNAME_BUFFER_LEN);
	main_task_handle = wimp_initialise(wimp_VERSION_RO3, task_name, NULL, NULL);

	if (tasks_test_for_duplicate(task_name, main_task_handle, "DupTask", "DupTaskB"))
		main_quit_flag = TRUE;

	event_add_message_handler(message_QUIT, EVENT_MESSAGE_INCOMING, main_message_quit);

	/* Initialise the flex heap. */

	flex_init(task_name, 0, 0);
	heap_initialise();

	/* Read the mode size and details. */

	/* read_mode_size(); */

	/* Load the configuration. */

	config_initialise(task_name, "Launcher", "<Launcher$Dir>");

	config_int_init("WindowColumns", 8);					/**< The number of columns to display on expanding the window.		*/
	config_int_init("GridSize", 44);					/**< The number of OS units to a grid square.				*/
	config_int_init("GridSpacing", 4);					/**< The number of OS units between grid squares.			*/
	config_opt_init("ConfirmDelete", TRUE);					/**< TRUE to confirm button deletion; FALSE to delete immediately.	*/

	config_load();

	/* Load the menu structure. */

	string_printf(res_temp, MAIN_FILENAME_BUFFER_LEN, "%s.Menus", resources);
	templates_load_menus(res_temp);

	/* Load the window templates. */

	string_printf(res_temp, MAIN_FILENAME_BUFFER_LEN, "%s.Templates", resources);
	templates_open(res_temp);

	/* Initialise the individual modules. */

	ihelp_initialise();
	url_initialise();
	proginfo_initialise();
	paneldb_initialise();
	appdb_initialise();
	buttons_initialise();
	choices_initialise();

	templates_close();

	/* Load the button definitions. */

	filing_load("Buttons");
	appdb_boot_all();
	buttons_create_from_db();

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

