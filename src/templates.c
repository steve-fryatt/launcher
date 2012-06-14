/* Launcher - templates.c
 *
 * (C) Stephen Fryatt, 2012
 */

/* ANSI C header files */

#include <stdlib.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/windows.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/debug.h"
#include "sflib/url.h"

/* Application header files */

#include "templates.h"

#include "ihelp.h"

#define TEMPLATES_HELP_TOKEN_LENGTH 64

static wimp_menu	*menu_up = NULL;					/**< The currently open menu.					*/
static wimp_menu	*templates_menu_list[TEMPLATES_MENU_MAX_EXTENT];	/**< The menu definitions loaded from the menus template.	*/
static menu_template	menu_definitions;					/**< The menu definition block handle.				*/
static char		templates_menu_help_token[TEMPLATES_HELP_TOKEN_LENGTH];	/**< The current token to be used for interactive help.		*/


/**
 * Open the window templates file for processing.
 *
 * \param *file		The template file to open.
 */

void templates_open(char *file)
{
	os_error	*error;

	error = xwimp_open_template(file);
	if (error != NULL)
		error_report_program(error);
}


/**
 * Close the window templates file.
 */

void templates_close(void)
{
	wimp_close_template();
}


/**
 * Load a window definition from the current templates file.
 *
 * \param *name		The name of the template to load.
 * \return		Pointer to the loaded definition, or NULL.
 */

wimp_window *templates_load_window(char *name)
{
	wimp_window	*definition;

	definition = windows_load_template(name);

	if (definition == NULL)
		error_msgs_report_fatal("BadTemplate");

	return definition;
}


/**
 * Create a window from the current templates file.
 *
 * \param *name		The name of the window to create.
 * \return		The window handle if the new window.
 */

wimp_w templates_create_window(char *name)
{
	wimp_window	*definition;
	wimp_w		w = NULL;

	definition = windows_load_template(name);

	if (definition != NULL) {
		w = wimp_create_window(definition);
		free(definition);
	} else {
		error_msgs_report_fatal("BadTemplate");
	}

	return w;
}


/**
 * Load the menu file into memory and link in any dialogue boxes.  The
 * templates file must be open when this function is called.
 *
 * \param *file		The filename of the menu file.
 */

void templates_load_menus(char *file)
{
	menu_definitions = menus_load_templates(file, NULL, templates_menu_list, TEMPLATES_MENU_MAX_EXTENT);
	event_set_menu_pointer(&menu_up);
}


/**
 * Link a dialogue box to the menu block once the template has been
 * loaded into memory.
 *
 * \param *dbox		The dialogue box name in the menu tree.
 * \param w		The window handle to link.
 */

void templates_link_menu_dialogue(char *dbox, wimp_w w)
{
	menus_link_dbox(menu_definitions, dbox, w);
}


/**
 * Return a menu handle based on a menu file index value.
 *
 * \param menu		The menu file index of the required menu.
 * \return		The menu block pointer, or NULL for an invalid index.
 */

wimp_menu *templates_get_menu(enum templates_menus menu)
{
	if (menu >= 0 && menu < TEMPLATES_MENU_MAX_EXTENT)
		return templates_menu_list[menu];

	return NULL;
}


/**
 * Update the address of a menu to reflect changes in status from within
 * a client module.
 *
 * \param menu		The menu file index of the required menu.
 * \param *address	The new address of the menu block.
 */

void templates_set_menu(enum templates_menus menu, wimp_menu *address)
{
	if (menu >= 0 && menu < TEMPLATES_MENU_MAX_EXTENT)
		templates_menu_list[menu] = address;

}


/**
 * Update the interactive help token to be supplied for undefined
 * menus.
 *
 * \param *token	The token to use, or NULL to unset.
 */

void templates_set_menu_token(char *token)
{
	if (token == NULL)
		strcpy(templates_menu_help_token, "");
	else
		strcpy(templates_menu_help_token, token);
}


/**
 * Set details of the menu handle which is currently on screen.
 *
 * \param *menu		The menu handle currently on screen.
 */

void templates_set_menu_handle(wimp_menu *menu)
{
	menu_up = menu;
}


/**
 * Return a pointer to the name of the current menu.
 *
 * \param *buffer	Pointer to a buffer to hold the menu name.
 * \return		Pointer to the returned name.
 */

char *templates_get_current_menu_name(char *buffer)
{
	if (buffer == NULL)
		return NULL;

	*buffer = '\0';

	if (menu_up == templates_menu_list[TEMPLATES_MENU_MAIN])
		strcpy(buffer, "MainMenu");
	else
		strcpy(buffer, templates_menu_help_token);

	return buffer;
}

