<!--
SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>

SPDX-License-Identifier: MPL-2.0
-->

# ZEReader

[![Build Zephyr ZEReader Application](https://github.com/Allegra42/ZEReader/actions/workflows/build-zephyr.yaml/badge.svg)](https://github.com/Allegra42/ZEReader/actions/workflows/build-zephyr.yaml)
[![Build ZEReader documentation](https://github.com/Allegra42/ZEReader/actions/workflows/build-docs.yaml/badge.svg)](https://github.com/Allegra42/ZEReader/actions/workflows/build-docs.yaml)

<a href="https://allegra42.github.io/ZEReader/">
  <img alt="Documentation" src="https://img.shields.io/badge/documentation-3D578C?logo=sphinx&logoColor=white">
</a>
<a href="https://allegra42.github.io/ZEReader/doxygen">
  <img alt="API Documentation" src="https://img.shields.io/badge/API-documentation-3D578C?logo=c&logoColor=white">
</a>

<img alt="ZEReader in Action" src='/doc/pics/ZEReader.webp' width="500" />

ZEReader is a Zephyr based firmware for an open hardware eBook reader approach.
See https://github.com/Allegra42/ZEReader-KiCad for more information about the hardware design.

The firmware is currently in a really early PoC stage.
Nevertheless, due to Zephyr, it is easily portable and adaptable to different hardware platforms
which provide the essential components and a proper device tree (overlay) configuration.

## Basic Hardware Components
For developing and testing the following basic components are used:

- Raspberry Pi Pico 1/2
- Waveshare ePaper Display 7,5" (Gooddisplay GDEW075T7, GDEY075T7) with UC8179 controller
- SD Card Reader (SPI)
- 4 Buttons for navigation

With proper device tree (overlay) and maybe some prj.conf or currently even code configurations,
the firmware should be easily adaptable to other microcontrollers and/or displays.

## Setup for local development
```
west init -m git@github.com:Allegra42/ZEReader.git zereader-workspace

cd zereader-workspace

west update
pip install -r zephyr-rtos/scripts/requirements.txt
cd zephyr-rtos
west sdk install
```


## Build
### Build for Raspberry Pi Pico 1
```
west build -b rpi_pico app -p always
```

### Build for Raspberry Pi Pico 2
```
west build -b rpi_pico2/rp2350a/m33 app -p always
```

### Other useful build commands
```
# Update Zephyr and external modules
# The west.yml of this project uses potentially unstable main branches from Zephyr and the modules.
# This was needed as neighter Pi Pico 2 nor LVGL v9 were included in the 4.0 release, but more or less important for this project.
# Be aware this can break!

west update

# May check if patches in this repo are (still) needed or new ones needed or new issues showed up.

# Generate SBoM
west spdx
```

## Flash Pico
- disconnect USB
- press the 'BOOTSEL' button during reconnect
- the device shows up as a mass storage
- copy 'build/zephyr/zephyr.uf2' to the device
- it flashes itself and restarts running the firmware

## Logs
Logs are printed via the Pi Pico default UART pins with 115200 bauds.

## Debugging
With Blackmagic Debug Probe using the SWD interface:

- connect the Blackmagic Debug Probe to the Pi Pico's SWD header
- `west debug --runner blackmagicprobe`
-> Connects to a GDB session

- set breakpoint with
  `break <full/source/code/file/path>:<linenumber>`
- step through / step in with GDB commands (next/n, step in/si, ..)

## Current state
This is still a work-in-process.
The firmware shows a proof-of-concept that reading EPUBs as they are is possible with the available resources of a cheap microcontroller unit like the RP2040/RP2350.
(Right now, without unzipping the EPUB files directly from Zephyr.)

The firmware supports showing the contents of a book in correct order while jumping over pictures and leave (X)HTML formatting untouched.

The UI implementation is still very basic.
The display orientation is still landscape and refresh configurations need to be tuned for an actually usable reading experience.

Config options for EPUB handling are still part of epub.h and should move to an own KConfig definition as the project proceeds in order to keep these configurations in a central location adopt Zephyr idioms.

See the GitHub issue page for a closer look on planned tasks.