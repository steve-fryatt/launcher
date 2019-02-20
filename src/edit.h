/* Copyright 2003-2019, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: edit.h
 */

#ifndef LAUNCHER_EDIT
#define LAUNCHER_EDIT

#include "appdb.h"

/**
 * Initialise the Edit dialogue.
 */

void edit_initialise(void);


/**
 * Terminate the Edit dialogue.
 */

void edit_terminate(void);


/**
 * Open a Button Edit dialogue for a given target.
 *
 * \param *pointer	The pointer location at which to open the dialogue.
 * \param *target	A client-specified target for the dialogue.
 * \param *data		The AppDB data to display in the dialogue.
 */

void edit_open_dialogue(wimp_pointer *pointer, void *target, struct appdb_entry *data);

#endif

