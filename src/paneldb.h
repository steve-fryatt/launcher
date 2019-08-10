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
 * The possible panel positions.
 *
 * This enum must match the names in the paneldb.c file.
 */

enum paneldb_position {
	PANELDB_POSITION_LEFT,
	PANELDB_POSITION_RIGHT,
	PANELDB_POSITION_TOP,
	PANELDB_POSITION_BOTTOM,
	PANELDB_POSITION_MAX,
	PANELDB_POSITION_UNKNOWN
};

/**
 * Panel data structure -- Implementation.
 */

struct paneldb_entry {

	/**
	 * Primary key to index database entries.
	 */
	unsigned		key;

	/**
	 * Panel name.
	 */
	char			name[PANELDB_NAME_LENGTH];

	/**
	 * Panel position.
	 */
	enum paneldb_position	position;

	/**
	 * Panel width weight.
	 */
	int			width;

	/**
	 * Panel sort value.
	 */
	int			sort;
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
 * Reset the panels database.
 */

void paneldb_reset(void);


/**
 * Create a single, default panel to match the old single-panel version
 * of Launcher.
 *
 * \return		The panel index on success; else -1.
 */

int paneldb_create_old_panel(void);


/**
 * Save the contents of a new format button file into the panels
 * database.
 *
 * \param *in		The filing operation to load from.
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_load_new_file(struct filing_block *in);


/**
 * Create a default bar if none exists in the database.
 *
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_create_default(void);


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
 * Given a key, return a pointer to the name of the panel associated with
 * it. The data pointed to is in the flex heap, and so will only remain
 * valid until the heap contents are changed.
 *
 * \param key		The key of the entry to be returned.
 * \return		Pointer to the name, or "" on failure.
 */

char *paneldb_get_name(unsigned key);


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
 * Given a panel name, look it up in the database. If a match is found,
 * return the index. Otherwise, create a new phantom entry with a
 * key of PANELDB_NULL_KEY, fill in the name, and return that index.
 *
 * \param *name		The name to look up.
 * \return		The index of the entry.
 */

int paneldb_lookup_name(char *name);


/**
 * Given an index, return the key associated with it.
 *
 * \param index		The index to look up.
 * \return		The associated key, or PANELDB_NULL_KEY.
 */

unsigned paneldb_lookup_key(int index);


/**
 * Copy the contents of a panel block into a second block.
 *
 * \param *to		The block to copy the data to.
 * \param *from		The block to copy the data from.
 */

void paneldb_copy(struct paneldb_entry *to, struct paneldb_entry *from);

#endif

