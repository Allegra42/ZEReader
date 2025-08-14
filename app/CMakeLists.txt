# SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
#
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ZEReader LANGUAGES C)

add_subdirectory(src/sd)
add_subdirectory(src/epub)
add_subdirectory(src/ui)

target_sources(app PRIVATE src/main.c)
