/* Launcher - templates.h
 *
 * (C) Stephen Fryatt, 2012
 */

#ifndef LAUNCHER_TEMPLATES
#define LAUNCHER_TEMPLATES

/**
 * The menus defined in the menus template, in the order that they
 * appear.
 */

enum templates_menus {
	TEMPLATES_MENU_MAIN = 0,						/**< The Main Menu.			*/
	TEMPLATES_MENU_MAX_EXTENT						/**< Determine the number of entries.	*/
};


/**
 * Open the window templates file for processing.
 *
 * \param *file		The template file to open.
 */

void templates_open(char *file);


/**
 * Close the window templates file.
 */

void templates_close(void);


/**
 * Load a window definition from the current templates file.
 *
 * \param *name		The name of the template to load.
 * \return		Pointer to the loaded definition, or NULL.
 */

wimp_window *templates_load_window(char *name);


/**
 * Create a window from the current templates file.
 *
 * \param *name		The name of the window to create.
 * \return		The window handle if the new window.
 */


wimp_w templates_create_window(char *name);

/**
 * Load the menu file into memory and link in any dialogue boxes.  The
 * templates file must be open when this function is called.
 *
 * \param *file		The filename of the menu file.
 */

void templates_load_menus(char *file);


/**
 * Link a dialogue box to the menu block once the template has been
 * loaded into memory.
 *
 * \param *dbox		The dialogue box name in the menu tree.
 * \param w		The window handle to link.
 */

void templates_link_menu_dialogue(char *dbox, wimp_w w);


/**
 * Return a menu handle based on a menu file index value.
 *
 * \param menu		The menu file index of the required menu.
 * \return		The menu block pointer, or NULL for an invalid index.
 */

wimp_menu *templates_get_menu(enum templates_menus menu);


/**
 * Update the address of a menu to reflect changes in status from within
 * a client module.
 *
 * \param menu		The menu file index of the required menu.
 * \param *address	The new address of the menu block.
 */

void templates_set_menu(enum templates_menus menu, wimp_menu *address);


/**
 * Update the interactive help token to be supplied for undefined
 * menus.
 *
 * \param *token	The token to use.
 */

void templates_set_menu_token(char *token);


/**
 * Set details of the menu handle which is currently on screen.
 *
 * \param *menu		The menu handle currently on screen.
 */

void templates_set_menu_handle(wimp_menu *menu);


/**
 * Return a pointer to the name of the current menu.
 *
 * \param *buffer	Pointer to a buffer to hold the menu name.
 * \return		Pointer to the returned name.
 */

char *templates_get_current_menu_name(char *buffer);

#endif

