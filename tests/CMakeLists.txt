cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

project(tests C)

set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXPORT_DIR}")

set(CMAKE_C_STANDARD 99)

include(get_SDL3)
get_SDL3("3.2.12")

set(PICO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")

set(base_sources
  ${PICO_DIR}/api.c
  ${PICO_DIR}/auxiliary.c
  ${PICO_DIR}/memory.c
  ${PICO_DIR}/p8scii.c
  ${PICO_DIR}/z8lua/lapi.c
  ${PICO_DIR}/z8lua/lauxlib.c
  ${PICO_DIR}/z8lua/lbaselib.c
  ${PICO_DIR}/z8lua/lcode.c
  ${PICO_DIR}/z8lua/lcorolib.c
  ${PICO_DIR}/z8lua/lctype.c
  ${PICO_DIR}/z8lua/ldblib.c
  ${PICO_DIR}/z8lua/ldebug.c
  ${PICO_DIR}/z8lua/ldo.c
  ${PICO_DIR}/z8lua/ldump.c
  ${PICO_DIR}/z8lua/lfunc.c
  ${PICO_DIR}/z8lua/lgc.c
  ${PICO_DIR}/z8lua/linit.c
  ${PICO_DIR}/z8lua/llex.c
  ${PICO_DIR}/z8lua/lmem.c
  ${PICO_DIR}/z8lua/lobject.c
  ${PICO_DIR}/z8lua/lopcodes.c
  ${PICO_DIR}/z8lua/lparser.c
  ${PICO_DIR}/z8lua/lpico8lib.c
  ${PICO_DIR}/z8lua/lstate.c
  ${PICO_DIR}/z8lua/lstring.c
  ${PICO_DIR}/z8lua/lstrlib.c
  ${PICO_DIR}/z8lua/ltable.c
  ${PICO_DIR}/z8lua/ltablib.c
  ${PICO_DIR}/z8lua/ltm.c
  ${PICO_DIR}/z8lua/lundump.c
  ${PICO_DIR}/z8lua/lvm.c
  ${PICO_DIR}/z8lua/lzio.c)

set(tests_sources ${CMAKE_CURRENT_SOURCE_DIR}/tests.c)

add_executable(tests ${base_sources} ${tests_sources})
target_link_libraries(tests PRIVATE ${SDL3_LIBRARIES})

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/../src"
  "${sdl3_SOURCE_DIR}/include")
