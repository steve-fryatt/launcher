/* Copyright 2003-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: buttons.h
 */

#ifndef LAUNCHER_BUTTONS
#define LAUNCHER_BUTTONS


/**
 * Initialise the buttons window.
 */

void buttons_initialise(void);


/**
 * Terminate the buttons window.
 */

void buttons_terminate(void);


/**
 * Create a full set of buttons from the contents of the application database.
 */

void buttons_create_from_db(void);


/**
 * Inform tha buttons module that the glocal system choices have changed,
 * and force an update of any relevant parameters.
 */

void buttons_refresh_choices(void);

#endif

