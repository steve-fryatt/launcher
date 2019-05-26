/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: icondb.h
 */

#ifndef LAUNCHER_ICONDB
#define LAUNCHER_ICONDB

/**
 * The maximum length allowed for an icon's validation string.
 */

#define ICONDB_VALIDATION_LENGTH 40

/**
 * Structure to hold the definition of a button's icon.
 */

struct icondb_button {

	/**
	 * The database key relating to the button.
	 */

	unsigned	key;

	/**
	 * The Wimp's handle for the panel window.
	 */

	wimp_w		window;

	/**
	 * The Wimp's handle for the icon.
	 */

	wimp_i		icon;

	/**
	 * Storage for the icon's validation string.
	 */

	char		validation[ICONDB_VALIDATION_LENGTH];

	/**
	 * Pointer to the next button definition.
	 */

	struct icondb_button	*next;
};


/**
 * Initialise the icon database.
 */

void icondb_initialise(void);


/**
 * Terminate the icon database.
 */

void icondb_terminate(void);


/**
 * Create a new button entry in the IconDB.
 *
 * \param key		The key to assign to the new entry.
 * \return		Pointer to the new entry, or NULL on failure.
 */

struct icondb_button *icondb_create_icon(unsigned key);


/**
 * Delete a button entry from the IconDB.
 *
 * \param *button	Pointer to the entry to delete.
 */

void icondb_delete_icon(struct icondb_button *button);


/**
 * Request the first entry in the IconDB list.
 *
 * \return		Pointer to the first entry, or NULL.
 */

struct icondb_button *icondb_get_list(void);


/**
 * Given an icon handle, find the associated IconDB entry.
 *
 * \param window	The window handle to search for.
 * \param icon		The icon handle to search for.
 * \return		The associated IconDB entry, or NULL.
 */

struct icondb_button *icondb_find_icon(wimp_w window, wimp_i icon);

#endif

