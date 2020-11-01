/* Copyright 2002-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
	icondb_instances = NULL;
}

/**
 * Terminate the icon database.
 */

void icondb_terminate(void)
{
	/* Free any instances. */

	while (icondb_instances != NULL)
		icondb_destroy_instance(icondb_instances);
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
	struct icondb_block *parent;

	if (instance == NULL)
		return;

	/* Deallocate any memory from the icon list. */

	icondb_reset_instance(instance);

	/* Delink the instance. */

	if (icondb_instances == instance) {
		icondb_instances = instance->next;
	} else {
		for (parent = icondb_instances; parent != NULL && parent->next != instance; parent = parent->next);

		if (parent != NULL && parent->next == instance)
			parent->next = instance->next;
	}

	/* Free the memory used. */

	heap_free(instance);
}

/**
 * Reset an icon database instance, clearing all of the icons.
 *
 * \param *instance	The instance to reset.
 */

void icondb_reset_instance(struct icondb_block *instance)
{
	if (instance == NULL)
		return;

	while (instance->buttons != NULL)
		icondb_delete_icon(instance, instance->buttons);
}

/**
 * Create a new button entry in an icon database instance, and
 * link it in according to position in the panel.
 *
 * \param *instance	The instance to add the button to.
 * \param key		The key to assign to the new entry.
 * \param *position	The position to assign to the new entry.
 * \return		Pointer to the new entry, or NULL on failure.
 */

struct icondb_button *icondb_create_icon(struct icondb_block *instance, unsigned key, os_coord *position)
{
	struct icondb_button *button = NULL, **current = NULL;

	if (instance == NULL || key == APPDB_NULL_KEY)
		return NULL;

	button = heap_alloc(sizeof(struct icondb_button));

	if (button == NULL)
		return NULL;

	/* Fill in the default values. */

	button->key = key;
	button->window = NULL;
	button->icon = -1;
	button->text = NULL;
	button->position.x = position->x;
	button->position.y = position->y;
	button->inset.x0 = 0;
	button->inset.y0 = 0;
	button->inset.x1 = 0;
	button->inset.y1 = 0;

	/* Link the icon into the database, in descending position order. */

	current = &(instance->buttons);

	while ((*current != NULL) && (((*current)->position.y < position->y) ||
			(((*current)->position.y == position->y) && ((*current)->position.x < position->x))))
		current = &((*current)->next);

	button->next = *current;
	*current = button;

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

	if (button->text != NULL)
		heap_free(button->text);

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

