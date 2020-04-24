/* Copyright 2002-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: main.h
 */

#ifndef LAUNCHER_MAIN
#define LAUNCHER_MAIN


/**
 * Application-wide global variables.
 */

extern wimp_t			main_task_handle;
extern osbool			main_quit_flag;

/**
 * Main code entry point.
 */

int main (void);

/**
 * Check for unsaved data in the Panel and App databases, and ask the user if
 * they wish to discard the changes.
 * 
 * \return			TRUE if the user wishes to save the data.
 */

osbool main_check_for_unsaved_data(void);

#endif

