.. SPDX-FileCopyrightText: 2025 Florian Limberger
..
.. SPDX-License-Identifier: MPL-2.0

Emulator
========

The ZEReader firmware can run in an emulator on Linux,
based on the ``native_sim`` board.
It enables developers to run the firmware without needing actual hardware,
which comes in handy for testing changes not related to the hardware,
e.g. to the UI or EPUB handling code.

Building and Running
--------------------

To build the emulator,
simply build the ``app`` for the ``native_sim`` board::

    west build -b native_sim/native/64 app

To simplify access to the emulated flash storage,
the FUSE development headers need to be present on the build host.
On Ubuntu, they are provided by the ``libfuse-dev`` package,
on AlmaLinux 9 by the ``fuse-devel`` package.
The app needs to be configured to use FUSE to access the emulated storage::

    west build -b native_sim/native/64 app -- -DCONFIG_FUSE_FS_ACCESS=y

The emulator is started by running the ``zephyr.exe`` binary file from the ``native_sim`` build
directory::

    ./build/zephyr/zephyr.exe

Storage Emulation
-----------------

The emulated storage uses a file named ``flash.bin``.
If it is not already present when the emulator is started,
the emulator creates it.

To put books onto the flash,
it needs to be mounted as a loopback device::

    sudo mount -o loop flash.bin /mnt
    sudo cp -r my-book /mnt
    sudo umount /mnt

If the emulator is built with FUSE support,
the books can just be copied to the ``flash/SD:`` directory when the emulator is running::

    cp -r my-book flash/SD\:

The emulator creates a ``flash`` directory in its working directory,
which can be safely removed after the emulator is stopped.
