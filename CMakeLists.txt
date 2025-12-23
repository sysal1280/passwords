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

#
# ============================
#   RELEASE HARDENING (ALL)
# ============================
#

# Remove Qt debug output in Release
target_compile_definitions(password PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

# Hide internal symbols (GCC/Clang/MinGW)
target_compile_options(password PRIVATE
    $<$<CONFIG:Release>:-fvisibility=hidden>
)

#
# ============================
#   GCC / CLANG / MINGW
# ============================
#
if(NOT MSVC)
    message(STATUS "GCC/Clang/MinGW hardening enabled")

    # Stack protection + fortified libc
    target_compile_options(password PRIVATE
        $<$<CONFIG:Release>:-fstack-protector-strong>
        $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>
    )

    # Linker hardening + strip symbols
    target_link_options(password PRIVATE
        $<$<CONFIG:Release>:-Wl,-z,relro,-z,now>
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
    )

    # Remove debug info + optimize + fold identical functions
    target_link_options(password PRIVATE
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<CONFIG:Release>:/DEBUG:NONE>
    )
endif()
