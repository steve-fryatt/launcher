/* Launcher - appdb.h
 * (c) Stephen Fryatt, 2012
 *
 * Desktop application launcher.
 */

#ifndef LAUNCHER_APPDB
#define LAUNCHER_APPDB


#define APPDB_NULL_KEY 0xffffffffu

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

#endif

