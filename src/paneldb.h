/* Copyright 2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: paneldb.h
 */

#ifndef LAUNCHER_PANELDB
#define LAUNCHER_PANELDB

#define PANELDB_NULL_KEY 0xffffffffu

#define PANELDB_NAME_LENGTH 64

#include <stdio.h>
#include "filing.h"


/**
 * Panel data structure -- Implementation.
 */

struct paneldb_entry {

	/**
	 * Primary key to index database entries.
	 */

	unsigned	key;

	/**
	 * Panel name.
	 */

	char		name[PANELDB_NAME_LENGTH];
};




/**
 * Initialise the panel database.
 */

void paneldb_initialise(void);


/**
 * Terminate the panel database.
 */

void paneldb_terminate(void);


/**
 * Create a single, default panel to match the old single-panel version
 * of Launcher.
 *
 * \return		The panel key on success; else PANELDB_NULL_KEY.
 */

unsigned paneldb_create_old_panel(void);


/**
 * Save the contents of a new format button file into the panels
 * database.
 *
 * \param *in		The filing operation to load from.
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_load_new_file(struct filing_block *in);


/**
 * Save the contents of the panels database into a buttons file.
 *
 * \param *file		The file handle to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_save_file(FILE *file);


/**
 * Create a new, empty entry in the database and return its key.
 *
 * \return			The new key, or PANELDB_NULL_KEY.
 */

unsigned paneldb_create_key(void);


/**
 * Delete an entry from the database.
 *
 * \param key			The key of the entry to delete.
 */

void paneldb_delete_key(unsigned key);


/**
 * Given a database key, return the next key from the database.
 *
 * \param key			The current key, or PANELDB_NULL_KEY to start sequence.
 * \return			The next key, or PANELDB_NULL_KEY.
 */

unsigned paneldb_get_next_key(unsigned key);


/**
 * Given a key, return details of the panel associated with it. If a structure
 * is provided, the data is copied into it; otherwise, a pointer to a structure
 * in the flex heap is returned which will remain valid only the heap contents
 * are changed.
 * 
 *
 * \param key		The key of the entry to be returned.
 * \param *data		Pointer to structure to return the data, or NULL.
 * \return		Pointer to the returned data, or NULL on failure.
 */

struct paneldb_entry *paneldb_get_panel_info(unsigned key, struct paneldb_entry *data);


/**
 * Given a data structure, set the details of a database entry by copying the
 * contents of the structure into the database.
 *
 * \param *data			Pointer to the structure containing the data.
 * \return			TRUE if an entry was updated; else FALSE.
 */

osbool paneldb_set_panel_info(struct paneldb_entry *data);


/**
 * Copy the contents of a panel block into a second block.
 *
 * \param *to		The block to copy the data to.
 * \param *from		The block to copy the data from.
 */

void paneldb_copy(struct paneldb_entry *to, struct paneldb_entry *from);

#endif

