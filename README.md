# RingFS, a small Flash-based ring buffer.

[![Build Status](https://travis-ci.org/cloudyourcar/ringfs.svg)](https://travis-ci.org/cloudyourcar/ringfs)

RingFS is a persistent, Flash-based ring buffer designed for embedded software.
It's aimed at storing non-critical data that can be expunged on the FIFO basis
as needed. Typical uses include:

* telemetry data,
* debug logs,
* stack traces.

RingFS has been designed to run on NOR Flash memory, which exhibits the following
semantics:

1. Bits are programmed by flipping them from 1 to 0 with byte granularity.
2. Bits are erased by flipping them from 0 to 1 with sector granularity.

## Features

* Designed and optimized for NOR Flash memory.
* Stores fixed-size objects in a FIFO buffer.
* Written in ISO C99.
* No dynamic memory allocation.
* Basic robustness features for error recovery.

## Usage

1. Add ``ringfs.c`` and ``ringfs.h`` to your project.
2. Implement the required Flash ops (``sector_erase``, ``program``, ``read``).
3. Glue your Flash ops with ringfs using ``struct ringfs_flash_partition``.

See ``example.c`` if this sounds complicated.

### Write behavior when full

Ringfs requires at least one sector free when it appends data as it moves the
read- and cursor heads to the next sector when the current one is full. By
default it will erase the next sector if needed. This behavior can be prevented
by setting `ringfs.reject_write_when_full` to `1`. When enabled `ringfs_append`
will return `RINGFS_FULL` and the data will be rejected.

Due to this the filesystem can at most hold `size - sector_size` of data.

## Documentation

See Doxygen-generated documentation at http://cloudyourcar.github.io/ringfs/.

## Non-Features

The ring buffer has been designed to be as simple as possible. Therefore, the
following are non-features that will *not* be implemented:

* Variable object sizes (makes things much more complicated).
* Complicated error recovery (we can lose data in edge cases).
* Upgrades (complex, also unnecessary in our use cases).

Actually, on the second thought, I may consider adding support for variable
object sizes some day.

## License

> Copyright © 2014 Kosma Moczek \<kosma@cloudyourcar.com\>
>
> This program is free software. It comes without any warranty, to the extent
> permitted by applicable law. You can redistribute it and/or modify it under
> the terms of the Do What The Fuck You Want To Public License, Version 2, as
> published by Sam Hocevar. See the COPYING file for more details.
