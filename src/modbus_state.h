#include <glib.h>
#include <modbus.h>

typedef struct {
	modbus_t *ctx;
	int current_baud;
	int baud_fd;
	int n_errors;
	int cumulative_errors;
	int read_counter;
} modbus_state_t;

// Initialize libmodbus context based on the parameters in the
// configuration file. Terminates if connection fails.
modbus_state_t modbus_state_init(GKeyFile *map);

// Unallocate state
void modbus_state_free(modbus_state_t *state);

// Use Glib helpers to manage memory
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(modbus_state_t, modbus_state_free);

void modbus_state_ensure_baud_rate(modbus_state_t *state, int baud);
