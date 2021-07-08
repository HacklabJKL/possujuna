#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include "config.h"
#include "modbus_state.h"
#include "baudhack.h"

modbus_state_t modbus_state_init(GKeyFile *map)
{
	const char *group = "serial";
	modbus_state_t state = {NULL, 0, 0, 0, 0, 0};

	// Get serial parameters from the configuration file
	g_autoptr(GError) error = NULL;
	g_autofree gchar *uart_path = g_key_file_get_string(map, group, "port", &error);
	config_check_key(error);
	state.current_baud = 9600; // Set later
	gboolean rs485 = g_key_file_get_boolean(map, group, "rs485", &error);
	config_check_key(error);

	// Tracer settings. Move to somewhere out here
	state.tracer_busid = g_key_file_get_integer(map, "tracer", "busid", &error);
	config_check_key(error);
	state.tracer_baud = g_key_file_get_integer(map, "tracer", "baud", &error);
	config_check_key(error);

	// Open file descriptor just for baud rate hacking
	state.baud_fd = open(uart_path, O_RDONLY);
	if (state.baud_fd == -1) {
		err(5, "Unable to open serial port");
	}
	
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

void modbus_state_ensure_baud_rate(modbus_state_t *state, int baud)
{
	if (state->current_baud == baud) {
		// Baud rate is perfect
		return;
	}
	
	if (baudhack(state->baud_fd, baud)) {
		err(5, "Unable to hack baud rate");
	}
	state->current_baud = baud;
}	
