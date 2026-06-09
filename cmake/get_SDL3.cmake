macro(get_SDL3 version)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)

  # For DOS/DJGPP, build SDL3 from source
  # SDL3's built-in PreseedDOSCache.cmake will automatically configure DOS support
  if(CMAKE_SYSTEM_NAME STREQUAL "DOS")
    find_package(SDL3 QUIET)
    if(NOT SDL3_FOUND)
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
      )
      FetchContent_MakeAvailable(SDL3)
      set(SDL3_LIBRARIES SDL3::SDL3)
    endif()
  elseif(HAIKU OR CMAKE_SYSTEM_NAME STREQUAL "PS2")
    FetchContent_Declare(
      SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG release-${version}
    )
    FetchContent_MakeAvailable(SDL3)
    set(SDL3_LIBRARIES SDL3::SDL3)
  elseif(HAIKU OR CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    FetchContent_Declare(
      SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG release-${version}
    )
    FetchContent_MakeAvailable(SDL3)
    set(SDL3_LIBRARIES SDL3::SDL3)
  elseif(MSVC OR (WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang"))
    FetchContent_Declare(
      SDL3
      URL https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-devel-${version}-VC.zip
      URL_HASH
        SHA256=e2b336b10b037934af98308027410732ef7b22f2c6697d58092aa1c209fae7d7)
  elseif(MINGW)
    FetchContent_Declare(
      SDL3
      URL https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-devel-${version}-mingw.zip
      URL_HASH
        SHA256=5b2bc8589974391e7b3fa4d1bca6c4e09657254afeea57bb4dbe239932188695)
  else()
    find_package(SDL3 QUIET)
    if(NOT SDL3_FOUND)
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG "release-${version}")
      FetchContent_MakeAvailable(SDL3)
      set(SDL3_LIBRARIES SDL3::SDL3)
    endif()
  endif()

  if(WIN32 AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386"))
    FetchContent_MakeAvailable(SDL3)
    find_package(SDL3 CONFIG REQUIRED PATHS ${sdl3_SOURCE_DIR} NO_DEFAULT_PATH)
    get_target_property(SDL3_DLL SDL3::SDL3 IMPORTED_LOCATION)
    if(SDL3_DLL)
      file(COPY_FILE ${SDL3_DLL} ${EXPORT_DIR}/SDL3.dll ONLY_IF_DIFFERENT)
    endif()
  endif()
endmacro(get_SDL3)
