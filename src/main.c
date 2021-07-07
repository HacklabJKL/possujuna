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

#define NB_REGS 2
#define UART_PATH "/dev/ttyS0"
#define BAUDRATE 115200

// Prototypes
static void check_key_error(GError *error);

// Command line option parser
static gchar *conf_file = NULL;
static GOptionEntry entries[] =
{
	{ "conf", 'c', 0, G_OPTION_ARG_FILENAME, &conf_file, "Configuration file", "FILE"},
	{ NULL }
};

// Terminate program if GLib error detected
static void check_key_error(GError *error)
{
	// If ok, don't do anything
	if (error == NULL) return;
	
	if (!g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND))
	{
		errx(4, "Key not found in configuration: %s", error->message);
	}

	if (!g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE))
	{
		errx(4, "Key data type is invalid in configuration: %s", error->message);
	}

	errx(4, "GLib error while reading configuration");
}

int main( int argc, char *argv[])
{
	g_autoptr(GKeyFile) map = g_key_file_new();
	int n_errors = 0;
	int cumulative_errors = 0;
	int read_counter = 0;
	int ret;
	modbus_t *ctx;
	uint16_t dest [NB_REGS];

	{
		GError *error = NULL;
		GOptionContext *context;
	
		context = g_option_context_new("- Reads MODBUS and writes database");
		g_option_context_add_main_entries(context, entries, NULL);
		if (!g_option_context_parse(context, &argc, &argv, &error))
		{
			errx(1, "option parsing failed: %s", error->message);
		}
		
		if (conf_file == NULL) {
			errx(2, "Option --conf is mandatory. Try '%s --help'", argv[0]);
		}
	}
	
	{
		g_autoptr(GError) error = NULL;
		if (!g_key_file_load_from_file(map, conf_file, G_KEY_FILE_NONE, &error))
		{
			if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
				errx(3, "Error loading key file: %s", error->message);
			}
			err(3, "Unable to read configuration");
		}
	}

	ctx = modbus_new_rtu(UART_PATH, BAUDRATE, 'N', 8, 1);
	modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
	modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_DOWN);

	if (ctx == NULL) {
		perror("Unable to create the libmodbus context\n");
		return -1;
	}

	ret = modbus_set_slave(ctx, 1);
	if(ret < 0){
		perror("modbus_set_slave error\n");
		return -1;
	}
	ret = modbus_connect(ctx);
	if(ret < 0){
		perror("modbus_connect error\n");
		return -1;
	}

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
