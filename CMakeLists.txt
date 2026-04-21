cmake_minimum_required(VERSION 3.20)
project(npy VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Try to find libgit2 via pkg-config first, fall back to FetchContent
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBGIT2 QUIET libgit2)
endif()

if(NOT LIBGIT2_FOUND)
    message(STATUS "libgit2 not found via pkg-config, fetching from source...")
    include(FetchContent)
    FetchContent_Declare(
        libgit2
        GIT_REPOSITORY https://github.com/libgit2/libgit2.git
        GIT_TAG        v1.7.2
    )
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_CLI OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(libgit2)

    add_executable(npy
        src/main.cpp
        src/global.cpp
        src/cmd.cpp
        src/fs.cpp
        src/git_w.cpp
    )
    target_include_directories(npy PRIVATE
        ${libgit2_SOURCE_DIR}/include
        ${libgit2_BINARY_DIR}/include
    )
    target_link_libraries(npy PRIVATE libgit2package)
else()
    add_executable(npy
        src/main.cpp
        src/global.cpp
        src/cmd.cpp
        src/fs.cpp
        src/git_w.cpp
    )
    target_include_directories(npy PRIVATE ${LIBGIT2_INCLUDE_DIRS})
    target_link_libraries(npy PRIVATE ${LIBGIT2_LIBRARIES})
    target_link_directories(npy PRIVATE ${LIBGIT2_LIBRARY_DIRS})
endif()
