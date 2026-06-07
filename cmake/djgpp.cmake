# DJGPP Toolchain for CMake
# This file configures CMake to build DOS executables using DJGPP

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i386)

# Specify the cross-compiler
find_program(CMAKE_C_COMPILER i586-pc-msdosdjgpp-gcc)
find_program(CMAKE_CXX_COMPILER i586-pc-msdosdjgpp-g++)
find_program(CMAKE_AR i586-pc-msdosdjgpp-ar)
find_program(CMAKE_RANLIB i586-pc-msdosdjgpp-ranlib)

# Adjust the default behavior of the find_xxx() commands
set(CMAKE_FIND_ROOT_PATH /djgpp)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# DJGPP specific flags
set(CMAKE_C_FLAGS_INIT "-std=c99")
set(CMAKE_CXX_FLAGS_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_INIT "")

# Disable shared libraries for DOS
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
