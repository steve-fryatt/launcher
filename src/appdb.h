/* Launcher - appdb.h
 * (c) Stephen Fryatt, 2012
 *
 * Desktop application launcher.
 */

#ifndef LAUNCHER_APPDB
#define LAUNCHER_APPDB

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

#endif

