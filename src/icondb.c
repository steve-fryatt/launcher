/* Copyright 2002-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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

/**
 * Structure to hold an icondb instance.
 */

struct icondb_block {
	/**
	 * The button list associated with the instance.
	 */
	struct icondb_button	*buttons;

	/**
	 * The next instance in the list, or NULL.
	 */
	struct icondb_block	*next;
};

/* Global Variables */

static struct icondb_block *icondb_instances = NULL;



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
	struct icondb_block *current;

	/* Free any instances. */

	while (icondb_instances != NULL) {
		current = icondb_instances;
		icondb_instances = current->next;
		icondb_destroy_instance(current);
	}
}

/**
 * Construct a new icon database instance.
 * 
 * \return		The new instance, or NULL.
 */

struct icondb_block *icondb_create_instance()
{
	struct icondb_block *new = NULL;

	new = heap_alloc(sizeof(struct icondb_block));
	if (new == NULL)
		return NULL;

	/* Initialise the instance data. */

	new->buttons = NULL;

	/* Link the instance into the list. */

	new->next = icondb_instances;
	icondb_instances = new;

	return new;
}

/**
 * Destroy an icon database instance, freeing up memory.
 * 
 * \param *instance	The instance to destroy.
 */

void icondb_destroy_instance(struct icondb_block *instance)
{
	if (instance == NULL)
		return;

	icondb_reset_instance(instance);
	heap_free(instance);
}

/**
 * Reset an icon database instance, clearing all of the icons.
 *
 * \param *instance	The instance to reset.
 */

void icondb_reset_instance(struct icondb_block *instance)
{
	struct icondb_button *current = NULL;

	if (instance == NULL)
		return;

	while (instance->buttons != NULL) {
		current = instance->buttons;
		instance->buttons = current->next;
		heap_free(current);
	}
}

/**
 * Create a new button entry in an icon database instance.
 *
 * \param *instance	The instance to add the button to.
 * \param key		The key to assign to the new entry.
 * \return		Pointer to the new entry, or NULL on failure.
 */

struct icondb_button *icondb_create_icon(struct icondb_block *instance, unsigned key)
{
	struct icondb_button *button = NULL;

	if (instance == NULL || key == APPDB_NULL_KEY)
		return NULL;

	button = heap_alloc(sizeof(struct icondb_button));

	if (button == NULL)
		return NULL;

	button->key = key;
	button->window = NULL;
	button->icon = -1;
	button->validation[0] = '\0';

	button->next = instance->buttons;
	instance->buttons = button;

	return button;
}


/**
 * Delete a button entry from an icon database instance.
 *
 * \param *instance	The instance to delete the button from.
 * \param *button	Pointer to the entry to delete.
 */

void icondb_delete_icon(struct icondb_block *instance, struct icondb_button *button)
{
	struct icondb_button *parent = NULL;

	if (instance == NULL)
		return;

	if (instance->buttons == button) {
		instance->buttons = button->next;
	} else {
		parent = instance->buttons;

		while (parent != NULL && parent->next != button)
			parent = parent->next;

		if (parent != NULL)
			parent->next = button->next;
	}

	heap_free(button);
}


/**
 * Request the first entry in an icon database instance.
 *
 * \param *instance	The instance to return the button from.
 * \return		Pointer to the first entry, or NULL.
 */

struct icondb_button *icondb_get_list(struct icondb_block *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->buttons;
}


/**
 * Given an icon handle, find the associated entry in an icon
 * database instance.
 *
 * \param *instance	The instance to search within.
 * \param window	The window handle to search for.
 * \param icon		The icon handle to search for.
 * \return		The associated IconDB entry, or NULL.
 */

struct icondb_button *icondb_find_icon(struct icondb_block *instance, wimp_w window, wimp_i icon)
{
	struct icondb_button *button;

	if (instance == NULL)
		return NULL;
	
	button = instance->buttons;

	while (button != NULL && (button->window != window || button->icon != icon))
		button = button->next;

	return button;
}

