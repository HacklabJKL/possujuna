
#include <stdio.h>
#include <stdlib.h>
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


int main( int argc, char *argv[])
{
	int n_errors = 0;
	int cumulative_errors = 0;
	int read_counter = 0;
	int ret;
	modbus_t *ctx;
	uint16_t dest [NB_REGS];

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
