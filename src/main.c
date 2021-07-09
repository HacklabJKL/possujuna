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
#include <regex.h>
#include "config.h"
#include "modbus_state.h"
#include "zmq_state.h"
#include "time.h"

#define NB_REGS 2

static bool handle_query(GKeyFile *conf, zmq_state_t *state, modbus_state_t *modbus_state, int left);
static bool handle_periodic(GKeyFile *conf, modbus_state_t *state);

static const int query_delay = 1000;

int main(int argc, char *argv[])
{
	g_autoptr(GKeyFile) conf = g_key_file_new();

	// Parse command line and config. If anything fails, the
	// program is terminated.
	config_parse_all(conf, &argc, &argv);

	// Initialize message queue
	g_auto(zmq_state_t) zmq_state = zmq_state_init(conf);
	
	// Initialize MODBUS handler.
	g_auto(modbus_state_t) modbus_state = modbus_state_init(conf);

	struct timespec loop_start = time_get_monotonic();
	int target = 0;

	// Ready to communicate.
	while (true) {
		while (true) {
			// Calculate how long we need to spend here
			// before proceeding to routine tasks.
			struct timespec now = time_get_monotonic();
			int elapsed_ms = time_nanodiff(&now, &loop_start) / 1000000;
			int left = target - elapsed_ms;
			if (left < 0) break;

			// Call ZMQ part with the timeout value.
			if (handle_query(conf, &zmq_state, &modbus_state, left) == false) {
				return 10;
			}
		}

		// Be punctual. Try to be here every query_delay
		// milliseconds.
		target += query_delay;

		printf("Periodic run\n");
		if ( handle_periodic(conf, &modbus_state) == false) {
			return 10;
		}
	}
}

static bool handle_query(GKeyFile *conf, zmq_state_t *state, modbus_state_t *modbus_state, int left)
{
	zsock_t *match = (zsock_t *)zpoller_wait(state->poller, left);

	// If socket closed, just quit.
	if (zpoller_terminated(state->poller)) {
		return false;
	}

	// Stop when timeout.
	if (match == NULL) {
		return true;;
	}
	char *msg = zstr_recv(match);
	if (msg == NULL) {
		// Unexpected input
		return false;
	}
	regmatch_t pmatch[3];
	if (regexec(&state->re_relay, msg, 3, pmatch, 0)) {
		puts("No match");
	} else {
		// Match. Check which relay
		int relay = msg[pmatch[1].rm_so] - '0';
		// Quick hack to get ON/OFF. ON has length of 2
		bool mode = pmatch[2].rm_eo == pmatch[2].rm_so+2;

		// Ok, we need to control. Dig out the configuration.
		g_autoptr(GError) error = NULL;
		gint busid = g_key_file_get_integer(conf, "relay", "busid", &error);
		config_check_key(error);
		gint baud = g_key_file_get_integer(conf, "relay", "baud", &error);
		config_check_key(error);
		
		printf("Releohjaus. Rele %d -> %d\n", relay, mode);
		modbus_state_ensure_baud_rate(modbus_state, baud);

		if (modbus_set_slave(modbus_state->ctx, busid)) {
			err(5, "Unable to set slave");
		}

		// Write to relay
	}

	zstr_free (&msg);
	return true;
}

static bool handle_periodic(GKeyFile *conf, modbus_state_t *state)
{
	uint16_t dest[NB_REGS];
	int ret;

	// Tracer settings.
	g_autoptr(GError) error = NULL;
	gint busid = g_key_file_get_integer(conf, "tracer", "busid", &error);
	config_check_key(error);
	gint baud = g_key_file_get_integer(conf, "tracer", "baud", &error);
	config_check_key(error);

	modbus_state_ensure_baud_rate(state, baud);

	if (modbus_set_slave(state->ctx, busid)) {
		err(5, "Unable to set slave");
	}

	ret = modbus_read_input_registers(state->ctx, 0x3104, NB_REGS, dest);
	if (ret < 0){
		state->n_errors++;
		state->cumulative_errors++;
		if (state->n_errors >= 3){
			perror("modbus_read_regs error\n");
			return false;
		}
	} else {
		state->n_errors = 0;
	}
	state->read_counter++;
	printf("Read counter = %d\n", state->read_counter);
	printf("Error counter = %d\n", state->n_errors);
	printf("Cumulative Error counter = %d\n", state->cumulative_errors);
	printf("Battery Voltage = %d\n", dest[0]);
	printf("Battery Charging current = %d\n", dest[1]);
	return true;
}
