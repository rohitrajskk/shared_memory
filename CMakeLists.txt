cmake_minimum_required(VERSION 3.0)
project(shm)

# Find libuv library
find_library(LIBUV uv)

# Add the executable for the first source file
add_executable(prod src/prod.c)

# Link libuv to the first binary
target_link_libraries(prod PRIVATE ${LIBUV})

target_compile_options(prod PRIVATE -g)

# Include libuv headers
target_include_directories(prod PRIVATE ${LIBUV_INCLUDE_DIRS})

# Add the executable for the second source file
add_executable(cons src/cons.c)

# Link libuv to the second binary
target_link_libraries(cons PRIVATE ${LIBUV})

target_compile_options(cons PRIVATE -g)

# Include libuv headers
# target_include_directories(cons PRIVATE ${LIBUV_INCLUDE_DIRS})

