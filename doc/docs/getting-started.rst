.. SPDX-FileCopyrightText: 2025 Anna-Lena Marx
..
.. SPDX-License-Identifier: MPL-2.0

Getting Started
===============

Setup a Local Development Environment
-------------------------------------
.. code-block:: bash

    west init -m git@github.com:Allegra42/ZEReader.git zereader-workspace

    cd zereader-workspace
    source .venv/bin/activate

    west update
    pip install -r zephyr-rtos/scripts/requirements.txt
    cd zephyr-rtos
    west sdk install

Build the Firmware Binary
-------------------------
Always source the venv environment first!

.. code-block:: bash

    cd zereader-workspace
    source .venv/bin/activate

    cd ZEReader

Raspberry Pi Pico 1
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    west build -b rpi_pico app -p

Raspberry Pi Pico 2
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    west build -b rpi_pico2/rp2350a/m33 app -p


Flash the Microcontroller
-------------------------
For the Raspberry Pi Pico controllers with UF2 binaries, do

+ Disconnect USB.
+ Press the ``BOOTSEL`` button during reconnecting.
  The device shows up as a mass storage.
+ Copy the ``build/zephyr/zephyr.uf2`` binary onto the device.
  It will flash the binary into the correct location and restarts.


Build the Documentation
-----------------------

Sphinx (Project Documentation)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. code-block:: bash

    cd doc
    make html

Doxygen (API Documentation)
~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. code-block:: bash

    cd doc
    doxygen


Update Zephyr and Zephyr-Modules
--------------------------------

.. code-block:: bash

    west update


