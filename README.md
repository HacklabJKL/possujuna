# Possujuna
Modbus to PostgreSQL data bridge in C

## Requirements

```
sudo apt install libpq-dev libmodbus-dev libczmq-dev
```

## Building

It uses CMake, so pretty standard. A very common practice is to use a
separate build directory:

```sh
mkdir build
cd build
cmake ..
```

And then just:

```sh
make
```

## Usage

Make a copy of `example.conf` and adapt it to your needs. Then start
the program by running:

```sh
./possujuna -c CONFIG_FILE
```

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.
