#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <errno.h>
#include <unistd.h>

/* Based on Kryptoradio init_serial_port function.
 * Setting serial parameters using non-standard baud rate
 * hack. References:
 * https://github.com/koodilehto/kryptoradio/blob/master/encoder/serial.c
 * http://stackoverflow.com/questions/3192478/specifying-non-standard-baud-rate-for-ftdi-virtual-serial-port-under-linux
 * http://stackoverflow.com/questions/4968529/how-to-set-baud-rate-to-307200-on-linux
 */

// Prototypes
static tcflag_t to_speed(const int speed);

int baudhack(int fd, int speed)
{
	// Check if speed preset is found or do we need to set a custom speed
	tcflag_t speed_preset = to_speed(speed);
	if (speed_preset == B0) {
		// Find current configuration
		struct serial_struct serial;
		if(ioctl(fd, TIOCGSERIAL, &serial) == -1) return -1;
		serial.flags = (serial.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
		serial.custom_divisor = (serial.baud_base + (speed / 2)) / speed;

		// Check that the serial timing error is no more than 2%
		int real_speed = serial.baud_base / serial.custom_divisor;
		if (real_speed < speed * 98 / 100 ||
		    real_speed > speed * 102 / 100) {
			errno = ENOTSUP;
			return -1;
		}

		// Activate
		if(ioctl(fd, TIOCSSERIAL, &serial) == -1) return -1;

		// Custom baudrate only works with this magic value
		speed_preset = B38400;
	}

	// Start with raw values
	struct termios term;
	if (tcgetattr(fd, &term) == -1) return -1;
	cfsetspeed(&term, speed_preset);
	if (tcsetattr(fd, TCSANOW, &term) == -1) return -1;

	return 0;
}

/**
 * Converts integer to baud rate. If no suitable preset is found,
 * return B0.
 */
static tcflag_t to_speed(const int speed)
{
	switch (speed) {
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	case 230400: return B230400;
	default: return B0; // Marks "non-standard"
	}
}
