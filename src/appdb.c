/* Copyright 2012-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: appdb.c
 */

/* ANSI C header files. */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files. */

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

#include "appdb.h"

#include "filing.h"
#include "paneldb.h"

/**
 * The number of new blocks to allocate when more space is required.
 */

#define APPDB_ALLOC_CHUNK 10

/**
 * The length of the *Filer_Boot command string.
 */

#define APPDB_FILER_BOOT_LENGTH 15

/* Global Variables. */

/**
 * The flex array of application data.
 */

static struct appdb_entry		*appdb_list = NULL;

/**
 * The number of applications stored in the database.
 */

static int				appdb_apps = 0;

/**
 * The number of applications for which space is allocated.
 */

static int				appdb_allocation = 0;

/**
 * The next unique database primary key.
 */

static unsigned				appdb_key = 0;

/* Static Function Prototypes. */

static int	appdb_find(unsigned key);
static int	appdb_new();
static void	appdb_delete(int index);

/**
 * Initialise the application database.
 */

void appdb_initialise(void)
{
	if (flex_alloc((flex_ptr) &appdb_list,
			(appdb_allocation + APPDB_ALLOC_CHUNK) * sizeof(struct appdb_entry)) == 1)
		appdb_allocation += APPDB_ALLOC_CHUNK;
}


/**
 * Terminate the application database.
 */

void appdb_terminate(void)
{
	if (appdb_list != NULL)
		flex_free((flex_ptr) &appdb_list);
}


/**
 * Reset the application database.
 */

void appdb_reset(void)
{
	appdb_apps = 0;
	appdb_key = 0;
}


/**
 * Load the contents of an old format button file into the buttons
 * database.
 *
 * \param *in		The filing operation to load from.
 * \param panel		The index of the panel to add the entries to.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_load_old_file(struct filing_block *in, int panel)
{
	int current = -1;

	debug_printf("Loading an old file...");

	if (panel == -1) {
		 filing_set_status(in, FILING_STATUS_MEMORY);
		 return FALSE;
	}

	while (filing_get_next_section(in)) {
		current = appdb_new();

		if (current == -1) {
			 filing_set_status(in, FILING_STATUS_MEMORY);
			 return FALSE;
		}

		filing_get_section_name(in, appdb_list[current].name, APPDB_NAME_LENGTH);
		appdb_list[current].panel = panel;

		debug_printf("Loading section '%s'", appdb_list[current].name);

		do {
			if (current != -1) {
				if (filing_test_token(in, "XPos"))
					appdb_list[current].x = filing_get_int_value(in);
				else if (filing_test_token(in, "YPos"))
					appdb_list[current].y = filing_get_int_value(in);
				else if (filing_test_token(in, "Sprite"))
					filing_get_text_value(in, appdb_list[current].sprite, APPDB_SPRITE_LENGTH);
				else if (filing_test_token(in, "RunPath"))
					filing_get_text_value(in, appdb_list[current].command, APPDB_COMMAND_LENGTH);
				else if (filing_test_token(in, "Boot"))
					appdb_list[current].filer_boot = filing_get_opt_value(in);
				else
					filing_set_status(in, FILING_STATUS_UNEXPECTED);
			}
		} while (filing_get_next_token(in));
	}

	return TRUE;
}


/**
 * Load the contents of a new format button file into the buttons
 * database.
 *
 * \param *in		The filing operation to load from.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_load_new_file(struct filing_block *in)
{
	int current = -1, panel = -1;

	debug_printf("Loading a new file...");

	do {
		if (filing_test_token(in, "@")) {
			current = appdb_new();

			if (current == -1) {
				 filing_set_status(in, FILING_STATUS_MEMORY);
				 return FALSE;
			}

			filing_get_text_value(in, appdb_list[current].name, APPDB_NAME_LENGTH);
			debug_printf("Loading section '%s'", appdb_list[current].name);
		} else if ((current != -1) && filing_test_token(in, "Panel")) {
			panel = paneldb_lookup_name(filing_get_text_value(in, NULL, 0));
			if (panel == -1) {
				 filing_set_status(in, FILING_STATUS_MEMORY);
				 return FALSE;
			}
			appdb_list[current].panel = panel;
		} else if ((current != -1) && filing_test_token(in, "XPos"))
			appdb_list[current].x = filing_get_int_value(in);
		else if ((current != -1) && filing_test_token(in, "YPos"))
			appdb_list[current].y = filing_get_int_value(in);
		else if ((current != -1) && filing_test_token(in, "Sprite"))
			filing_get_text_value(in, appdb_list[current].sprite, APPDB_SPRITE_LENGTH);
		else if ((current != -1) && filing_test_token(in, "RunPath"))
			filing_get_text_value(in, appdb_list[current].command, APPDB_COMMAND_LENGTH);
		else if ((current != -1) && filing_test_token(in, "Boot"))
			appdb_list[current].filer_boot = filing_get_opt_value(in);
		else
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
	} while (filing_get_next_token(in));

	return TRUE;
}


/**
 * Once panels and buttons are loaded, scan the buttons replacing the
 * panel indexes with the associated panel keys.
 *
 * \return		TRUE if successful; FALSE if errors occurred.
 */

osbool appdb_complete_file_load(void)
{
	int		i;
	unsigned	key;

	for (i = 0; i < appdb_apps; i++) {
		key = paneldb_lookup_key(appdb_list[i].panel);
		if (key == PANELDB_NULL_KEY) {
			debug_printf("Unexpected panel for button '%s'", appdb_list[i].name);
			return FALSE;
		}
		appdb_list[i].panel = key;
	}

	return TRUE;
}

/**
 * Save the contents of the buttons database into a buttons file.
 *
 * \param *file		The file handle to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_save_file(FILE *file)
{
	int	current;

	if (file == NULL)
		return FALSE;

	fprintf(file, "\n[Buttons]");

	for (current = 0; current < appdb_apps; current++) {
		fprintf(file, "\n@: %s\n", appdb_list[current].name);
		fprintf(file, "Panel: %s\n", paneldb_get_name(appdb_list[current].panel));
		fprintf(file, "XPos: %d\n", appdb_list[current].x);
		fprintf(file, "YPos: %d\n", appdb_list[current].y);
		fprintf(file, "Sprite: %s\n", appdb_list[current].sprite);
		fprintf(file, "RunPath: %s\n", appdb_list[current].command);
		fprintf(file, "Boot: %s\n", config_return_opt_string(appdb_list[current].filer_boot));
	}

	return TRUE;
}


/**
 * Filer_Boot all of the applications stored in the application database, if
 * selected.
 */

void appdb_boot_all(void)
{
	int		current;
	char		command[APPDB_FILER_BOOT_LENGTH + APPDB_COMMAND_LENGTH];
	os_error	*error;

	for (current = 0; current < appdb_apps; current++) {
		if (appdb_list[current].filer_boot) {
			string_printf(command, APPDB_FILER_BOOT_LENGTH + APPDB_COMMAND_LENGTH, "Filer_Boot %s", appdb_list[current].command);
			error = xos_cli(command);

			if ((error != NULL) &&
					(error_msgs_param_report_error("BootFail", appdb_list[current].name, error->errmess, NULL, NULL) == wimp_ERROR_BOX_SELECTED_CANCEL))
				break;
		}
	}
}


/**
 * Create a new, empty entry in the database and return its key.
 *
 * \return		The new key, or APPDB_NULL_KEY.
 */

unsigned appdb_create_key(void)
{
	unsigned	key = APPDB_NULL_KEY;
	int		index = appdb_new();

	if (index != -1)
		key = appdb_list[index].key;

	return key;
}


/**
 * Delete an entry from the database.
 *
 * \param key		The key of the entry to delete.
 */

void appdb_delete_key(unsigned key)
{
	int		index;

	if (key == APPDB_NULL_KEY)
		return;

	index = appdb_find(key);

	if (index != -1)
		appdb_delete(index);
}


/**
 * Given a database key, return the next key from the database.
 *
 * \param key		The current key, or APPDB_NULL_KEY to start sequence.
 * \return		The next key, or APPDB_NULL_KEY.
 */

unsigned appdb_get_next_key(unsigned key)
{
	int index;

	if (key == APPDB_NULL_KEY && appdb_apps > 0)
		return appdb_list[0].key;

	index = appdb_find(key);

	return (index != -1 && index < (appdb_apps - 1)) ? appdb_list[index + 1].key : APPDB_NULL_KEY;
}


/**
 * Given a database key, return the associated button panel ID.
 *
 * \param key		The database key to query.
 * \return		The associated panel ID, or APPDB_NULL_PANEL.
 */

unsigned appdb_get_panel(unsigned key)
{
	int index;

	index = appdb_find(key);

	if (index == APPDB_NULL_PANEL)
		return APPDB_NULL_PANEL;

	return appdb_list[index].panel;
}


/**
 * Given a key, return details of the button associated with the application.
 * If a structure is provided, the data is copied into it; otherwise, a pointer
 * to a structure in the flex heap is returned which will remain valid only
 * the heap contents are changed.
 * 
 *
 * \param key		The key of the entry to be returned.
 * \param *data		Pointer to structure to return the data, or NULL.
 * \return		Pointer to the returned data, or NULL on failure.
 */

struct appdb_entry *appdb_get_button_info(unsigned key, struct appdb_entry *data)
{
	int index;

	/* Locate the requested key, and return NULL if it isn't found. */

	index = appdb_find(key);

	if (index == -1)
		return NULL;

	/* If no buffer is supplied, just return a pointer into the flex heap. */

	if (data == NULL)
		return appdb_list + index;

	/* Copy the data into the supplied buffer. */

	appdb_copy(data, appdb_list + index);

	return data;
}


/**
 * Given a data structure, set the details of a database entry by copying the
 * contents of the structure into the database.
 *
 * \param *data		Pointer to the structure containing the data.
 * \return		TRUE if an entry was updated; else FALSE.
 */

osbool appdb_set_button_info(struct appdb_entry *data)
{
	int index;

	if (data == NULL)
		return FALSE;

	index = appdb_find(data->key);

	if (index == -1)
		return FALSE;

	appdb_copy(appdb_list + index, data);

	return TRUE;
}

/**
 * Find the index of an application based on its key.
 *
 * \param key		The key to locate.
 * \return		The current index, or -1 if not found.
 */

static int appdb_find(unsigned key)
{
	int index;

	/* We know that keys are allocated in ascending order, possibly
	 * with gaps in the sequence.  Therefore we can hit the list
	 * at the corresponding index (or the end) and count back until
	 * we pass the key we're looking for.
	 */

	index = (key >= appdb_apps) ? appdb_apps - 1 : key;

	while (index >= 0 && appdb_list[index].key > key)
		index--;

	if (index != -1 && appdb_list[index].key != key)
		index = -1;

	return index;
}


/**
 * Claim a block for a new application, fill in the unique key and set
 * default values for the data.
 *
 * \return		The new block number, or -1 on failure.
 */

static int appdb_new()
{
	if (appdb_apps >= appdb_allocation && flex_extend((flex_ptr) &appdb_list,
			(appdb_allocation + APPDB_ALLOC_CHUNK) * sizeof(struct appdb_entry)) == 1)
		appdb_allocation += APPDB_ALLOC_CHUNK;

	if (appdb_apps >= appdb_allocation)
		return -1;

	appdb_list[appdb_apps].key = appdb_key++;
	appdb_list[appdb_apps].panel = APPDB_NULL_PANEL;
	appdb_list[appdb_apps].x = 0;
	appdb_list[appdb_apps].y = 0;
	appdb_list[appdb_apps].local_copy = FALSE;
	appdb_list[appdb_apps].filer_boot = TRUE;

	*(appdb_list[appdb_apps].name) = '\0';
	*(appdb_list[appdb_apps].sprite) = '\0';
	*(appdb_list[appdb_apps].command) = '\0';

	return appdb_apps++;
}


/**
 * Delete an application block, given its index.
 *
 * \param index		The index of the block to be deleted.
 */

static void appdb_delete(int index)
{
	if (index < 0 || index >= appdb_apps)
		return;

	flex_midextend((flex_ptr) &appdb_list, (index + 1) * sizeof(struct appdb_entry),
			-sizeof(struct appdb_entry));
}


/**
 * Copy the contents of an application block into a second block.
 *
 * \param *to		The block to copy the data to.
 * \param *from		The block to copy the data from.
 */

void appdb_copy(struct appdb_entry *to, struct appdb_entry *from)
{
	to->panel = from->panel;
	to->x = from->x;
	to->y = from->y;
	to->local_copy = from->local_copy;
	to->filer_boot = from->filer_boot;

	string_copy(to->name, from->name, APPDB_NAME_LENGTH);
	string_copy(to->sprite, from->sprite, APPDB_SPRITE_LENGTH);
	string_copy(to->command, from->command, APPDB_COMMAND_LENGTH);
}

