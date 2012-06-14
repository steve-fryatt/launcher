/* Launcher - buttons.h
 * (c) Stephen Fryatt, 2003
 *
 * Desktop application launcher.
 */

#ifndef LAUNCHER_LAUNCHER
#define LAUNCHER_LAUNCHER

/* ================================================================================================================== */

#define LAUNCHER_TOP    1
#define LAUNCHER_BOTTOM 2
#define LAUNCHER_LEFT   3
#define LAUNCHER_RIGHT  4

#define WINDOW_YPOS   140
#define WINDOW_HEIGHT 1856

#define BUTTON_GUTTER 4

/* ================================================================================================================== */

/* Structure to hold details of a button. */

typedef struct button
{
  char            name[64];               /* Button name */
  int             x, y;                   /* X and Y positions of the button in the window */
  char            sprite[20];             /* The sprite name */
  int             local_copy;             /* Do we keep a local copy of the sprite? */
  char            command[1024];          /* The command to be executed */
  int             filer_boot;             /* Should the item be Filer_Booted on startup? */

  wimp_i          icon;                   /* The icon number allocated to the button */
  char            validation[40];         /* The validation string for the icon */

  struct button   *next;
}
button;

/* ================================================================================================================== */

/* Launch functions. */

void initialise_buttons_window (wimp_w window, wimp_icon sample_icon, wimp_icon edge_icon);
int load_buttons_file (char *leaf_name);
int save_buttons_file (char *leaf_name);
void boot_buttons (void);
int create_button_icon (button *button_def);

void toggle_launch_window (void);
void open_launch_window (int columns, wimp_w window_level);

int fill_edit_button_window (wimp_i icon);
int open_edit_button_window (wimp_pointer *pointer);
int redraw_edit_button_window (void);
int read_edit_button_window (button *button_def);
int close_edit_button_window (void);

int press_button (wimp_i icon);

#endif
