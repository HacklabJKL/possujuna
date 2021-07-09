#include <glib.h>
#include <czmq.h>

typedef struct {
	zpoller_t *poller;
	zsock_t *pull;
	regex_t re_relay;
} zmq_state_t;

zmq_state_t zmq_state_init(GKeyFile *conf);
void zmq_state_free(zmq_state_t *state);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(zmq_state_t, zmq_state_free);
