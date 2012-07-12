/* Launcher - appdb.c
 * (c) Stephen Fryatt, 2012
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


#define APPDB_ALLOC_CHUNK 10

/**
 * Application data structure -- Implementation.
 */

struct application
{
	unsigned	key;							/**< Primary key to index database entries.			*/
	char		name[64];						/**< Button name.						*/
	int		x, y;							/**< X and Y positions of the button in the window.		*/
	char		sprite[20];						/**< The sprite name.						*/
	osbool		local_copy;						/**< Do we keep a local copy of the sprite?			*/
	char		command[1024];						/**< The command to be executed.				*/
	osbool		filer_boot;						/**< Should the item be Filer_Booted on startup?		*/
};


static struct application		*appdb_list = NULL;			/**< Array of application data.					*/

static int				appdb_apps = 0;				/**< The number of applications stored in the database.		*/
static int				appdb_allocation = 0;			/**< The number of applications for which space is allocated.	*/

static unsigned				appdb_key = 0;				/**< Track new unique primary keys.				*/

static int	appdb_find(unsigned key);
static int	appdb_new();
static void	appdb_delete(int index);

/**
 * Initialise the application database.
 */

void appdb_initialise(void)
{
	if (flex_alloc((flex_ptr) &appdb_list,
			(appdb_allocation + APPDB_ALLOC_CHUNK) * sizeof(struct application)) == 1)
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
 * \param *leaf_name		The file leafname to load.
 * \return			TRUE on success; else FALSE.
 */

osbool appdb_load_file(char *leaf_name)
{
	int	result, current = -1;
	char	token[1024], contents[1024], section[1024], filename[1024];
	FILE	*file;

	/* Find a buttons file somewhere in the usual config locations. */

	config_find_load_file(filename, sizeof(filename), leaf_name);

	if (*filename == '\0')
		return FALSE;

	/* Open the file and work through it using the config file handling library. */

	file = fopen(filename, "r");

	if (file == NULL)
		return FALSE;

	while ((result = config_read_token_pair(file, token, contents, section)) != sf_READ_CONFIG_EOF) {

		/* A new section of the file, so create, initialise and link in a new button object. */

		if (result == sf_READ_CONFIG_NEW_SECTION) {
			current = appdb_new();

			if (current != -1) {
				strcpy(appdb_list[current].name, section);
				*(appdb_list[current].sprite) = '\0';
				*(appdb_list[current].command) = '\0';

				appdb_list[current].x = 0;
				appdb_list[current].y = 0;
				appdb_list[current].local_copy = 0;
				appdb_list[current].filer_boot = 1;

//				appdb_list[current].icon = -1;
//				*(appdb_list[current].validation) = '\0';
			}
		}

		/* If there is a current button object, add the current piece of data to it. */

		if (current != -1) {
			if (strcmp(token, "XPos") == 0)
				appdb_list[current].x = atoi(contents);
			else if (strcmp(token, "YPos") == 0)
				appdb_list[current].y = atoi(contents);
			else if (strcmp(token, "Sprite") == 0)
				strcpy (appdb_list[current].sprite, contents);
			else if (strcmp(token, "RunPath") == 0)
				strcpy (appdb_list[current].command, contents);
			else if (strcmp(token, "Boot") == 0)
				appdb_list[current].filer_boot = config_read_opt_string(contents);
		}
	}

	fclose(file);

	/* Work through the list, creating the icons in the window. */

//	current = button_list;

//	while (current != NULL) {
//		create_button_icon (current);
//		current = current->next;
//	}

	return TRUE;
}


/**
 * Save the contents of the buttons database into a buttons file.
 *
 * \param *leaf_name		The file leafname to save to.
 * \return			TRUE on success; else FALSE.
 */

osbool appdb_save_file(char *leaf_name)
{
	char	filename[1024];
	int	current;
	FILE	*file;


	/* Find a buttons file to write somewhere in the usual config locations. */

	config_find_save_file(filename, sizeof(filename), leaf_name);

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
	char		command[1024];
	os_error	*error;

	for (current = 0; current < appdb_apps; current++) {
		if (appdb_list[current].filer_boot) {
			snprintf(command, sizeof(command), "Filer_Boot %s", appdb_list[current].command);
			error = xos_cli(command);
		}
	}
}


/**
 * Find the index of an application based on its key.
 *
 * \param key			The key to locate.
 * \return			The current index, or -1 if not found.
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
 * Claim a block for a new application, and fill in the unique key value.
 *
 * \return			The new block number, or -1 on failure.
 */

static int appdb_new()
{
	if (appdb_apps >= appdb_allocation && flex_extend((flex_ptr) &appdb_list,
			(appdb_allocation + APPDB_ALLOC_CHUNK) * sizeof(struct application)) == 1)
		appdb_allocation += APPDB_ALLOC_CHUNK;

	if (appdb_apps >= appdb_allocation)
		return -1;

	appdb_list[appdb_apps].key = appdb_key++;

	return appdb_apps++;
}


/**
 * Delete an application block, given its index.
 *
 * \param index			The index of the block to be deleted.
 */

static void appdb_delete(int index)
{
	if (index < 0 || index >= appdb_apps)
		return;

	flex_midextend((flex_ptr) &appdb_list, (index + 1) * sizeof(struct application),
			-sizeof(struct application));
}

