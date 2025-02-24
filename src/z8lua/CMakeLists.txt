cmake_minimum_required(VERSION 3.10)

project(z8lua C)

set(CMAKE_C_STANDARD 99)

set (z8lua_sources
  lapi.c
  lauxlib.c
  lbaselib.c
  lbitlib.c
  lcode.c
  lcorolib.c
  lctype.c
  ldblib.c
  ldebug.c
  ldo.c
  ldump.c
  lfunc.c
  lgc.c
  linit.c
  liolib.c
  llex.c
  #lmathlib.c
  lmem.c
  loadlib.c
  lobject.c
  lopcodes.c
  loslib.c
  lparser.c
  lpico8lib.c
  lstate.c
  lstring.c
  lstrlib.c
  ltable.c
  ltablib.c
  ltests.c
  ltm.c
  lua.c
  lundump.c
  lvm.c
  lzio.c)

add_executable(z8lua ${z8lua_sources})
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
