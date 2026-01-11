# passwords - A GnuPG based password manager
# Copyright (C) 2025  Adam Lanzafame
# Licensed under the GNU GPL v3 or later


cmake_minimum_required(VERSION 3.16)

project(password VERSION 0.1 LANGUAGES CXX)

# Commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Branch name
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Dirty state (uncommitted changes)
execute_process(
    COMMAND git diff --quiet --ignore-submodules HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE GIT_DIRTY_RESULT
)

if(GIT_DIRTY_RESULT EQUAL 1)
    set(GIT_DIRTY "-dirty")
else()
    set(GIT_DIRTY "")
endif()

# Build date/time
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M")

configure_file(
    ${CMAKE_SOURCE_DIR}/src/gitversion.h.in
    ${CMAKE_BINARY_DIR}/gitversion.h
    @ONLY
)

# Qt auto features
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Tell AUTOUIC where .ui files live
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Sql Network Charts Concurrent)

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

qt_add_executable(password WIN32 MACOSX_BUNDLE
    MANUAL_FINALIZATION
    ${SRC_FILES}
    ${UI_FILES}
    ${RESOURCE_FILES}
    # Removed resources/appicon.rc from here; handled below for Windows only
)

if(APPLE)
    set_source_files_properties(
        resources/password.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )

    target_sources(password PRIVATE resources/password.icns)

    set_target_properties(password PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "password.icns"
    )
endif()

# Windows-only resource file (keep icon on Windows, ignore on macOS/Linux)
if(WIN32)
    target_sources(password PRIVATE resources/appicon.rc)
endif()

target_compile_definitions(password PRIVATE
    $<$<CONFIG:Debug>:APP_DEBUG_BUILD>
    $<$<CONFIG:Release>:APP_RELEASE_BUILD>
)

target_link_libraries(password PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Charts
    Qt6::Concurrent
)

target_include_directories(password PRIVATE
    src
    ui
)

target_include_directories(password PRIVATE ${CMAKE_BINARY_DIR})

qt_finalize_executable(password)

# Explicit macOS bundle properties – help ensure a proper .app with Info.plist
if(APPLE)
    set_target_properties(password PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "passwords"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.adamlanzafame.passwords"
    )
endif()

#
# ============================
#   RELEASE HARDENING (ALL)
# ============================
#

# Remove Qt debug output in Release
target_compile_definitions(password PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

#
# ============================
#   GCC / CLANG / MINGW
# ============================
#
if(NOT MSVC)
    message(STATUS "GCC/Clang/MinGW hardening enabled")

    # Hide internal symbols
    target_compile_options(password PRIVATE
        $<$<CONFIG:Release>:-fvisibility=hidden>
    )

    # Stack protection + fortified libc
    target_compile_options(password PRIVATE
        $<$<CONFIG:Release>:-fstack-protector-strong>
        $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>
    )

    # ---- PIE + ASLR ----
    if(UNIX OR MINGW)
        target_compile_options(password PRIVATE
            $<$<CONFIG:Release>:-fPIE>
        )
        target_link_options(password PRIVATE
            $<$<CONFIG:Release>:-pie>
        )
    endif()

    # ---- Additional hardening ----
    if(UNIX AND NOT APPLE)
        target_compile_options(password PRIVATE
            $<$<CONFIG:Release>:-fstack-clash-protection>
            $<$<CONFIG:Release>:-fno-plt>
        )
        target_link_options(password PRIVATE
            $<$<CONFIG:Release>:-Wl,-z,noexecstack,-z,separate-code>
        )
    endif()

    # Linux-only linker hardening
    if(UNIX AND NOT APPLE)
        target_link_options(password PRIVATE
            $<$<CONFIG:Release>:-Wl,-z,relro,-z,now>
        )
    endif()

    # Strip symbols (works on MinGW too)
    target_link_options(password PRIVATE
        $<$<CONFIG:Release>:-s>
    )
endif()

#
# ============================
#   MSVC (Visual Studio)
# ============================
#
if(MSVC)
    message(STATUS "MSVC hardening enabled")

    # Security flags
    target_compile_options(password PRIVATE
        $<$<CONFIG:Release>:/GS>
        $<$<CONFIG:Release>:/guard:cf>
        $<$<CONFIG:Release>:/sdl>
        $<$<CONFIG:Release>:/Qspectre>
        $<$<CONFIG:Release>:/GL>
    )

    # ASLR + DEP (Windows equivalents of PIE + NX)
    target_link_options(password PRIVATE
        $<$<CONFIG:Release>:/DYNAMICBASE>
        $<$<CONFIG:Release>:/NXCOMPAT>
        #$<$<CONFIG:Release>:/GUARD:EHCONT>
        $<$<CONFIG:Release>:/LTCG>
    )

    # Remove debug info + optimize + fold identical functions
    target_link_options(password PRIVATE
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<CONFIG:Release>:/DEBUG:NONE>
    )
endif()
