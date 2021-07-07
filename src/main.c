#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <err.h>
#include <unistd.h>
#include <modbus.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/ioctls.h>
#include "config.h"

#define NB_REGS 2

int main( int argc, char *argv[])
{
	g_autoptr(GKeyFile) map = g_key_file_new();
	int n_errors = 0;
	int cumulative_errors = 0;
	int read_counter = 0;
	int ret;
	modbus_t *ctx;
	uint16_t dest [NB_REGS];

	// Parse command line and config. If anything fails, the
	// program is terminated.
	config_parse_all(map, &argc, &argv);

	// Serial port settings.
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

	if (modbus_set_slave(ctx, 2)) {
		err(5, "Unable to set slave");
	}

	// Ready to communicate.
	for(;;){
		ret = modbus_read_input_registers(ctx, 0x3104, NB_REGS, dest);
		if(ret < 0){
			n_errors++;
			cumulative_errors++;
			if(n_errors >= 3){
				perror("modbus_read_regs error\n");
				return -1;
			}
		}else{
			n_errors = 0;
		}
		read_counter++;
		printf("Read counter = %d\n",read_counter);
		printf("Error counter = %d\n",n_errors);
		printf("Cumulative Error counter = %d\n",cumulative_errors);
		printf("Battery Voltage = %d\n",dest[0]);
		printf("Battery Charging current = %d\n",dest[1]);
		// sleep(1);
	}

	modbus_close(ctx);
	modbus_free(ctx);

}
