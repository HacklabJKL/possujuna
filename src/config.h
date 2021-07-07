#include <glib.h>

// Terminate program if GLib error detected.
void config_check_key(GError *error);

// Parse command line parameters and populate configuration map based
// on it. If anything fails, the whole program is terminated.
void config_parse_all(GKeyFile *map, gint *argc, gchar ***argv);
