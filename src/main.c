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

typedef struct {
	zpoller_t *poller;
	zsock_t *pull;
} zmq_state_t;

typedef struct {
	modbus_t *ctx;
	int n_errors;
	int cumulative_errors;
	int read_counter;
} modbus_state_t;

static bool do_zmq_magic(zmq_state_t *state, int left);
static bool do_modbus_magic(modbus_state_t *state);
static void zmq_state_free(zmq_state_t *state);
static void modbus_state_free(modbus_state_t *state);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(zmq_state_t, zmq_state_free);
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(modbus_state_t, modbus_state_free);

static const int query_delay = 1000;

int main( int argc, char *argv[])
{
	g_autoptr(GKeyFile) map = g_key_file_new();
	g_auto(zmq_state_t) zmq_state;

	// Parse command line and config. If anything fails, the
	// program is terminated.
	config_parse_all(map, &argc, &argv);

	// Initialize message queue
	{
		g_autoptr(GError) error = NULL;
		g_autofree gchar *path = g_key_file_get_string(map, "zmq", "socket", &error);
		config_check_key(error);
		zmq_state.pull = zsock_new_pull(path);
		if (zmq_state.pull == NULL) err(7,"jooh");

		// Create poller from given puller
		zmq_state.poller = zpoller_new(zmq_state.pull, NULL);
		if (zmq_state.poller == NULL) err(7,"juuh");
	}
	
	// Serial port settings.
	g_auto(modbus_state_t) modbus_state = {NULL, 0, 0, 0};
	modbus_state.ctx = bustools_initialize(map, "serial");

	if (modbus_set_slave(modbus_state.ctx, 2)) {
		err(5, "Unable to set slave");
	}

	struct timespec loop_start = time_get_monotonic();
	int target = 0;
	
	// Ready to communicate.
	while (true) {
		// Be punctual. Try to be at do_modbus_magic() every
		// query_delay milliseconds.
		target += query_delay;

		while (true) {
			// Calculate how long we need to spend here
			// before proceeding to routine tasks.
			struct timespec now = time_get_monotonic();
			int elapsed_ms = time_nanodiff(&now, &loop_start) / 1000000;
			int left = target - elapsed_ms;
			if (left < 0) break;

			// Call ZMQ part with the timeout value.
			if (do_zmq_magic(&zmq_state, left) == false) {
				return 10;
			}
		}

		// Scheduled
		printf("Back to routine modbus\n");

		if ( do_modbus_magic(&modbus_state) == false) {
			return 10;
		}
	}
}

static bool do_zmq_magic(zmq_state_t *state, int left)
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
	char *string = zstr_recv(match);
	if (string == NULL) {
		// Unexpected input
		return false;
	}
	puts(string);
	zstr_free (&string);
	return true;
}

static void zmq_state_free(zmq_state_t *state)
{
	zpoller_destroy(&state->poller);
	zsock_destroy(&state->pull);
}

static bool do_modbus_magic(modbus_state_t *state)
{
	uint16_t dest[NB_REGS];
	int ret;

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

static void modbus_state_free(modbus_state_t *state)
{
	modbus_close(state->ctx);
	modbus_free(state->ctx);
}
