cmake_minimum_required(VERSION 3.10)

option (BUILD_TESTS "Build Unit Tests" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(open8 C CXX)

if (NGAGESDK)
  find_package(SDL3 REQUIRED)
elseif(VITA)
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

set_property(TARGET open8 PROPERTY C_STANDARD 99)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(open8 PRIVATE SDL3::SDL3)

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

  target_link_options(open8 PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
  target_link_options(open8 PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
  target_link_options(open8 PRIVATE "SHELL:-s UID3=0x1000c37e") # open8.exe UID
else()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua)
  target_link_libraries(open8 PRIVATE z8lua)
endif()

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

elseif(PLATFORM_DREAMCAST)
  # FIXME: This should be inherited from the SDL3::SDL3 target
  target_link_libraries(open8 PRIVATE pthread GL)

  target_compile_options(open8 PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-ffast-math>
    $<$<CONFIG:Release>:-funroll-loops>
    $<$<CONFIG:Release>:-fomit-frame-pointer>)

  target_compile_options(open8 PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

elseif(PS2)
  set_target_properties(open8 PROPERTIES SUFFIX ".elf")

  target_compile_options(open8 PRIVATE
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-ffast-math>)

elseif(PSP)
  create_pbp_file(
	TARGET open8
    ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icon.png"
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
	TITLE open8)

elseif(VITA)

  if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "VITASDK not set")
  endif()

  # Vita SDK helper macros.
  include("$ENV{VITASDK}/share/vita.cmake")

  # Vita metadata.
  set(VITA_APP_NAME "open8")
  set(VITA_TITLE_ID "OPEN00001")

  # Build SELF + VPK.
  vita_create_self(eboot.self open8 UNSAFE)

  vita_create_vpk(open8.vpk ${VITA_TITLE_ID} eboot.self
    VERSION "01.00"
    NAME ${VITA_APP_NAME}
  )

elseif(EMSCRIPTEN)
  target_link_options(open8 PRIVATE
    "SHELL:--preload-file ${EXPORT_DIR}/overlay.png@overlay.png"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/0FUZ.PNG@carts/0FUZ.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/1CELESTE.PNG@carts/1CELESTE.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/2API.PNG@carts/2API.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/3JELPI.PNG@carts/3JELPI.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/4RACER.PNG@carts/4RACER.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/5PICOPON.PNG@carts/5PICOPON.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/6PROVNCE.PNG@carts/6PROVNCE.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/7PICROSS.PNG@carts/7PICROSS.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/8CABRIDE.PNG@carts/8CABRIDE.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/9CARDSWP.PNG@carts/9CARDSWP.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/APICOFOX.PNG@carts/APICOFOX.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/BSMB.PNG@carts/BSMB.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/CMOTNREC.PNG@carts/CMOTNREC.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/PICOPORT.PNG@carts/PICOPORT.PNG"
    "SHELL:--preload-file ${EXPORT_DIR}/carts/WOLFHUNT.PNG@carts/WOLFHUNT.PNG")

  set_property(TARGET open8 APPEND_STRING PROPERTY LINK_FLAGS " ${EMSCRIPTEN_PRELOAD_FLAGS}")

elseif(RISCOS)
  file(COPY
    ${CMAKE_SOURCE_DIR}/res/!Boot,feb
    ${CMAKE_SOURCE_DIR}/res/!Run,feb
    ${CMAKE_SOURCE_DIR}/res/!Sprites,ff9
    ${CMAKE_SOURCE_DIR}/export/carts
    DESTINATION !Open8/
  )

  target_link_options(open8 PRIVATE -static)
  add_custom_command(
    OUTPUT "!Open8/!RunImage,ff8"
    COMMAND elf2aif "$<TARGET_FILE:open8>" "!Open8/!RunImage,ff8"
    DEPENDS open8
    VERBATIM
  )
  add_custom_target(
    open8_aif ALL
    DEPENDS "!Open8/!RunImage,ff8"
  )

elseif(UNIX)
  target_link_libraries(open8 PRIVATE m)
endif()

if (BUILD_TESTS)
  add_subdirectory(tests)
endif()
