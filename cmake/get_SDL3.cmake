macro(get_SDL3 version)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)
  if(MSVC OR (WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang"))
    FetchContent_Declare(
      SDL3
      URL https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-devel-${version}-VC.zip
      URL_HASH
        SHA256=dd7415a5245ac6cd7f6e08cd6261eebe35c48cfc67ac8c580139c36fd9945344)
  elseif(MINGW)
    FetchContent_Declare(
      SDL3
      URL https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-devel-${version}-mingw.zip
      URL_HASH
        SHA256=34e6eb4b964e612fb36ab58295a00e77ef625d5d5cc88f99317cd0d12a3df415)
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

  if(WIN32)
    FetchContent_MakeAvailable(SDL3)
    find_package(SDL3 CONFIG REQUIRED PATHS ${sdl3_SOURCE_DIR} NO_DEFAULT_PATH)
    get_target_property(SDL3_DLL SDL3::SDL3 IMPORTED_LOCATION)
    file(COPY_FILE ${SDL3_DLL} ${EXPORT_DIR}/SDL3.dll ONLY_IF_DIFFERENT)
  endif()
endmacro(get_SDL3)
