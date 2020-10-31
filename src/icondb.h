/* Copyright 2003-2020, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Launcher:
 *
 *   http://www.stevefryatt.org.uk/risc-os
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
 * \file: icondb.h
 */

#ifndef LAUNCHER_ICONDB
#define LAUNCHER_ICONDB

/**
 * The maximum length allowed for an icon's validation string.
 */

#define ICONDB_VALIDATION_LENGTH 40

/**
 * An icon database instance.
 */

struct icondb_block;

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
	 * The actual position of the icon in the panel, after reflowing.
	 */

	os_coord	position;

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
 * Construct a new icon database instance.
 * 
 * \return		The new instance, or NULL.
 */

struct icondb_block *icondb_create_instance();


/**
 * Destroy an icon database instance, freeing up memory.
 * 
 * \param *instance	The instance to destroy.
 */

void icondb_destroy_instance(struct icondb_block *instance);


/**
 * Reset an icon database instance, clearing all of the icons.
 *
 * \param *instance	The instance to reset.
 */

void icondb_reset_instance(struct icondb_block *instance);


/**
 * Create a new button entry in an icon database instance.
 *
 * \param *instance	The instance to add the button to.
 * \param key		The key to assign to the new entry.
 * \param *position	The position to assign to the new entry.
 * \return		Pointer to the new entry, or NULL on failure.
 */

struct icondb_button *icondb_create_icon(struct icondb_block *instance, unsigned key, os_coord *position);


/**
 * Delete a button entry from an icon database instance.
 *
 * \param *instance	The instance to delete the button from.
 * \param *button	Pointer to the entry to delete.
 */

void icondb_delete_icon(struct icondb_block *instance, struct icondb_button *button);


/**
 * Request the first entry in an icon database instance.
 *
 * \param *instance	The instance to return the button from.
 * \return		Pointer to the first entry, or NULL.
 */

struct icondb_button *icondb_get_list(struct icondb_block *instance);


/**
 * Given an icon handle, find the associated entry in an icon
 * database instance.
 *
 * \param *instance	The instance to search within.
 * \param window	The window handle to search for.
 * \param icon		The icon handle to search for.
 * \return		The associated IconDB entry, or NULL.
 */

struct icondb_button *icondb_find_icon(struct icondb_block *instance, wimp_w window, wimp_i icon);

#endif

