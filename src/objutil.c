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


/* ================================================================================================================== */



/**
 * Initialise the Object Utils.
 */

void objutil_initialise(void)
{
	/* Nothing to do! */
}


/**
 * Terminate the Object Utils.
 */

void objutil_terminate(void)
{
	/* Nothing to do! */
}


/**
 * Launch an object referenced by a supplied filename.
 * 
 * \param *file		The filename of the object to launch.
 * \return		TRUE if successful; FALSE on error.
 */ 

osbool objutil_launch(char *file)
{
	char			*buffer;
	int			length;
	fileswitch_object_type	object_type;
	bits			file_type;
	os_error		*error;
	osbool			command_ready = FALSE;

	if (file == NULL)
		return FALSE;

	/* Identify what we're working with. */

	error = xosfile_read_stamped_no_path(file, &object_type, NULL, NULL, NULL, NULL, &file_type);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
		return FALSE;
	}

	if (object_type == fileswitch_NOT_FOUND) {
		error_msgs_report_error("ObjectMissing");
		return FALSE;
	}

	/* Allocate a buffer for the command. */

	length = strlen(file) + OBJUTIL_COMMAND_LEN;

	buffer = heap_alloc(length);

	if (buffer == NULL) {
		error_msgs_report_error("NoMemLaunch");
		return FALSE;
	}

	/* Create a command. */

	switch (object_type) {
	case fileswitch_IS_FILE:
	case fileswitch_IS_IMAGE:
		string_printf(buffer, length, "%%Filer_Run %s", file);
		command_ready = TRUE;
		break;

	case fileswitch_IS_DIR:
		switch (file_type) {
		case osfile_TYPE_DIR:
			string_printf(buffer, length, "%%Filer_OpenDir %s", file);
			command_ready = TRUE;
			break;

		case osfile_TYPE_APPLICATION:
			string_printf(buffer, length, "%%StartDesktopTask %s", file);
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