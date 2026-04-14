# SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
#
# SPDX-License-Identifier: MPL-2.0

zephyr_library_named(sd)
zephyr_library_sources(sd.c)
zephyr_library_link_libraries(ELMFAT)
