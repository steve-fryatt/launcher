/* Launcher - global.h
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
 */

#ifndef _LAUNCHER_GLOBAL
#define _LAUNCHER_GLOBAL

/* ================================================================================================================== */

#define MESS_BLK_SIZE 256               /* The size of the default MessageTrans block. */

/* ================================================================================================================== */

/* Global structures to hold window handles, menu pointers and other global-type things. */

typedef struct
{
  wimp_w      launch;
  wimp_w      prog_info;
  wimp_w      edit;
}
global_windows;

typedef struct
{
  wimp_menu   *menu_up;

  wimp_menu   *main;
}
global_menus;


#endif
