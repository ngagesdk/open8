# Do not modify this file unless absolutely necessary.
# All project-specific settings can be found in the file:
# project_config.cmake

cmake_minimum_required(VERSION 3.10)

option (BUILD_CI_WORKFLOW "Build CI workflow" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(game C CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

add_compile_definitions(__NGAGE__)

set(UID1 0x1000007a) # KExecutableImageUidValue, e32uid.h
set(UID2 0x100039ce) # KAppUidValue16, apadef.h

set(game_static_libs
  ${EPOC_EXTRAS}/lib/armi/urel/SDL3.lib
  ${EPOC_EXTRAS}/lib/armi/urel/SDL3_mixer.lib)

set(game_libs
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

set(Pico-8_libs
  ${EPOC_LIB}/euser.lib
  ${EPOC_LIB}/apparc.lib
  ${EPOC_LIB}/cone.lib
  ${EPOC_LIB}/eikcore.lib
  ${EPOC_LIB}/avkon.lib)

set(Pico-8_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_application.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_appui.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_appview.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_document.cpp")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua)

add_library(game STATIC ${project_sources})
add_library(Pico-8 STATIC ${Pico-8_sources})
build_exe_static(game exe ${UID1} ${UID2} ${UID3} "${game_static_libs}" "${game_libs}")
build_dll(Pico-8 app ${UID1} ${UID2} ${APP_UID} "${Pico-8_libs}")

if(NOT BUILD_CI_WORKFLOW)
  build_aif(${CMAKE_CURRENT_SOURCE_DIR}/res Pico-8 ${APP_UID})
  build_resource(${CMAKE_CURRENT_SOURCE_DIR}/res Pico-8 "")
  build_sis(${CMAKE_CURRENT_SOURCE_DIR}/res Pico-8)

  add_dependencies(
    Pico-8.sis
    game.exe
    Pico-8.app
    Pico-8.aif
    Pico-8.rsc)
endif()

add_dependencies(
  game.exe
  game)

add_dependencies(
  game
  z8lua)

target_link_libraries(
  game
  z8lua)

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
  Pico-8.app
  Pico-8)

target_compile_definitions(
  Pico-8
  PUBLIC
  __DLL__
  UID1=${UID1}
  UID2=${UID2}
  UID3=${APP_UID})

target_include_directories(
  Pico-8
  PUBLIC
  ${SRC_DIR})
