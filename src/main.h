/* Launcher - main.h
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
 */

#ifndef _LAUNCHER_MAIN
#define _LAUNCHER_MAIN

/* ================================================================================================================== */

/* Function prototypes. */

int main (void);
int poll_loop (void);

void mouse_click_handler (wimp_pointer *);
void menu_selection_handler (wimp_selection *);
void user_message_handler (wimp_message *);
void bounced_message_handler (wimp_message *message);

#endif
