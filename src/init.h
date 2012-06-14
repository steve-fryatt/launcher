/* Launcher - init.h
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
 */

#ifndef LAUNCHER_INIT
#define LAUNCHER_INIT

/* ================================================================================================================== */

#define IND_DATA_SIZE 4095
#define RES_PATH_LEN 255

/* ================================================================================================================== */

/* Initialisation functions. */

void load_templates (char *template_file, global_windows *windows);

#endif
