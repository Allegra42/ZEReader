# SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
#
# SPDX-License-Identifier: MPL-2.0

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/context.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_style.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/widgets/button.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/widgets/control_bar.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/widgets/status_bar.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/widgets/text_area.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/widgets/logo.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/widgets/book_menu.c)