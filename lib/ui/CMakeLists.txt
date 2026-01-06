# SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
#
# SPDX-License-Identifier: MPL-2.0

zephyr_library()
zephyr_library_sources(
  context.c
  ui.c
  ui_style.c
  widgets/button.c
  widgets/book_menu.c
  widgets/control_bar.c
  widgets/status_bar.c
  widgets/text_area.c
  widgets/logo.c
  )
