# passwords - A GnuPG based password manager
# Copyright (C) 2025  Adam Lanzafame
# Licensed under the GNU GPL v3 or later


cmake_minimum_required(VERSION 3.16)
project(password VERSION 0.1 LANGUAGES CXX)

#
#
# ============================
#   GIT METADATA
# ============================
#

find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")

    # Commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Branch name
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Dirty state
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet --ignore-submodules HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DIRTY_RESULT
    )

    if(GIT_DIRTY_RESULT EQUAL 1)
        set(GIT_DIRTY "-dirty")
    else()
        set(GIT_DIRTY "")
    endif()

else()

    # Fallbacks for tarballs or missing Git
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_BRANCH "unknown")
    set(GIT_DIRTY "")
endif()

#
# ============================
#   BUILD TIMESTAMP (OPTIONAL)
# ============================
#

option(ENABLE_BUILD_TIMESTAMP "Embed build timestamp in binary" ON)

if(ENABLE_BUILD_TIMESTAMP)
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M")
else()
    set(BUILD_TIMESTAMP "unknown")
endif()

# Generate header
configure_file(
    ${CMAKE_SOURCE_DIR}/src/gitversion.h.in
    ${CMAKE_BINARY_DIR}/gitversion.h
    @ONLY
)


#
# ============================
#   QT SETUP
# ============================
#
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Sql Network Charts Concurrent)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")
file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS "ui/*.ui")
file(GLOB_RECURSE RESOURCE_FILES CONFIGURE_DEPENDS "resources/*.qrc")

qt_add_executable(password WIN32 MACOSX_BUNDLE
    MANUAL_FINALIZATION
    ${SRC_FILES}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

#
# ============================
#   PLATFORM RESOURCES
# ============================
#
if(APPLE)
    set_source_files_properties(resources/password.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(password PRIVATE resources/password.icns)

    set_target_properties(password PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE "password.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "passwords"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.adamlanzafame.passwords"
    )

    # Strip Mach-O binary
    add_custom_command(TARGET password POST_BUILD
        COMMAND strip -x "$<TARGET_FILE:password>"
    )
endif()

if(WIN32)
    target_sources(password PRIVATE resources/appicon.rc)
endif()

#
# ============================
#   TARGET SETUP
# ============================
#
target_compile_definitions(password PRIVATE
    $<$<CONFIG:Debug>:APP_DEBUG_BUILD>
    $<$<CONFIG:Release>:APP_RELEASE_BUILD>
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

target_link_libraries(password PRIVATE
    Qt6::Widgets Qt6::Sql Qt6::Network Qt6::Charts Qt6::Concurrent
)

target_include_directories(password PRIVATE src ui ${CMAKE_BINARY_DIR})

qt_finalize_executable(password)

#
# ============================
#   HARDENING: GCC / CLANG / MINGW
# ============================
#
if(NOT MSVC)
    message(STATUS "GCC/Clang/MinGW hardening enabled")

    # Common hardening
    target_compile_options(password PRIVATE
        $<$<CONFIG:Release>:-fvisibility=hidden>
        $<$<CONFIG:Release>:-fstack-protector-strong>
        $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>
    )

    # PIE (Linux + MinGW only)
    if((UNIX AND NOT APPLE) OR MINGW)
        target_compile_options(password PRIVATE $<$<CONFIG:Release>:-fPIE>)
        target_link_options(password PRIVATE $<$<CONFIG:Release>:-pie>)
    endif()

    # Linux-only hardening
    if(UNIX AND NOT APPLE)
        target_compile_options(password PRIVATE
            $<$<CONFIG:Release>:-fstack-clash-protection>
            $<$<CONFIG:Release>:-fno-plt>
        )
        target_link_options(password PRIVATE
            $<$<CONFIG:Release>:-Wl,-z,noexecstack,-z,separate-code>
            $<$<CONFIG:Release>:-Wl,-z,relro,-z,now>
        )
    endif()

    # Strip symbols (Linux + MinGW)
    if((UNIX AND NOT APPLE) OR MINGW)
        target_link_options(password PRIVATE $<$<CONFIG:Release>:-s>)
    endif()
endif()

#
# ============================
#   HARDENING: MSVC
# ============================
#
if(MSVC)
    message(STATUS "MSVC hardening enabled")

    target_compile_options(password PRIVATE
        $<$<CONFIG:Release>:/GS>
        $<$<CONFIG:Release>:/guard:cf>
        $<$<CONFIG:Release>:/sdl>
        $<$<CONFIG:Release>:/GL>
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/Qspectre>
    )

    target_link_options(password PRIVATE
        $<$<CONFIG:Release>:/DYNAMICBASE>
        $<$<CONFIG:Release>:/NXCOMPAT>
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<CONFIG:Release>:/DEBUG:NONE>
    )
endif()
