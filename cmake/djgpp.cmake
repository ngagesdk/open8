# DJGPP Toolchain for CMake
# This file configures CMake to build DOS executables using DJGPP

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i386)

# Mark that we're building for DOS
# This triggers SDL3's PreseedDOSCache.cmake automatically
set(DOS TRUE)

# Specify the cross-compiler
find_program(CMAKE_C_COMPILER i586-pc-msdosdjgpp-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER i586-pc-msdosdjgpp-g++ REQUIRED)
find_program(CMAKE_AR i586-pc-msdosdjgpp-ar REQUIRED)
find_program(CMAKE_RANLIB i586-pc-msdosdjgpp-ranlib REQUIRED)

# Get the DJGPP directory from the toolchain path
get_filename_component(DJGPP_BIN_DIR ${CMAKE_C_COMPILER} DIRECTORY)
get_filename_component(DJGPP_DIR ${DJGPP_BIN_DIR} DIRECTORY)

# Set DJGPP environment variable for proper compilation
set(ENV{DJGPP} ${DJGPP_DIR}/djgpp.env)

# Adjust the default behavior of the find_xxx() commands
set(CMAKE_FIND_ROOT_PATH ${DJGPP_DIR})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# DJGPP specific flags
set(CMAKE_C_FLAGS_INIT "-std=c99")
set(CMAKE_CXX_FLAGS_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_INIT "")

# Tell CMake not to run compiler checks that may fail with DJGPP cross-compiler
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Disable shared libraries for DOS
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)


