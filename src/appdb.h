/* Copyright 2012-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: appdb.h
 */

#ifndef LAUNCHER_APPDB
#define LAUNCHER_APPDB

#define APPDB_NULL_KEY 0xffffffffu
#define APPDB_NULL_PANEL 0xffffffffu

#define APPDB_NAME_LENGTH 64
#define APPDB_SPRITE_LENGTH 20
#define APPDB_COMMAND_LENGTH 1024

#include <stdio.h>
#include "filing.h"


/**
 * Application data structure -- Implementation.
 */

struct appdb_entry {
	/**
	 * The target panel key.
	 */

	unsigned	panel;

	/**
	 * Button name.
	 */

	char		name[APPDB_NAME_LENGTH];

	/**
	 * The position of the button in the window.
	 */

	os_coord	position;

	/**
	 * The sprite name.
	 */

	char		sprite[APPDB_SPRITE_LENGTH];

	/**
	 * Do we keep a local copy of the sprite?
	 */

	osbool		local_copy;

	/**
	 * The command to be executed.
	 */

	char		command[APPDB_COMMAND_LENGTH];

	/**
	 * Should the item be Filer_Booted on startup?
	 */

	osbool		filer_boot;
};




/**
 * Initialise the application database.
 */

void appdb_initialise(void);


/**
 * Terminate the application database.
 */

void appdb_terminate(void);


/**
 * Reset the application database.
 */

void appdb_reset(void);


/**
 * Load the contents of an old format button file into the buttons
 * database.
 *
 * \param *in		The filing operation to load from.
 * \param panel		The index of the panel to add the entries to.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_load_old_file(struct filing_block *in, int panel);


/**
 * Load the contents of a new format button file into the buttons
 * database.
 *
 * \param *in		The filing operation to load from.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_load_new_file(struct filing_block *in);


/**
 * Once panels and buttons are loaded, scan the buttons replacing the
 * panel indexes with the associated panel keys.
 *
 * \return		TRUE if successful; FALSE if errors occurred.
 */

osbool appdb_complete_file_load(void);


/**
 * Save the contents of the buttons database into a buttons file.
 *
 * \param *file		The file handle to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool appdb_save_file(FILE *file);


/**
 * Filer_Boot all of the applications stored in the application database, if
 * selected.
 */

void appdb_boot_all(void);


/**
 * Create a new, empty entry in the database and return its key.
 *
 * \return			The new key, or APPDB_NULL_KEY.
 */

unsigned appdb_create_key(void);


/**
 * Delete an entry from the database.
 *
 * \param key			The key of the entry to delete.
 */

void appdb_delete_key(unsigned key);


/**
 * Given a database key, return the next key from the database.
 *
 * \param key			The current key, or APPDB_NULL_KEY to start sequence.
 * \return			The next key, or APPDB_NULL_KEY.
 */

unsigned appdb_get_next_key(unsigned key);


/**
 * Given a database key, return the associated button panel ID.
 *
 * \param key		The database key to query.
 * \return		The associated panel ID, or APPDB_NULL_PANEL.
 */

unsigned appdb_get_panel(unsigned key);


/**
 * Given a key, return details of the button associated with the application.
 * If a structure is provided, the data is copied into it; otherwise, a pointer
 * to a structure in the flex heap is returned which will remain valid only
 * the heap contents are changed.
 * 
 *
 * \param key			The key of the entry to be returned.
 * \param *data			Pointer to structure to return the data, or NULL.
 * \return			Pointer to the returned data, or NULL on failure.
 */

struct appdb_entry *appdb_get_button_info(unsigned key, struct appdb_entry *data);


/**
 * Given a data structure, set the details of a database entry by copying the
 * contents of the structure into the database.
 *
 * \param key		The key of the entry to update.
 * \param *data		Pointer to the structure containing the data.
 * \return		TRUE if an entry was updated; else FALSE.
 */

osbool appdb_set_button_info(unsigned key, struct appdb_entry *data);


/**
 * Copy the contents of an application block into a second block.
 *
 * \param *to		The block to copy the data to.
 * \param *from		The block to copy the data from.
 */

void appdb_copy(struct appdb_entry *to, struct appdb_entry *from);


/**
 * Set some default values for an AppDB entry.
 *
 * \param *entry	The entry to set.
 */

void appdb_set_defaults(struct appdb_entry *entry);

#endif
