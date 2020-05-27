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
 * \file: objutil.h
 */

#ifndef LAUNCHER_OBJUTIL
#define LAUNCHER_OBJUTIL

/**
 * Initialise the Object Utils.
 */

void objutil_initialise(void);


/**
 * Terminate the Object Utils.
 */

void objutil_terminate(void);


/**
 * Launch an object referenced by a supplied filename.
 * 
 * \param *file		The filename of the object to launch.
 * \return		TRUE if successful; FALSE on error.
 */ 

osbool objutil_launch(char *file);

#endif

