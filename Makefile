asd: asd.c
	gcc -Wall asd.c -o modbussauttaja `pkg-config --cflags --libs libmodbus`
