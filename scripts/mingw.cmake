# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER C:/MinGW/bin/gcc.exe)
SET(CMAKE_CXX_COMPILER C:/MinGW/bin/g++.exe)
SET(CMAKE_RC_COMPILER C:/MinGW/bin/windres.exe)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH C:/MinGW/ )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)