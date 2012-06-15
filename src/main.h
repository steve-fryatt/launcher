/* Launcher - main.h
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
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

#endif

