# Project configuration.

set(project_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/api.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/auxiliary.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/image_loader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/memory.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/p8scii.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/p8_compress.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/pxa_compress_snippets.c)

set(ngage_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ngage/api.cpp)

set(UID3 0x1000c37e) # game.exe UID
set(APP_UID 0x100051C0) # open8.app UID
set(APP_NAME "open8")
