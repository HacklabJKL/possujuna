#include <glib.h>
#include <modbus.h>
#include <err.h>
#include "config.h"

modbus_t *bustools_initialize(GKeyFile *map)
{
	modbus_t *ctx;

	// Get serial parameters from the configuration file
	g_autoptr(GError) error = NULL;
	g_autofree gchar *uart_path = g_key_file_get_string(map, "serial", "port", &error);
	config_check_key(error);
	gint uart_baud = g_key_file_get_integer(map, "serial", "baud", &error);
	config_check_key(error);
	gboolean rs485 = g_key_file_get_boolean(map, "serial", "rs485", &error);
	config_check_key(error);

	// Prepare and connect MODBUS
	ctx = modbus_new_rtu(uart_path, uart_baud, 'N', 8, 1);
	if (ctx == NULL) {
		err(5, "Unable to create the libmodbus context");
	}

	if (modbus_connect(ctx)){
		err(5, "Unable to open serial port");
	}

	if (rs485) {
		if (modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485)) {
			err(5, "Unable to set RS485 mode");
		}
		if (modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_DOWN)) {
			err(5, "Unable to set RS485 RTS setting");
		}
	}

	return ctx;
}
