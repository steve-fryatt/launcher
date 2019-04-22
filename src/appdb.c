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

/**
 * The number of new blocks to allocate when more space is required.
 */

#define APPDB_ALLOC_CHUNK 10

/**
 * The maximum length of a settings filename.
 */

#define APPDB_MAX_FILENAME_LENGTH 1024

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
 * Load the contents of a button file into the buttons database.
 *
 * \param *leaf_name	The file leafname to load.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_load_file(char *leaf_name)
{
	enum config_read_status	result;
	int			current = -1;
	char			token[1024], contents[1024], section[1024], filename[APPDB_MAX_FILENAME_LENGTH];
	FILE			*file;

	/* Find a buttons file somewhere in the usual config locations. */

	config_find_load_file(filename, APPDB_MAX_FILENAME_LENGTH, leaf_name);

	if (*filename == '\0')
		return FALSE;

	/* Open the file and work through it using the config file handling library. */

	file = fopen(filename, "r");

	if (file == NULL)
		return FALSE;

	while ((result = config_read_token_pair(file, token, contents, section)) != sf_CONFIG_READ_EOF) {

		/* A new section of the file, so create, initialise and link in a new button object. */

		if (result == sf_CONFIG_READ_NEW_SECTION) {
			current = appdb_new();

			if (current != -1)
				strncpy(appdb_list[current].name, section, APPDB_NAME_LENGTH);
		}

		/* If there is a current button object, add the current piece of data to it. */

		if (current != -1) {
			if (strcmp(token, "XPos") == 0)
				appdb_list[current].x = atoi(contents);
			else if (strcmp(token, "YPos") == 0)
				appdb_list[current].y = atoi(contents);
			else if (strcmp(token, "Sprite") == 0)
				strncpy(appdb_list[current].sprite, contents, APPDB_SPRITE_LENGTH);
			else if (strcmp(token, "RunPath") == 0)
				strncpy(appdb_list[current].command, contents, APPDB_COMMAND_LENGTH);
			else if (strcmp(token, "Boot") == 0)
				appdb_list[current].filer_boot = config_read_opt_string(contents);
		}
	}

	fclose(file);

	return TRUE;
}


/**
 * Save the contents of the buttons database into a buttons file.
 *
 * \param *leaf_name	The file leafname to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_save_file(char *leaf_name)
{
	char	filename[APPDB_MAX_FILENAME_LENGTH];
	int	current;
	FILE	*file;


	/* Find a buttons file to write somewhere in the usual config locations. */

	config_find_save_file(filename, APPDB_MAX_FILENAME_LENGTH, leaf_name);

	if (*filename == '\0')
		return FALSE;

	/* Open the file and work through it using the config file handling library. */

	file = fopen(filename, "w");

	if (file == NULL)
		return FALSE;

	fprintf(file, "# >Buttons\n#\n# Saved by Launcher.\n");

	for (current = 0; current < appdb_apps; current++) {
		fprintf(file, "\n[%s]\n", appdb_list[current].name);
		fprintf(file, "XPos: %d\n", appdb_list[current].x);
		fprintf(file, "YPos: %d\n", appdb_list[current].y);
		fprintf(file, "Sprite: %s\n", appdb_list[current].sprite);
		fprintf(file, "RunPath: %s\n", appdb_list[current].command);
		fprintf(file, "Boot: %s\n", config_return_opt_string(appdb_list[current].filer_boot));
	}

	fclose(file);

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
 * Delete an enrty from the database.
 *
 * \param key		The key of the enrty to delete.
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
 * Given a key, return details of the button associated with the application.
 * If a structure is provided, the data is copied into it; otherwise, a pointer
 * to a structure to the flex heap is returned which will remain valid only
 * the heap contents are changed.
 * 
 *
 * \param key		The key of the netry to be returned.
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
	appdb_list[appdb_apps].x = 0;
	appdb_list[appdb_apps].y = 0;
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
	to->x = from->x;
	to->y = from->y;
	to->filer_boot = from->filer_boot;

	string_copy(to->name, from->name, APPDB_NAME_LENGTH);
	string_copy(to->sprite, from->sprite, APPDB_SPRITE_LENGTH);
	string_copy(to->command, from->command, APPDB_COMMAND_LENGTH);
}

