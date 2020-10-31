/* Copyright 2019-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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

/**
 * The internal database entry container.
 */

struct paneldb_container {
	/**
	 * Primary key to index database entries.
	 */

	unsigned		key;

	/**
	 * The database entry.
	 */

	struct paneldb_entry	entry;	
};

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

static struct paneldb_container		*paneldb_list = NULL;

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

/**
 * Track whether the data has changed since the last save.
 */

static osbool				paneldb_unsafe = FALSE;

/* Static Function Prototypes. */

static int paneldb_find(unsigned key);
static int paneldb_find_name(char *name);
static char *paneldb_position_to_name(enum paneldb_position position);
static enum paneldb_position paneldb_position_from_name(char *name);
static int paneldb_new(osbool allocate);
static void paneldb_delete(int index);

/**
 * Initialise the panels database.
 */

void paneldb_initialise(void)
{
	if (flex_alloc((flex_ptr) &paneldb_list,
			(paneldb_allocation + PANELDB_ALLOC_CHUNK) * sizeof(struct paneldb_container)) == 1)
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
 * Reset the panels database.
 */

void paneldb_reset(void)
{
	paneldb_panels = 0;
	paneldb_key = 0;
	paneldb_unsafe = FALSE;
}

/**
 * Create a single, default panel to match the old single-panel version
 * of Launcher.
 *
 * \return		The panel index on success; else -1.
 */

int paneldb_create_old_panel(void)
{
	int current = -1;

	current = paneldb_new(TRUE);
	if (current == -1)
		return -1;

	paneldb_list[current].entry.position = PANELDB_POSITION_LEFT;
	string_copy(paneldb_list[current].entry.name, "Default", PANELDB_NAME_LENGTH);

	return current;
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
	int current = -1;

	do {
		if (filing_test_token(in, "@")) {
			current = paneldb_lookup_name(filing_get_text_value(in, NULL, 0));

			if (current != -1) {
				if (paneldb_list[current].key == PANELDB_NULL_KEY) {
					paneldb_list[current].key = current;
				} else {
					filing_set_status(in, FILING_STATUS_CORRUPT);
					return FALSE;
				}
			} else {
				current = paneldb_new(TRUE);
			}

			if (current == -1) {
				 filing_set_status(in, FILING_STATUS_MEMORY);
				 return FALSE;
			}

			filing_get_text_value(in, paneldb_list[current].entry.name, PANELDB_NAME_LENGTH);
		} else if ((current != -1) && filing_test_token(in, "Position"))
			paneldb_list[current].entry.position = paneldb_position_from_name(filing_get_text_value(in, NULL, 0));
		else if ((current != -1) && filing_test_token(in, "Sort"))
			paneldb_list[current].entry.sort = filing_get_int_value(in);
		else if ((current != -1) && filing_test_token(in, "Width"))
			paneldb_list[current].entry.width = filing_get_int_value(in);
		else if ((current != -1) && filing_test_token(in, "SlabXSize"))
			paneldb_list[current].entry.slab_size.x = filing_get_int_value(in);
		else if ((current != -1) && filing_test_token(in, "SlabYSize"))
			paneldb_list[current].entry.slab_size.y = filing_get_int_value(in);
		else if ((current != -1) && filing_test_token(in, "Depth"))
			paneldb_list[current].entry.depth = filing_get_int_value(in);
		else if (!filing_test_token(in, ""))
			filing_set_status(in, FILING_STATUS_UNEXPECTED);
	} while (filing_get_next_token(in));

	paneldb_unsafe = FALSE;

	return TRUE;
}


/**
 * Create a default bar if none exists in the database.
 *
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_create_default(void)
{
	int index;

	if (paneldb_panels > 0)
		return TRUE;

	index = paneldb_new(TRUE);
	if (index == -1)
		return FALSE;

	string_copy(paneldb_list[index].entry.name, "Default", PANELDB_NAME_LENGTH);

	paneldb_unsafe = FALSE;

	return TRUE;
}


/**
 * Save the contents of the panels database into a buttons file.
 *
 * \param *file		The file handle to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool paneldb_save_file(FILE *file)
{
	int			current;
	struct paneldb_entry	*entry = NULL;

	if (file == NULL)
		return FALSE;

	if (paneldb_panels <= 0)
		return TRUE;

	fprintf(file, "\n[Panels]");

	for (current = 0; current < paneldb_panels; current++) {
		entry = &(paneldb_list[current].entry);

		fprintf(file, "\n@: %s\n", entry->name);
		fprintf(file, "Position: %s\n", paneldb_position_to_name(entry->position));
		fprintf(file, "Sort: %d\n", entry->sort);
		fprintf(file, "Width: %d\n", entry->width);
		fprintf(file, "SlabXSize: %d\n", entry->slab_size.x);
		fprintf(file, "SlabYSize: %d\n", entry->slab_size.y);
		fprintf(file, "Depth: %d\n", entry->depth);
	}

	paneldb_unsafe = FALSE;

	return TRUE;
}


/**
 * Indicate whether any data in the PanelDB is currently unsaved.
 * 
 * \return		TRUE if there is unsaved data; otherwise FALSE.
 */

osbool paneldb_data_unsafe(void)
{
	return paneldb_unsafe;
}


/**
 * Create a new, empty entry in the database and return its key.
 *
 * \return		The new key, or PANELDB_NULL_KEY.
 */

unsigned paneldb_create_key(void)
{
	unsigned	key = PANELDB_NULL_KEY;
	int		index = paneldb_new(TRUE);

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
 * Given a key, return a pointer to the name of the panel associated with
 * it. The data pointed to is in the flex heap, and so will only remain
 * valid until the heap contents are changed.
 *
 * \param key		The key of the entry to be returned.
 * \return		Pointer to the name, or "" on failure.
 */

char *paneldb_get_name(unsigned key)
{
	int index;

	/* Locate the index, and return an empty string if it isn't found. */

	index = paneldb_find(key);

	if (index == -1)
		return "";

	/* Return a volatile pointer to the name in the flex heap. */

	return paneldb_list[index].entry.name;
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
		return &(paneldb_list[index].entry);

	/* Copy the data into the supplied buffer. */

	paneldb_copy(data, &(paneldb_list[index].entry));

	return data;
}


/**
 * Given a data structure, set the details of a database entry by copying the
 * contents of the structure into the database.
 *
 * \param key		The key of the entry to be updated.
 * \param *data		Pointer to the structure containing the data.
 * \return		TRUE if an entry was updated; else FALSE.
 */

osbool paneldb_set_panel_info(unsigned key, struct paneldb_entry *data)
{
	int index;

	if (data == NULL)
		return FALSE;

	index = paneldb_find(key);

	if (index == -1)
		return FALSE;

	paneldb_copy(&(paneldb_list[index].entry), data);

	paneldb_unsafe = TRUE;

	return TRUE;
}


/**
 * Given a panel name, return its key.
 * 
 * \param *name		The name to look up.
 * \return		The panel key, or PANELDB_NULL_KEY if not found.
 */

osbool paneldb_key_from_name(char *name)
{
	int index = -1;

	index = paneldb_find_name(name);
	if (index == -1)
		return PANELDB_NULL_KEY;

	return paneldb_list[index].key;
}


/**
 * Given a panel name, look it up in the database. If a match is found,
 * return the index. Otherwise, create a new phantom entry with a
 * key of PANELDB_NULL_KEY, fill in the name, and return that index.
 *
 * This is ONLY for use during file loading, when we create all of the
 * linkages based on database index, then fill in the keys later to allow
 * the file to arrive in any order.
 *
 * \param *name		The name to look up.
 * \return		The index of the entry.
 */

int paneldb_lookup_name(char *name)
{
	int index;

	if (name == NULL)
		return -1;

	/* Attempt to find the name. */

	index = paneldb_find_name(name);

	/* If the name already exists, return the index. */

	if (index != -1)
		return index;

	/* If not, create a new entry and copy the name across. */

	index = paneldb_new(FALSE);

	if (index != -1)
		string_copy(paneldb_list[index].entry.name, name, PANELDB_NAME_LENGTH);

	return index;
}


/**
 * Given an index, return the key associated with it.
 *
 * \param index		The index to look up.
 * \return		The associated key, or PANELDB_NULL_KEY.
 */

unsigned paneldb_lookup_key(int index)
{
	if (index < 0 || index >= paneldb_panels)
		return PANELDB_NULL_KEY;

	return paneldb_list[index].key;
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
 * Find the index of a panel based on its name.
 *
 * \param *name		The name to locate.
 * \return		The current index, or -1 if not found.
 */

static int paneldb_find_name(char *name)
{
	int index;

	for (index = 0; index < paneldb_panels; index++) {
		if (string_nocase_strcmp(name, paneldb_list[index].entry.name) == 0)
			return index;
	}

	return -1;
}


/**
 * Convert a panel position into textual form.
 *
 * \param position	The position to be converted.
 * \return		Pointer to the corresponding name.
 */

static char *paneldb_position_to_name(enum paneldb_position position)
{
	if (position < 0 || position >= PANELDB_POSITION_MAX)
		return "Unknown";

	return paneldb_position_names[position];
}


/**
 * Convert a textual name into a panel position.
 *
 * \param *name		Pointer to the name to be converted.
 * \return		The corresponding position.
 */

static enum paneldb_position paneldb_position_from_name(char *name)
{
	int position;

	for (position = 0; position < PANELDB_POSITION_MAX; position++) {
		if (string_nocase_strcmp(name, paneldb_position_names[position]) == 0)
			return position;
	}

	return PANELDB_POSITION_UNKNOWN;
}


/**
 * Claim a block for a new panel, fill in the unique key and set
 * default values for the data.
 * 
 * Use with allocate set FALSE is ONLY for use during the initial
 * file load.
 *
 * \param allocate	TRUE to allocate a key; FALSE to set to PANELBD_NULL_KEY.
 * \return		The new block number, or -1 on failure.
 */

static int paneldb_new(osbool allocate)
{
	if (paneldb_panels >= paneldb_allocation && flex_extend((flex_ptr) &paneldb_list,
			(paneldb_allocation + PANELDB_ALLOC_CHUNK) * sizeof(struct paneldb_container)) == 1)
		paneldb_allocation += PANELDB_ALLOC_CHUNK;

	if (paneldb_panels >= paneldb_allocation)
		return -1;

	paneldb_list[paneldb_panels].key = paneldb_key++;
	paneldb_set_defaults(&(paneldb_list[paneldb_panels].entry));

	/* If we're not allocating a key, blank the entry out. We still
	 * claim it, so that the next key number advances. Note that
	 * non-allocation only works if the database has had no deletions.
	 */
 
	if (!allocate) {
		if (paneldb_list[paneldb_panels].key != paneldb_panels)
			return -1;

		paneldb_list[paneldb_panels].key = PANELDB_NULL_KEY;
	}

	paneldb_unsafe = TRUE;

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

	if (flex_midextend((flex_ptr) &paneldb_list, (index + 1) * sizeof(struct paneldb_container),
			-sizeof(struct paneldb_container))) {
		paneldb_allocation--;
		paneldb_panels--;
	}

	paneldb_unsafe = TRUE;
}


/**
 * Copy the contents of a panel block into a second block.
 *
 * \param *to		The block to copy the data to.
 * \param *from		The block to copy the data from.
 */

void paneldb_copy(struct paneldb_entry *to, struct paneldb_entry *from)
{
	to->position = from->position;
	to->width = from->width;
	to->sort = from->sort;
	to->slab_size.x = from->slab_size.x;
	to->slab_size.y = from->slab_size.y;
	to->depth = from->depth;
	string_copy(to->name, from->name, PANELDB_NAME_LENGTH);
}


/**
 * Set some default values for a PanelDB entry.
 *
 * \param *entry	The entry to set.
 */

void paneldb_set_defaults(struct paneldb_entry *entry)
{
	if (entry == NULL)
		return;

	entry->position = PANELDB_POSITION_LEFT;
	entry->width = 100;
	entry->sort = 1;
	entry->slab_size.x = config_int_read("SlabXSize");
	entry->slab_size.y = config_int_read("SlabYSize");
	entry->depth = config_int_read("WindowColumns");
	*(entry->name) = '\0';
}
