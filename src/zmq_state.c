#include <regex.h>
#include <err.h>
#include "zmq_state.h"
#include "config.h"

zmq_state_t zmq_state_init(GKeyFile *conf)
{
	zmq_state_t state;
	g_autoptr(GError) error = NULL;
	g_autofree gchar *path = g_key_file_get_string(conf, "zmq", "socket", &error);
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

void zmq_state_free(zmq_state_t *state)
{
	zpoller_destroy(&state->poller);
	zsock_destroy(&state->pull);
	regfree(&state->re_relay);
}
