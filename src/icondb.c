/* Copyright 2002-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: icondb.c
 */

/* ANSI C header files. */

/* OSLib header files. */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/heap.h"

/* Application header files. */

#include "icondb.h"

#include "appdb.h"

/* Global Variables */



static struct icondb_button	*icondb_button_list = NULL;


/**
 * Initialise the icon database.
 */

void icondb_initialise(void)
{
}

/**
 * Terminate the icon database.
 */

void icondb_terminate(void)
{
	struct icondb_button *current;

	/* Free the allocated memory blocks. */

	while (icondb_button_list != NULL) {
		current = icondb_button_list;
		icondb_button_list = current->next;
		heap_free(current);
	}
}


/**
 * Create a new button entry in the IconDB.
 *
 * \param key		The key to assign to the new entry.
 * \return		Pointer to the new entry, or NULL on failure.
 */

struct icondb_button *icondb_create_icon(unsigned key)
{
	struct icondb_button *button = NULL;

	if (key == APPDB_NULL_KEY)
		return NULL;

	button = heap_alloc(sizeof(struct icondb_button));

	if (button == NULL)
		return NULL;

	button->key = key;
	button->window = NULL;
	button->icon = -1;
	button->validation[0] = '\0';

	button->next = icondb_button_list;
	icondb_button_list = button;

	return button;
}


/**
 * Delete a button entry from the IconDB.
 *
 * \param *button	Pointer to the entry to delete.
 */

void icondb_delete_icon(struct icondb_button *button)
{
	struct icondb_button *parent = NULL;

	if (icondb_button_list == button) {
		icondb_button_list = button->next;
	} else {
		parent = icondb_button_list;

		while (parent != NULL && parent->next != button)
			parent = parent->next;

		if (parent != NULL)
			parent->next = button->next;
	}

	heap_free(button);
}


/**
 * Request the first entry in the IconDB list.
 *
 * \return		Pointer to the first entry, or NULL.
 */

struct icondb_button *icondb_get_list(void)
{
	return icondb_button_list;
}


/**
 * Given an icon handle, find the associated IconDB entry.
 *
 * \param window	The window handle to search for.
 * \param icon		The icon handle to search for.
 * \return		The associated IconDB entry, or NULL.
 */

struct icondb_button *icondb_find_icon(wimp_w window, wimp_i icon)
{
	struct icondb_button *button = icondb_button_list;

	while (button != NULL && (button->window != window || button->icon != icon))
		button = button->next;

	return button;
}

