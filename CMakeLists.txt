cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(open8 C CXX)

if (NGAGESDK)
  find_package(SDL3 REQUIRED)
else()
  set(EXPORT_DIR ${PROJECT_SOURCE_DIR}/export)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXPORT_DIR})
  include(get_SDL3)
  get_SDL3("3.4.10")
endif()

set(open8_sources
  src/icon.rc
  src/main.c
  src/api.c
  src/app.c
  src/auxiliary.c
  src/core.c
  src/memory.c
  src/p8scii.c
  src/lexaloffle/p8_compress.c
  src/lexaloffle/pxa_compress_snippets.c)

add_executable(open8 WIN32 ${open8_sources})

if (NGAGESDK)
  set(open8_app_sources
    src/ngage/ngage.cpp
    src/ngage/ngage_application.cpp
    src/ngage/ngage_appui.cpp
    src/ngage/ngage_appview.cpp
    src/ngage/ngage_document.cpp
  )

  set(open8_app_libs
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/apparc.lib
    ${EPOC_LIB}/cone.lib
    ${EPOC_LIB}/eikcore.lib
    ${EPOC_LIB}/avkon.lib
  )

  add_library(open8_app STATIC ${open8_app_sources})

  target_compile_options(open8_app PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-ffast-math>
    $<$<CONFIG:Release>:-funroll-loops>
    $<$<CONFIG:Release>:-fomit-frame-pointer>
    $<$<CONFIG:Release>:-finline-functions>
    $<$<CONFIG:Release>:-fno-exceptions>
    $<$<CONFIG:Release>:-fno-rtti>
    $<$<CONFIG:Release>:-fstrict-aliasing>
    $<$<CONFIG:Release>:-fexpensive-optimizations>
  )

  build_dll(open8_app open8 app 0x1000007a 0x100039ce 0x100051c0 "${open8_app_libs}")
  build_aif(${PROJECT_SOURCE_DIR}/res open8 0x100051c0)
  build_resource(${PROJECT_SOURCE_DIR}/res open8 "")

  target_link_libraries(open8 PRIVATE SDL3::SDL3)
  target_link_libraries(open8 PRIVATE ${PROJECT_SOURCE_DIR}/lib/armi/urel/z8lua.lib)

  # Common optimizations for both C and C++
  target_compile_options(open8 PRIVATE
    $<$<CONFIG:Release>:-O3>                       # Maximum optimization level
    $<$<CONFIG:Release>:-ffast-math>               # Fast floating-point operations
    $<$<CONFIG:Release>:-funroll-loops>            # Loop unrolling for performance
    $<$<CONFIG:Release>:-fomit-frame-pointer>      # Use frame pointer as general register
    $<$<CONFIG:Release>:-finline-functions>        # Inline small functions
    $<$<CONFIG:Release>:-fstrict-aliasing>         # Strict aliasing rules
    $<$<CONFIG:Release>:-fexpensive-optimizations> # Expensive optimizations
  )

  # C++-only optimizations
  target_compile_options(open8 PRIVATE
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-fno-exceptions> # Disable C++ exceptions overhead
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-fno-rtti>       # Disable C++ RTTI overhead
  )

  set_property(TARGET open8 PROPERTY C_STANDARD 99)

  target_link_options(open8 PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
  target_link_options(open8 PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
  target_link_options(open8 PRIVATE "SHELL:-s UID3=0x1000c37e") # open8.exe UID
else()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua)
  target_link_libraries(open8 PRIVATE ${SDL3_LIBRARIES} z8lua)
endif()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${sdl3_SOURCE_DIR}/include")

if (NINTENDO_3DS)
  set(ROMFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/romfs")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/export/carts DESTINATION ${ROMFS_DIR})
  set(SMDH_FILE "${CMAKE_CURRENT_BINARY_DIR}/open8.smdh")
  ctr_generate_smdh("${SMDH_FILE}"
    NAME "open8"
    DESCRIPTION "A portable PICO-8 emulator"
    AUTHOR "Michael Fitzmayer"
    ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")
  ctr_create_3dsx(
    open8
    ROMFS "${ROMFS_DIR}"
    SMDH "${SMDH_FILE}")

elseif(EMSCRIPTEN)
  target_link_libraries(open8 PRIVATE ${SDL3_LIBRARIES})
  target_link_options(open8 PRIVATE
    "SHELL:--preload-file ${EXPORT_DIR}/carts/api.p8.png@carts/api.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/automata.p8.png@carts/automata.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/celeste.p8.png@carts/celeste.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/demo.p8.png@carts/demo.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/hello.p8.png@carts/hello.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/lukiprueba.p8.png@carts/lukiprueba.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/p8scii.p8.png@carts/p8scii.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/porterpatch1.p8.png@carts/porterpatch1.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/sort.p8.png@carts/sort.p8.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/waves.p8.png@carts/waves.p8.png"
  )
  set_property(TARGET open8 APPEND_STRING PROPERTY LINK_FLAGS " ${EMSCRIPTEN_PRELOAD_FLAGS}")

elseif(UNIX)
  target_link_libraries(open8 PRIVATE ${SDL3_LIBRARIES} m)
endif()
