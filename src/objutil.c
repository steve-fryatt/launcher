/* Copyright 2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: objutil.c
 */

/* ANSI C header files. */

#include <string.h>
#include <stdio.h>

/* OSLib header files. */

#include "oslib/fileswitch.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/wimpspriteop.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files. */

#include "objutil.h"

/**
 * Space allocated for FS commands.
 */

#define OBJUTIL_COMMAND_LEN 30

/* Static Function Prototypes. */

static osbool objutil_test_sprite(char *sprite);


/**
 * Given an object referenced by a filename, find an appropriate sprite
 * from the Wimp Sprite Pool and return its name in the supplied buffer.
 * 
 * \param *object	The filename of the object to process.
 * \param *sprite	Pointer to a buffer to hold the sprite name.
 * \param length	The length of the supplied buffer.
 * \return		TRUE if successful; FALSE on failure.
 */

osbool objutil_find_sprite(char *object, char *sprite, size_t length)
{
	fileswitch_object_type	object_type;
	bits			load_addr, exec_addr, file_type;
	os_error		*error;
	char			*leafname;

	if (sprite == NULL || length == 0)
		return FALSE;

	*sprite = '\0';

	if (object == NULL)
		return FALSE;

	/* Identify what we're working with. */

	leafname = string_find_leafname(object);

	error = xosfile_read_stamped_no_path(object, &object_type, &load_addr, &exec_addr, NULL, NULL, &file_type);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
		return FALSE;
	}

	if (object_type == fileswitch_NOT_FOUND) {
		error_msgs_report_error("ObjectMissing");
		return FALSE;
	}

	switch (object_type) {
	case fileswitch_IS_FILE:
	case fileswitch_IS_IMAGE:
		if (file_type == osfile_TYPE_UNTYPED) {
			string_copy(sprite, "file_lxa", length);
		} else {
			string_printf(sprite, length, "file_%3x", file_type);
			if (!objutil_test_sprite(sprite))
				string_copy(sprite, "file_xxx", length);
		}
		break;

	case fileswitch_IS_DIR:
		switch (file_type) {
		case osfile_TYPE_DIR:
			string_copy(sprite, "directory", length);
			break;

		case osfile_TYPE_APPLICATION:
			string_copy(sprite, leafname, length);
			string_tolower(sprite);
			if (!objutil_test_sprite(sprite))
				string_copy(sprite, "application", length);
			break;
		}
		break;
	}

	return TRUE;
}

/**
 * Test a sprite to see if it is in the Wimp Sprite Pool.
 *
 * \param *sprite	The name of the sprite to test.
 * \return		TRUE if the sprite exists; else FALSE.
 */

static osbool objutil_test_sprite(char *sprite)
{
	if (sprite == NULL)
		return FALSE;

	return xwimpspriteop_read_sprite_info(sprite, NULL, NULL, NULL, NULL) == NULL;
}

/**
 * Launch an object referenced by a supplied filename.
 * 
 * \param *object	The filename of the object to launch.
 * \return		TRUE if successful; FALSE on error.
 */ 

osbool objutil_launch(char *object)
{
	char			*buffer;
	int			length;
	fileswitch_object_type	object_type;
	bits			file_type;
	os_error		*error;
	osbool			command_ready = FALSE;

	if (object == NULL)
		return FALSE;

	/* Identify what we're working with. */

	error = xosfile_read_stamped_no_path(object, &object_type, NULL, NULL, NULL, NULL, &file_type);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
		return FALSE;
	}

	if (object_type == fileswitch_NOT_FOUND) {
		error_msgs_report_error("ObjectMissing");
		return FALSE;
	}

	/* Allocate a buffer for the command. */

	length = strlen(object) + OBJUTIL_COMMAND_LEN;

	buffer = heap_alloc(length);

	if (buffer == NULL) {
		error_msgs_report_error("NoMemLaunch");
		return FALSE;
	}

	/* Create a command. */

	switch (object_type) {
	case fileswitch_IS_FILE:
	case fileswitch_IS_IMAGE:
		string_printf(buffer, length, "%%Filer_Run %s", object);
		command_ready = TRUE;
		break;

	case fileswitch_IS_DIR:
		switch (file_type) {
		case osfile_TYPE_DIR:
			string_printf(buffer, length, "%%Filer_OpenDir %s", object);
			command_ready = TRUE;
			break;

		case osfile_TYPE_APPLICATION:
			string_printf(buffer, length, "%%StartDesktopTask %s", object);
			command_ready = TRUE;
			break;
		}
		break;
	}

	/* Launch the command, if ready. */

	if (command_ready == TRUE) {
		error = xos_cli(buffer);
		if (error != NULL) {
			error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
			command_ready = FALSE;
		}
	} else {
		error_msgs_report_error("ObjectBadType");
	}

	/* Free the buffer and return. */

	heap_free(buffer);

	return command_ready;
}