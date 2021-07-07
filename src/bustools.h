#include <glib.h>

// Initialize libmodbus context based on the parameters in the
// configuration file. Terminates if connection fails.
modbus_t *bustools_initialize(GKeyFile *map);
