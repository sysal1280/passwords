cmake_minimum_required(VERSION 3.16)

project(password VERSION 0.1 LANGUAGES CXX)

# Qt auto features
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Tell AUTOUIC where .ui files live
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Harden Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-fvisibility=hidden)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Sql)

# Collect sources
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
)

file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS
    "ui/*.ui"
)

file(GLOB_RECURSE RESOURCE_FILES CONFIGURE_DEPENDS
    "resources/*.qrc"
)

qt_add_executable(password WIN32
    MANUAL_FINALIZATION
    ${SRC_FILES}
    ${UI_FILES}
    ${RESOURCE_FILES}
    resources/appicon.rc
)

target_link_libraries(password PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

target_include_directories(password PRIVATE
    src
    ui
)

qt_finalize_executable(password)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(password PRIVATE QT_NO_DEBUG_OUTPUT)
endif()
