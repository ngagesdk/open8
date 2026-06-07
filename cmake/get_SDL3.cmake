macro(get_SDL3 version)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)

  # For DOS/DJGPP, build SDL3 from source with DOS-appropriate configuration
  if(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386")
    find_package(SDL3 QUIET)
    if(NOT SDL3_FOUND)
      # Set cache variables for SDL3 DJGPP/DOS build
      # SDL3 has built-in DOS support, so we configure minimal features
      set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as a shared library" FORCE)
      set(SDL_STATIC ON CACHE BOOL "Build SDL3 as a static library" FORCE)
      set(SDL_TESTS OFF CACHE BOOL "Build SDL3 tests" FORCE)
      set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL3 examples" FORCE)
      set(SDL_GPU OFF CACHE BOOL "Disable GPU support for DOS" FORCE)
      set(SDL_CAMERA OFF CACHE BOOL "Disable camera for DOS" FORCE)
      set(SDL_HAPTIC OFF CACHE BOOL "Disable haptic for DOS" FORCE)
      set(SDL_HIDAPI OFF CACHE BOOL "Disable HIDAPI for DOS" FORCE)
      set(SDL_POWER OFF CACHE BOOL "Disable power for DOS" FORCE)
      set(SDL_SENSOR OFF CACHE BOOL "Disable sensors for DOS" FORCE)
      set(SDL_DIALOG OFF CACHE BOOL "Disable dialogs for DOS" FORCE)
      set(SDL_DUMMYCAMERA OFF CACHE BOOL "Disable dummy camera for DOS" FORCE)
      set(SDL_OFFSCREEN OFF CACHE BOOL "Disable offscreen video for DOS" FORCE)
      set(SDL_RENDER_GPU OFF CACHE BOOL "Disable GPU render for DOS" FORCE)
      set(SDL_TRAY OFF CACHE BOOL "Disable tray for DOS" FORCE)
      set(SDL_PROCESS OFF CACHE BOOL "Disable process for DOS" FORCE)
      # Note: SDL_THREADS is NOT disabled - SDL3 requires it even for DOS

      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG "release-${version}")
      FetchContent_MakeAvailable(SDL3)
      set(SDL3_LIBRARIES SDL3::SDL3)
    endif()
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
