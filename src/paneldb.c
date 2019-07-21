/* Copyright 2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: paneldb.c
 */

/* ANSI C header files. */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files. */

#include "oslib/os.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files. */

#include "paneldb.h"

#include "filing.h"

/**
 * The number of new blocks to allocate when more space is required.
 */

#define PANELDB_ALLOC_CHUNK 10

/* Global Variables. */

/**
 * The names of the possible panel positions.
 *
 * This must match the enum defined in paneldb.h.
 */

static char *paneldb_position_names[] = { "Left", "Right", "Top", "Bottom" };

/**
 * The flex array of panel data.
 */

static struct paneldb_entry		*paneldb_list = NULL;

/**
 * The number of panels stored in the database.
 */

static int				paneldb_panels = 0;

/**
 * The number of panels for which space is allocated.
 */

static int				paneldb_allocation = 0;

/**
 * The next unique database primary key.
 */

static unsigned				paneldb_key = 0;

/* Static Function Prototypes. */

static int	paneldb_find(unsigned key);
static int	paneldb_new();
static void	paneldb_delete(int index);

/**
 * Initialise the panels database.
 */

void paneldb_initialise(void)
{
	if (flex_alloc((flex_ptr) &paneldb_list,
			(paneldb_allocation + PANELDB_ALLOC_CHUNK) * sizeof(struct paneldb_entry)) == 1)
		paneldb_allocation += PANELDB_ALLOC_CHUNK;
}


/**
 * Terminate the panels database.
 */

void paneldb_terminate(void)
{
	if (paneldb_list != NULL)
		flex_free((flex_ptr) &paneldb_list);
}


/**
 * Create a single, default panel to match the old single-panel version
 * of Launcher.
 *
 * \return		The panel key on success; else PANELDB_NULL_KEY.
 */

unsigned paneldb_create_old_panel(void)
{
	int current = -1;

	debug_printf("Loading an old file...");

	current = paneldb_new();
	if (current == -1)
		return PANELDB_NULL_KEY;

	paneldb_list[current].position = PANELDB_POSITION_LEFT;
	string_copy(paneldb_list[current].name, "Default", PANELDB_NAME_LENGTH);

	return paneldb_list[current].key;
}


/**
 * Load the contents of a new format button file into the panels
 * database.
 *
 * \param *in		The filing operation to load from.
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_load_new_file(struct filing_block *in)
{
	return FALSE;
}


/**
 * Save the contents of the panels database into a buttons file.
 *
 * \param *file		The file handle to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_save_file(FILE *file)
{
	int	current;

	if (file == NULL)
		return FALSE;

	fprintf(file, "[Panels]\n");

	for (current = 0; current < paneldb_panels; current++) {
		fprintf(file, "\n@:%s\n", paneldb_list[current].name);
	}

	return TRUE;
}


/**
 * Create a new, empty entry in the database and return its key.
 *
 * \return		The new key, or PANELDB_NULL_KEY.
 */

unsigned paneldb_create_key(void)
{
	unsigned	key = PANELDB_NULL_KEY;
	int		index = paneldb_new();

	if (index != -1)
		key = paneldb_list[index].key;

	return key;
}


/**
 * Delete an entry from the database.
 *
 * \param key		The key of the entry to delete.
 */

void paneldb_delete_key(unsigned key)
{
	int		index;

	if (key == PANELDB_NULL_KEY)
		return;

	index = paneldb_find(key);

	if (index != -1)
		paneldb_delete(index);
}


/**
 * Given a database key, return the next key from the database.
 *
 * \param key		The current key, or PANELDB_NULL_KEY to start sequence.
 * \return		The next key, or PANELDB_NULL_KEY.
 */

unsigned paneldb_get_next_key(unsigned key)
{
	int index;

	if (key == PANELDB_NULL_KEY && paneldb_panels > 0)
		return paneldb_list[0].key;

	index = paneldb_find(key);

	return (index != -1 && index < (paneldb_panels - 1)) ? paneldb_list[index + 1].key : PANELDB_NULL_KEY;
}


/**
 * Given a key, return details of the panel associated with it. If a structure
 * is provided, the data is copied into it; otherwise, a pointer to a structure
 * in the flex heap is returned which will remain valid only the heap contents
 * are changed.
 * 
 *
 * \param key		The key of the entry to be returned.
 * \param *data		Pointer to structure to return the data, or NULL.
 * \return		Pointer to the returned data, or NULL on failure.
 */

struct paneldb_entry *paneldb_get_panel_info(unsigned key, struct paneldb_entry *data)
{
	int index;

	/* Locate the requested key, and return NULL if it isn't found. */

	index = paneldb_find(key);

	if (index == -1)
		return NULL;

	/* If no buffer is supplied, just return a pointer into the flex heap. */

	if (data == NULL)
		return paneldb_list + index;

	/* Copy the data into the supplied buffer. */

	paneldb_copy(data, paneldb_list + index);

	return data;
}


/**
 * Given a data structure, set the details of a database entry by copying the
 * contents of the structure into the database.
 *
 * \param *data		Pointer to the structure containing the data.
 * \return		TRUE if an entry was updated; else FALSE.
 */

osbool paneldb_set_panel_info(struct paneldb_entry *data)
{
	int index;

	if (data == NULL)
		return FALSE;

	index = paneldb_find(data->key);

	if (index == -1)
		return FALSE;

	paneldb_copy(paneldb_list + index, data);

	return TRUE;
}


/**
 * Find the index of a panel based on its key.
 *
 * \param key		The key to locate.
 * \return		The current index, or -1 if not found.
 */

static int paneldb_find(unsigned key)
{
	int index;

	/* We know that keys are allocated in ascending order, possibly
	 * with gaps in the sequence.  Therefore we can hit the list
	 * at the corresponding index (or the end) and count back until
	 * we pass the key we're looking for.
	 */

	index = (key >= paneldb_panels) ? paneldb_panels - 1 : key;

	while (index >= 0 && paneldb_list[index].key > key)
		index--;

	if (index != -1 && paneldb_list[index].key != key)
		index = -1;

	return index;
}


/**
 * Claim a block for a new panel, fill in the unique key and set
 * default values for the data.
 *
 * \return		The new block number, or -1 on failure.
 */

static int paneldb_new()
{
	if (paneldb_panels >= paneldb_allocation && flex_extend((flex_ptr) &paneldb_list,
			(paneldb_allocation + PANELDB_ALLOC_CHUNK) * sizeof(struct paneldb_entry)) == 1)
		paneldb_allocation += PANELDB_ALLOC_CHUNK;

	if (paneldb_panels >= paneldb_allocation)
		return -1;

	paneldb_list[paneldb_panels].key = paneldb_key++;
	paneldb_list[paneldb_panels].position = PANELDB_POSITION_LEFT;

	*(paneldb_list[paneldb_panels].name) = '\0';

	return paneldb_panels++;
}


/**
 * Delete a panel block, given its index.
 *
 * \param index		The index of the block to be deleted.
 */

static void paneldb_delete(int index)
{
	if (index < 0 || index >= paneldb_panels)
		return;

	flex_midextend((flex_ptr) &paneldb_list, (index + 1) * sizeof(struct paneldb_entry),
			-sizeof(struct paneldb_entry));
}


/**
 * Copy the contents of a panel block into a second block.
 *
 * \param *to		The block to copy the data to.
 * \param *from		The block to copy the data from.
 */

void paneldb_copy(struct paneldb_entry *to, struct paneldb_entry *from)
{
	string_copy(to->name, from->name, PANELDB_NAME_LENGTH);
}

