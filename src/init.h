/* Launcher - init.h
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
 */

#ifndef _LAUNCHER_INIT
#define _LAUNCHER_INIT

/* ================================================================================================================== */

#define IND_DATA_SIZE 4095
#define RES_PATH_LEN 255

/* ================================================================================================================== */

/* Initialisation functions. */

int initialise (void);
void load_templates (char *template_file, global_windows *windows);

#endif
