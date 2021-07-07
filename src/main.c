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
#include "bustools.h"

#define NB_REGS 2

int main( int argc, char *argv[])
{
	g_autoptr(GKeyFile) map = g_key_file_new();
	int n_errors = 0;
	int cumulative_errors = 0;
	int read_counter = 0;
	int ret;
	uint16_t dest [NB_REGS];

	// Parse command line and config. If anything fails, the
	// program is terminated.
	config_parse_all(map, &argc, &argv);

	// Serial port settings.
	modbus_t *ctx = bustools_initialize(map, "serial");

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
