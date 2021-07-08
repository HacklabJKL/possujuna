#include <err.h>
#include "config.h"
#include "modbus_state.h"

modbus_state_t modbus_state_init(GKeyFile *map)
{
	const char *group = "serial";
	modbus_state_t state = {NULL, 0, 0, 0, 0};

	// Get serial parameters from the configuration file
	g_autoptr(GError) error = NULL;
	g_autofree gchar *uart_path = g_key_file_get_string(map, group, "port", &error);
	config_check_key(error);
	state.current_baud = g_key_file_get_integer(map, group, "baud", &error);
	config_check_key(error);
	gboolean rs485 = g_key_file_get_boolean(map, group, "rs485", &error);
	config_check_key(error);

	// Prepare and connect MODBUS
	state.ctx = modbus_new_rtu(uart_path, state.current_baud, 'N', 8, 1);
	if (state.ctx == NULL) {
		err(5, "Unable to create the libmodbus context");
	}

	if (modbus_connect(state.ctx)) {
		err(5, "Unable to open serial port");
	}

	if (rs485) {
		if (modbus_rtu_set_serial_mode(state.ctx, MODBUS_RTU_RS485)) {
			err(5, "Unable to set RS485 mode");
		}
		if (modbus_rtu_set_rts(state.ctx, MODBUS_RTU_RTS_DOWN)) {
			err(5, "Unable to set RS485 RTS setting");
		}
	}

	return state;
}

void modbus_state_free(modbus_state_t *state)
{
	modbus_close(state->ctx);
	modbus_free(state->ctx);
}
