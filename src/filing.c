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
 * \file: filing.c
 */

/* ANSI C header files. */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files. */

#include "oslib/hourglass.h"
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

#include "filing.h"

#include "appdb.h"
#include "paneldb.h"

/**
 * The maximum length of a filename.
 */

#define FILING_MAX_FILENAME_LENGTH 1024

/**
 * The current Launcher file format version.
 */

#define FILING_CURRENT_FORMAT 200

/**
 * The format at which the new data structure comes into play.
 */

#define FILING_NEW_DATA_FORMAT 200

/**
 * The file load and save handle structure.
 */

struct filing_block {
	FILE			*handle;
	char			section[FILING_MAX_FILE_LINE_LEN];
	char			token[FILING_MAX_FILE_LINE_LEN];
	char			value[FILING_MAX_FILE_LINE_LEN];
	int			format;
	enum config_read_status	result;
	enum filing_status	status;
};


/**
 * Test for file load statuses which are considered OK for continuing.
 */

#define filing_load_status_is_ok(status) (((status) == FILING_STATUS_OK) || ((status) == FILING_STATUS_UNEXPECTED))


/**
 * Load the contents of a button file into the respective databases.
 *
 * \param *leaf_name	The file leafname to load.
 * \return		TRUE on success; else FALSE.
 */

osbool filing_load(char *leaf_name)
{
	char			filename[FILING_MAX_FILENAME_LENGTH];
	struct filing_block	in;
	int			default_panel_index = -1;

	/* Find a buttons file somewhere in the usual config locations. */

	config_find_load_file(filename, FILING_MAX_FILENAME_LENGTH, leaf_name);

	if (*filename == '\0')
		return FALSE;

	in.handle = fopen(filename, "r");

	if (in.handle == NULL)
		return FALSE;

	hourglass_on();

	appdb_reset();
	paneldb_reset();

	*in.section = '\0';
	*in.token = '\0';
	*in.value = '\0';

	in.format = 0;
	in.status = FILING_STATUS_OK;
	in.result = sf_CONFIG_READ_EOF;

	do {
		if ((string_nocase_strcmp(in.section, "Panels") == 0) && (in.format >= FILING_NEW_DATA_FORMAT)) {
			paneldb_load_new_file(&in);
		} else if ((string_nocase_strcmp(in.section, "Buttons") == 0) && (in.format >= FILING_NEW_DATA_FORMAT)) {
			appdb_load_new_file(&in);
		} else if ((*in.section != '\0') && (in.format < FILING_NEW_DATA_FORMAT)) {
			default_panel_index = paneldb_create_old_panel();
			appdb_load_old_file(&in, default_panel_index);
		} else {
			do {
				if (*in.section != '\0')
					in.status = FILING_STATUS_UNEXPECTED;

				/* Load in the file format, converting an n.nn number into an
				 * integer value (eg. 1.00 would become 100).  Supports 0.00 to 9.99.
				 */

				if (string_nocase_strcmp(in.token, "Format") == 0) {
					if (strlen(in.value) == 4 && isdigit(in.value[0]) && isdigit(in.value[2]) && isdigit(in.value[3]) && in.value[1] == '.') {
						in.value[1] = in.value[2];
						in.value[2] = in.value[3];
						in.value[3] = '\0';

						in.format = atoi(in.value);

						if (in.format > FILING_CURRENT_FORMAT)
							in.status = FILING_STATUS_VERSION;
					} else {
						in.status = FILING_STATUS_UNEXPECTED;
					}
				}
			} while (filing_get_next_token(&in));
		}
	} while (filing_load_status_is_ok(in.status) && in.result != sf_CONFIG_READ_EOF);

	fclose(in.handle);

	/* Check and link up the bar names. */

	if (!appdb_complete_file_load())
		in.status = FILING_STATUS_CORRUPT;

	/* If the file format wasn't understood, get out now. */

	if (!filing_load_status_is_ok(in.status)) {
		hourglass_off();
		switch (in.status) {
		case FILING_STATUS_VERSION:
			error_msgs_report_error("UnknownFileFormat");
			break;
		case FILING_STATUS_MEMORY:
			error_msgs_report_error("NoMemLoadFile");
			break;
		case FILING_STATUS_BAD_MEMORY:
			error_msgs_report_error("BadMemory");
			break;
		case FILING_STATUS_CORRUPT:
			error_msgs_report_error("CorruptFile");
			break;
		case FILING_STATUS_OK:
		case FILING_STATUS_UNEXPECTED:
			break;
		}
		return FALSE;
	}

	hourglass_off();

	if (in.status == FILING_STATUS_UNEXPECTED)
		error_msgs_report_info("UnknownFileData");

	return TRUE;
}


/**
 * Save the contents of the respective databases into a buttons file.
 *
 * \param *leaf_name	The file leafname to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool filing_save(char *leaf_name)
{
	char	filename[FILING_MAX_FILENAME_LENGTH];
	FILE	*file;


	/* Find a buttons file to write somewhere in the usual config locations. */

	config_find_save_file(filename, FILING_MAX_FILENAME_LENGTH, leaf_name);

	if (*filename == '\0')
		return FALSE;

	/* Open the file and work through it using the config file handling library. */

	file = fopen(filename, "w");

	if (file == NULL)
		return FALSE;

	fprintf(file, "# >Buttons\n#\n# Saved by Launcher.\n");

	fprintf(file, "\nFormat: 2.00\n");

	paneldb_save_file(file);
	appdb_save_file(file);

	fclose(file);

	return TRUE;

}

/**
 * Return details of the next section contained in the file.
 *
 * \param *in			The file being loaded.
 * \return			TRUE if the token is in the next section;
 *				FALSE if there are no more sections in the file.
 */

osbool filing_get_next_section(struct filing_block *in)
{
	if (in == NULL || !filing_load_status_is_ok(in->status))
		return FALSE;

	return ((in->result == sf_CONFIG_READ_NEW_SECTION) &&
			(in->section != NULL) && (*in->section != '\0')) ? TRUE : FALSE;
}


/**
 * Move the file pointer on to the next token contained in the file.
 * 
 * \param *in			The file being loaded.
 * \return			TRUE if the token is in the current section;
 *				FALSE if there are no more tokens in the section.
 */

osbool filing_get_next_token(struct filing_block *in)
{
	if (in == NULL || !filing_load_status_is_ok(in->status))
		return FALSE;

	in->result = config_read_token_pair(in->handle, in->token, in->value, in->section);

//#ifdef DEBUG
	debug_printf("Read line: section=%s, token=%s, value=%s, result=%d", in->section, in->token, in->value, in->result);
//#endif

	return (in->result != sf_CONFIG_READ_EOF && in->result != sf_CONFIG_READ_NEW_SECTION) ? TRUE : FALSE;
}


/**
 * Test the name of the current token in a file.
 *
 * \param *in			The filing being loaded.
 * \param *token		Pointer to the token name to test against.
 * \return			TRUE if the token name matches; otherwise FALSE.
 */

osbool filing_test_token(struct filing_block *in, char *token)
{
	if (in == NULL)
		return FALSE;

	return (string_nocase_strcmp(in->token, token) == 0) ? TRUE : FALSE;
}


/**
 * Get the name of a section in a file, either returning a pointer to
 * the volatile data in memory or copying it into a supplied buffer. If the
 * name is too long to fit in the buffer, the file is reported as
 * being corrupt.
 *
 * \param *in			The file being loaded.
 * \param *buffer		Pointer to a buffer to take the string, or
 *				NULL to return a pointer to the string in
 *				volatile memory.
 * \param length		The length of the supplied buffer, or 0.
 * \return			Pointer to the value string, either in the
 *				supplied buffer or in volatile memory.
 */

char *filing_get_section_name(struct filing_block *in, char *buffer, size_t length)
{
	if (in == NULL)
		return NULL;

#ifdef DEBUG
	debug_printf("Return section name: %s", in->value);
#endif

	if (buffer == NULL)
		return in->section;

	if (length == 0) {
		in->status = FILING_STATUS_BAD_MEMORY;
		return NULL;
	}

	buffer[length - 1] = '\0';
	strncpy(buffer, in->section, length);

	if (buffer[length - 1] != '\0') {
		in->status = FILING_STATUS_CORRUPT;
		buffer[length - 1] = '\0';
#ifdef DEBUG
		debug_printf("Field is too long: original=%s, copied=%s", in->section, buffer);
#endif
	}

	return buffer;
}


/**
 * Get the textual value of a token in a file, either returning a pointer to
 * the volatile data in memory or copying it into a supplied buffer. If the
 * token value is too long to fit in the buffer, the file is reported as
 * being corrupt.
 *
 * \param *in			The file being loaded.
 * \param *buffer		Pointer to a buffer to take the string, or
 *				NULL to return a pointer to the string in
 *				volatile memory.
 * \param length		The length of the supplied buffer, or 0.
 * \return			Pointer to the value string, either in the
 *				supplied buffer or in volatile memory.
 */

char *filing_get_text_value(struct filing_block *in, char *buffer, size_t length)
{
	if (in == NULL)
		return NULL;

#ifdef DEBUG
	debug_printf("Return text value: %s", in->value);
#endif

	if (buffer == NULL)
		return in->value;

	if (length == 0) {
		in->status = FILING_STATUS_BAD_MEMORY;
		return NULL;
	}

	buffer[length - 1] = '\0';
	strncpy(buffer, in->value, length);

	if (buffer[length - 1] != '\0') {
		in->status = FILING_STATUS_CORRUPT;
		buffer[length - 1] = '\0';
#ifdef DEBUG
		debug_printf("Field is too long: original=%s, copied=%s", in->value, buffer);
#endif
	}

	return buffer;
}


/**
 * Return the boolean value of a token in a file, which will be in "Yes"
 * or "No" format.
 *
 * \param *in			The file being loaded.
 * \return			The boolean value, or FALSE.
 */

osbool filing_get_opt_value(struct filing_block *in)
{
	if (in == NULL || in->value == NULL)
		return FALSE;

	return config_read_opt_string(in->value);
}


/**
 * Return the integer value of a token in a file.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or 0.
 */

int filing_get_int_value(struct filing_block *in)
{
	if (in == NULL || in->value == NULL)
		return 0;

	return atoi(in->value);
}


/**
 * Return the unsigned integer value of a token in a file.
 *
 * \param *in			The file being loaded.
 * \return			The unsigned integer value, or 0.
 */

unsigned filing_get_unsigned_value(struct filing_block *in)
{
	if (in == NULL || in->value == NULL)
		return 0;

	return (unsigned) atoi(in->value);
}


/**
 * Set the status of a file being loaded, to indicate problems that have
 * been encountered by the client modules.
 *
 * \param *in			The file being loaded.
 * \param status		The new status to set for the file.
 */

void filing_set_status(struct filing_block *in, enum filing_status status)
{
	if (in == NULL)
		return;

	in->status = status;
}

