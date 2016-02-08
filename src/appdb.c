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
	char		name[APPDB_NAME_LENGTH];				/**< Button name.						*/
	int		x, y;							/**< X and Y positions of the button in the window.		*/
	char		sprite[APPDB_SPRITE_LENGTH];				/**< The sprite name.						*/
	osbool		local_copy;						/**< Do we keep a local copy of the sprite?			*/
	char		command[APPDB_COMMAND_LENGTH];				/**< The command to be executed.				*/
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
	enum config_read_status	result;
	int			current = -1;
	char			token[1024], contents[1024], section[1024], filename[1024];
	FILE			*file;

	/* Find a buttons file somewhere in the usual config locations. */

	config_find_load_file(filename, sizeof(filename), leaf_name);

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
 * Create a new, empty entry in the database and return its key.
 *
 * \return			The new key, or APPDB_NULL_KEY.
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
 * \param key			The key of the enrty to delete.
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
 * \param key			The current key, or APPDB_NULL_KEY to start sequence.
 * \return			The next key, or APPDB_NULL_KEY.
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
 * Any parameters passed as NULL will not be returned. String pointers will only
 * remain valid until the memory heap is disturbed.
 *
 * \param key			The key of the netry to be returned.
 * \param *x_pos		Place to return the X coordinate of the button.
 * \param *y_pos		Place to return the Y coordinate of the button.
 * \param **name		Place to return a pointer to the button name.
 * \param **sprite		Place to return a pointer to the sprite name
 *				used in the button.
 * \param **command		Place to return a pointer to the command associated
 *				with a button.
 * \param *local_copy		Place to return the local copy flag.
 * \param *filer_boot		Place to return the filer boot flag.
 * \return			TRUE if an entry was found; else FALSE.
 */

osbool appdb_get_button_info(unsigned key, int *x_pos, int *y_pos, char **name, char **sprite,
		char **command, osbool *local_copy, osbool *filer_boot)
{
	int index;

	index = appdb_find(key);

	if (index == -1)
		return FALSE;

	if (x_pos != NULL)
		*x_pos = appdb_list[index].x;

	if (y_pos != NULL)
		*y_pos = appdb_list[index].y;

	if (name != NULL)
		*name = appdb_list[index].name;

	if (sprite != NULL)
		*sprite = appdb_list[index].sprite;

	if (command != NULL)
		*command = appdb_list[index].command;

	if (local_copy != NULL)
		*local_copy = appdb_list[index].local_copy;

	if (filer_boot != NULL)
		*filer_boot = appdb_list[index].filer_boot;

	return TRUE;
}


/**
 * Given a key, set details of the button associated with the application.
 *
 * \param key			The key of the entry to be updated.
 * \param *x_pos		The new X coordinate of the button.
 * \param *y_pos		The new Y coordinate of the button.
 * \param **name		Pointer to the new button name.
 * \param **sprite		Pointer to the new sprite name to be used in the button.
 * \param **command		Pointer to the command associated with the button.
 * \param *local_copy		The new local copy flag.
 * \param *filer_boot		The new filer boot flag.
 * \return			TRUE if an entry was updated; else FALSE.
 */

osbool appdb_set_button_info(unsigned key, int x_pos, int y_pos, char *name, char *sprite,
		char *command, osbool local_copy, osbool filer_boot)
{
	int index;

	index = appdb_find(key);

	if (index == -1)
		return FALSE;

	appdb_list[index].x = x_pos;
	appdb_list[index].y = y_pos;
	appdb_list[index].local_copy = local_copy;
	appdb_list[index].filer_boot = filer_boot;

	if (name != NULL)
		strncpy(appdb_list[index].name, name, APPDB_NAME_LENGTH);
	if (sprite != NULL)
		strncpy(appdb_list[index].sprite, sprite, APPDB_SPRITE_LENGTH);
	if (command != NULL)
		strncpy(appdb_list[index].command, command, APPDB_COMMAND_LENGTH);

	return TRUE;
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
 * Claim a block for a new application, fill in the unique key and set
 * default values for the data.
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
 * \param index			The index of the block to be deleted.
 */

static void appdb_delete(int index)
{
	if (index < 0 || index >= appdb_apps)
		return;

	flex_midextend((flex_ptr) &appdb_list, (index + 1) * sizeof(struct application),
			-sizeof(struct application));
}

