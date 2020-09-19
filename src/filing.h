/* Copyright 2019, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Launcher:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: filing.h
 */

#ifndef LAUNCHER_FILING
#define LAUNCHER_FILING

/**
 * File load result statuses.
 */

enum filing_status {
	FILING_STATUS_OK,							/**< The operation is OK.							*/
	FILING_STATUS_VERSION,							/**< An unknown file version number has been found.				*/ 
	FILING_STATUS_UNEXPECTED,						/**< The operation has encountered unexpected file contents.			*/
	FILING_STATUS_MEMORY,							/**< The operation has run out of memory.					*/
	FILING_STATUS_BAD_MEMORY,						/**< Something went wrong with the memory allocation.				*/
	FILING_STATUS_CORRUPT							/**< The file contents appeared to be corrupt.					*/
};

/** 
 * A load or save operation instance block.
 */

struct filing_block;


/**
 * Load the contents of a button file into the respective databases.
 *
 * \param *leaf_name	The file leafname to load.
 * \return		TRUE on success; else FALSE.
 */

osbool filing_load(char *leaf_name);


/**
 * Save the contents of the respective databases into a buttons file.
 *
 * \param *leaf_name	The file leafname to save to.
 * \return		TRUE on success; else FALSE.
 */

osbool filing_save(char *leaf_name);


/**
 * Return details of the next section contained in the file.
 *
 * \param *in			The file being loaded.
 * \return			TRUE if the token is in the next section;
 *				FALSE if there are no more sections in the file.
 */

osbool filing_get_next_section(struct filing_block *in);


/**
 * Move the file pointer on to the next token contained in the file.
 * 
 * \param *in			The file being loaded.
 * \return			TRUE if the token is in the current section;
 *				FALSE if there are no more tokens in the section.
 */

osbool filing_get_next_token(struct filing_block *in);


/**
 * Test the name of the current token in a file.
 *
 * \param *in			The filing being loaded.
 * \param *token		Pointer to the token name to test against.
 * \return			TRUE if the token name matches; otherwise FALSE.
 */

osbool filing_test_token(struct filing_block *in, char *token);


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

char *filing_get_section_name(struct filing_block *in, char *buffer, size_t length);


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

char *filing_get_text_value(struct filing_block *in, char *buffer, size_t length);


/**
 * Return the boolean value of a token in a file, which will be in "Yes"
 * or "No" format.
 *
 * \param *in			The file being loaded.
 * \return			The boolean value, or FALSE.
 */

osbool filing_get_opt_value(struct filing_block *in);


/**
 * Return the integer value of a token in a file.
 *
 * \param *in			The file being loaded.
 * \return			The integer value, or 0.
 */

int filing_get_int_value(struct filing_block *in);


/**
 * Return the unsigned integer value of a token in a file.
 *
 * \param *in			The file being loaded.
 * \return			The unsigned integer value, or 0.
 */

unsigned filing_get_unsigned_value(struct filing_block *in);


/**
 * Set the status of a file being loaded, to indicate problems that have
 * been encountered by the client modules.
 *
 * \param *in			The file being loaded.
 * \param status		The new status to set for the file.
 */

void filing_set_status(struct filing_block *in, enum filing_status status);









#endif

