#include <err.h>
#include "config.h"

// Command line option parser
static gchar *conf_file = NULL;
static GOptionEntry entries[] =
{
	{ "conf", 'c', 0, G_OPTION_ARG_FILENAME, &conf_file, "Configuration file", "FILE"},
	{ NULL }
};


void config_check_key(GError *error)
{
	// If ok, don't do anything
	if (error == NULL) return;
	
	if (!g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND))
	{
		errx(4, "Key not found in configuration: %s", error->message);
	}

	if (!g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE))
	{
		errx(4, "Key data type is invalid in configuration: %s", error->message);
	}

	errx(4, "GLib error while reading configuration");
}

void config_parse_all(GKeyFile *map, gint *argc, gchar ***argv)
{
	g_autoptr(GError) error = NULL;
	GOptionContext *context;
	
	context = g_option_context_new("- Reads MODBUS and writes database");
	g_option_context_add_main_entries(context, entries, NULL);
	if (!g_option_context_parse(context, argc, argv, &error))
	{
		errx(1, "Option parsing failed: %s", error->message);
	}
		
	if (conf_file == NULL) {
		errx(2, "Option --conf is mandatory. Try '%s --help'", *argv[0]);
	}

	if (!g_key_file_load_from_file(map, conf_file, G_KEY_FILE_NONE, &error))
	{
		if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
			errx(3, "Error loading key file: %s", error->message);
		}
		err(3, "Unable to read configuration");
	}
}
