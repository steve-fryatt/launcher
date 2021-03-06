/* Copyright 2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: objutil.h
 */

#ifndef LAUNCHER_OBJUTIL
#define LAUNCHER_OBJUTIL


/**
 * Given an object referenced by a filename, find an appropriate sprite
 * from the Wimp Sprite Pool and return its name in the supplied buffer.
 * 
 * \param *object	The filename of the object to process.
 * \param *sprite	Pointer to a buffer to hold the sprite name.
 * \param length	The length of the supplied buffer.
 * \param *bootable	Pointer to a variable to return the bootable state
 * \return		TRUE if successful; FALSE on failure.
 */

osbool objutil_find_sprite(char *object, char *sprite, size_t length, osbool *bootable);


/**
 * Test a sprite to see if it is in the Wimp Sprite Pool.
 *
 * \param *sprite	The name of the sprite to test.
 * \return		TRUE if the sprite exists; else FALSE.
 */

osbool objutil_test_sprite(char *sprite);


/**
 * Launch an object referenced by a supplied filename.
 * 
 * \param *object	The filename of the object to launch.
 * \return		TRUE if successful; FALSE on error.
 */ 

osbool objutil_launch(char *object);

#endif

