cmake_minimum_required(VERSION 3.13)
project(blackbook_core
    VERSION 1.0.0
    LANGUAGES CXX C
)

# We need compile_commands.json for the include paths in some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include_directories(
    src
    lib
    ../../lib/bobtail

    ../../lib/sqlite
    ../../lib/raudio/src

    ${ICU_INCLUDE_DIRS}
)

add_library(stb_image STATIC
    ../../lib/stb_image.c
)

# Dependencies
set(BOBTAIL_OPENGL ON)
add_subdirectory(../../lib/bobtail bobtail)

set(SUPPORT_FILEFORMAT_FLAC OFF)
set(SUPPORT_FILEFORMAT_QOA OFF)
set(SUPPORT_FILEFORMAT_OGG OFF)
add_subdirectory(../../lib/raudio/projects/CMake raudio)

# We turn off as many SQLite options as we can to reduce compile time
set(ENABLE_STATIC ON)
set(ENABLE_SHARED OFF)
set(ENABLE_DYNAMIC_EXTENSIONS ON)
set(BUILD_SHELL OFF)
set(ENABLE_FTS4 OFF) # We only use FTS5
set(ENABLE_RTREE OFF)
set(OMIT_JSON OFF)
set(OMIT_AUTOINIT OFF)
add_subdirectory(../../lib/sqlite sqlite)

add_library(blackbook_core STATIC
    id3.cxx
    import.cxx
    tags.cxx
    sqlgen.cxx
    expression.cxx
    blackbook_core.cxx

    # Generic sources
    arguments.cxx
    scope_timer.cxx
    text_i8n.cxx

    sqlite-crc32.c

    autocomplete.cxx
    db_versioning.cxx
    filecopy.cxx
)

target_link_libraries(blackbook_core PUBLIC
    bobtail sqlite3-static raudio stb_image
)

if (WIN32)
    # SQLite needs this to generate UUIDs
    target_link_libraries(blackbook_core PUBLIC Rpcrt4)
endif()
