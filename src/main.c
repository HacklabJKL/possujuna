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
#include "time.h"

#define NB_REGS 2

typedef struct {
	zpoller_t *poller;
	zsock_t *pull;
	regex_t re_relay;
} zmq_state_t;

static zmq_state_t zmq_state_init(GKeyFile *map);
static bool do_zmq_magic(zmq_state_t *state, modbus_state_t *modbus_state, int left);
static bool do_modbus_magic(modbus_state_t *state);
static void zmq_state_free(zmq_state_t *state);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(zmq_state_t, zmq_state_free);

static const int query_delay = 1000;

int main(int argc, char *argv[])
{
	g_autoptr(GKeyFile) map = g_key_file_new();

	// Parse command line and config. If anything fails, the
	// program is terminated.
	config_parse_all(map, &argc, &argv);

	// Initialize message queue
	g_auto(zmq_state_t) zmq_state = zmq_state_init(map);
	
	// Initialize MODBUS handler.
	g_auto(modbus_state_t) modbus_state = modbus_state_init(map);

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
			if (do_zmq_magic(&zmq_state, &modbus_state, left) == false) {
				return 10;
			}
		}

		printf("Back to routine modbus\n");

		if ( do_modbus_magic(&modbus_state) == false) {
			return 10;
		}
	}
}

static zmq_state_t zmq_state_init(GKeyFile *map)
{
	zmq_state_t state;
	g_autoptr(GError) error = NULL;
	g_autofree gchar *path = g_key_file_get_string(map, "zmq", "socket", &error);
	config_check_key(error);
	state.pull = zsock_new_pull(path);
	if (state.pull == NULL) {
		err(7,"Unable to create ZMQ pull");
	}

	// Create poller from given puller
	state.poller = zpoller_new(state.pull, NULL);
	if (state.poller == NULL) {
		err(7,"Unable to creat ZMQ poller");
	}

	if (regcomp(&state.re_relay, "^RELAY ([1-4]) (ON|OFF)$", REG_EXTENDED | REG_ICASE)) {
		errx(1, "Unable to compile regex");
	}

	return state;
}

static bool do_zmq_magic(zmq_state_t *state, modbus_state_t *modbus_state, int left)
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

		printf("Releohjaus. Rele %d -> %d\n", relay, mode);
		modbus_state_ensure_baud_rate(modbus_state, 9600);
		// Write to relays
	}

	zstr_free (&msg);
	return true;
}

static void zmq_state_free(zmq_state_t *state)
{
	zpoller_destroy(&state->poller);
	zsock_destroy(&state->pull);
	regfree(&state->re_relay);
}

static bool do_modbus_magic(modbus_state_t *state)
{
	uint16_t dest[NB_REGS];
	int ret;

	modbus_state_ensure_baud_rate(state, 115200);
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
