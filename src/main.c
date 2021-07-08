#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <err.h>
#include <czmq.h>
#include <unistd.h>
#include <modbus.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/ioctls.h>
#include "config.h"
#include "bustools.h"
#include "time.h"

#define NB_REGS 2

static const int query_delay = 1000;

int main( int argc, char *argv[])
{
	g_autoptr(GKeyFile) map = g_key_file_new();
	zsock_t *zmq_pull;
	int n_errors = 0;
	int cumulative_errors = 0;
	int read_counter = 0;
	int ret;
	uint16_t dest [NB_REGS];

	// Parse command line and config. If anything fails, the
	// program is terminated.
	config_parse_all(map, &argc, &argv);

	// Initialize message queue
	{
		g_autoptr(GError) error = NULL;
		g_autofree gchar *path = g_key_file_get_string(map, "zmq", "socket", &error);
		config_check_key(error);
		zmq_pull = zsock_new_pull(path);
		if (zmq_pull == NULL) err(7,"jooh");
	}
	zpoller_t *zmq_poller = zpoller_new(zmq_pull, NULL);
	if (zmq_poller == NULL) err(7,"juuh");
	
	// Serial port settings.
	modbus_t *ctx = bustools_initialize(map, "serial");

	if (modbus_set_slave(ctx, 2)) {
		err(5, "Unable to set slave");
	}

	struct timespec loop_start = time_get_monotonic();
	int target = 0;
	
	// Ready to communicate.
	for(;;){
		// Be punctual. Try to be at this point every
		// query_delay milliseconds.
		target += query_delay;

		while (true) {
			// Calculate how long we will spend in this
			// ZMQ part. When the time runs out, back to
			// routine tasks.
			struct timespec now = time_get_monotonic();
			int elapsed_ms = time_nanodiff(&now, &loop_start) / 1000000;
			int left = target - elapsed_ms;

			zsock_t *match = (zsock_t *)zpoller_wait(zmq_poller, left < 0 ? 0 : left);

			// If socket closed, just quit.
			if (zpoller_terminated(zmq_poller)) {
				zpoller_destroy(&zmq_poller);
				zsock_destroy(&zmq_pull);
				return 0;
			}

			// When we encounter timeout, fallback to the routine.
			if (match == NULL) break;
			char *string = zstr_recv(match);
			if (string == NULL) {
				printf("PASKAA\n");
				continue;
			}
			puts(string);
			zstr_free (&string);
		}

		// Scheduled
		printf("Back to routine modbus\n");

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
