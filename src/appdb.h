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
 * \file: appdb.h
 */

#ifndef LAUNCHER_APPDB
#define LAUNCHER_APPDB

#define APPDB_NULL_KEY 0xffffffffu

#define APPDB_NAME_LENGTH 64
#define APPDB_SPRITE_LENGTH 20
#define APPDB_COMMAND_LENGTH 1024


/**
 * Initialise the buttons window.
 */

void appdb_initialise(void);


/**
 * Terminate the application database.
 */

void appdb_terminate(void);


/**
 * Load the contents of a button file into the buttons database.
 *
 * \param *leaf_name		The file leafname to load.
 * \return			TRUE on success; else FALSE.
 */

osbool appdb_load_file(char *leaf_name);


/**
 * Save the contents of the buttons database into a buttons file.
 *
 * \param *leaf_name		The file leafname to save to.
 * \return			TRUE on success; else FALSE.
 */

osbool appdb_save_file(char *leaf_name);


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
 * Delete an enrty from the database.
 *
 * \param key			The key of the enrty to delete.
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
		char **command, osbool *local_copy, osbool *filer_boot);


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
		char *command, osbool local_copy, osbool filer_boot);

#endif

