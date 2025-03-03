# Do not modify this file unless absolutely necessary.
# All project-specific settings can be found in the file:
# project_config.cmake

cmake_minimum_required(VERSION 3.10)

option(BUILD_FOR_NOKIA_NGAGE "Build for Nokia N-Gage" OFF)
option (BUILD_CI_WORKFLOW "Build CI workflow" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

if (BUILD_FOR_NOKIA_NGAGE)

  project(game C CXX)

  add_compile_definitions(__NGAGE__)

  set(UID1 0x1000007a) # KExecutableImageUidValue, e32uid.h
  set(UID2 0x100039ce) # KAppUidValue16, apadef.h

  set(game_static_libs
    ${EPOC_EXTRAS}/lib/armi/urel/SDL3.lib
    ${EPOC_EXTRAS}/lib/armi/urel/SDL3_mixer.lib)

  set(game_libs
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/armi/urel/z8lua.lib
    ${EPOC_LIB}/NRenderer.lib
    ${EPOC_LIB}/3dtypes.a
    ${EPOC_LIB}/cone.lib
    ${EPOC_PLATFORM}/gcc/lib/gcc-lib/arm-epoc-pe/2.9-psion-98r2/libgcc.a
    ${EPOC_PLATFORM}/ngagesdk/lib/gcc/arm-epoc-pe/4.6.4/libgcc_ngage.a
    ${EPOC_LIB}/mediaclientaudiostream.lib
    ${EPOC_LIB}/charconv.lib
    ${EPOC_LIB}/bitgdi.lib
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/estlib.lib
    ${EPOC_LIB}/ws32.lib
    ${EPOC_LIB}/hal.lib
    ${EPOC_LIB}/fbscli.lib
    ${EPOC_LIB}/efsrv.lib
    ${EPOC_LIB}/scdv.lib
    ${EPOC_LIB}/gdi.lib)

  set(open8_libs
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/apparc.lib
    ${EPOC_LIB}/cone.lib
    ${EPOC_LIB}/eikcore.lib
    ${EPOC_LIB}/avkon.lib)

  set(open8_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage/ngage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage/ngage_application.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage/ngage_appui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage/ngage_appview.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage/ngage_document.cpp")

  add_library(game STATIC ${project_sources} ${ngage_sources})
  add_library(open8 STATIC ${open8_sources})
  build_exe_static(game exe ${UID1} ${UID2} ${UID3} "${game_static_libs}" "${game_libs}")
  build_dll(open8 app ${UID1} ${UID2} ${APP_UID} "${open8_libs}")

  if(NOT BUILD_CI_WORKFLOW)
    build_aif(${CMAKE_CURRENT_SOURCE_DIR}/res open8 ${APP_UID})
    build_resource(${CMAKE_CURRENT_SOURCE_DIR}/res open8 "")
    build_sis(${CMAKE_CURRENT_SOURCE_DIR}/res open8)

    add_dependencies(
      open8.sis
      game.exe
      open8.app
      open8.aif
      open8.rsc)
  endif()

  add_dependencies(
    game.exe
    game)

  target_compile_definitions(
    game
    PUBLIC
    __EXE__
    UID1=${UID1}
    UID2=${UID2}
    UID3=${UID3})

  target_include_directories(
    game
    PUBLIC
    ${SRC_DIR})

  add_dependencies(
    open8.app
    open8)

  target_compile_definitions(
    open8
    PUBLIC
    __DLL__
    UID1=${UID1}
    UID2=${UID2}
    UID3=${APP_UID})

  target_include_directories(
    open8
    PUBLIC
    ${SRC_DIR})

else()

  project(open8 C)

  set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/export)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXPORT_DIR})

  include(get_SDL3)
  get_SDL3("3.2.6")

  set(z8lua_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lauxlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lbaselib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lcode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lcorolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lctype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldblib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldebug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lgc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/linit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/llex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lobject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lopcodes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lparser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lpico8lib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstrlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltablib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lundump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lzio.c)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(open8 ${project_sources} ${z8lua_sources})
  else()
    add_executable(open8 WIN32 ${project_sources} ${z8lua_sources})
  endif()

  target_link_libraries(open8 PRIVATE ${SDL3_LIBRARIES})

  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

  configure_file("${sdl3_SOURCE_DIR}/lib/x86/SDL3.dll" ${CMAKE_CURRENT_SOURCE_DIR}/export COPYONLY)
  configure_file("${sdl3_SOURCE_DIR}/lib/x86/SDL3.pdb" ${CMAKE_CURRENT_SOURCE_DIR}/export COPYONLY)

  include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${sdl3_SOURCE_DIR}/include")

  if (NINTENDO_3DS)
    set(ROMFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/romfs")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/export/data DESTINATION ${ROMFS_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/export/carts DESTINATION ${ROMFS_DIR})
    set(SMDH_FILE "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.smdh")
    ctr_generate_smdh("${SMDH_FILE}"
      NAME "open8"
      DESCRIPTION "A portable PICO-8 emulator"
      AUTHOR "Michael Fitzmayer"
      ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")
    ctr_create_3dsx(
      ${APP_NAME}
      ROMFS "${ROMFS_DIR}"
      SMDH "${SMDH_FILE}")
  endif()

endif()
