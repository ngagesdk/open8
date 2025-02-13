# Project configuration.

set(project_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cartridge_loader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/image_loader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/p8_compress.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/pxa_compress_snippets.c)

set(UID3 0x1000c37e) # game.exe UID
set(APP_UID 0x100051C0) # Pico-8.app UID
set(APP_NAME "Pico-8")
