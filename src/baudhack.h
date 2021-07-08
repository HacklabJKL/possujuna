/**
 * Changes baud rate of live serial port. This function also supports
 * non-standard baud rates if the underlying hardware is capable. If
 * it fails, -1 is returned and errno is set. Otherwise, zero is
 * returned.
 */
int baudhack(int fd, int speed);
