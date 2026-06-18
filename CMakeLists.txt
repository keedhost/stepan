cmake_minimum_required(VERSION 3.20)
project(Stepan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebEngineWidgets WebChannel Network)
# QSystemTrayIcon lives in Widgets; no extra component needed.
qt_standard_project_setup()

# ── GeographicLib (static, built from source via FetchContent) ────────────────
include(FetchContent)
FetchContent_Declare(
    GeographicLib
    GIT_REPOSITORY https://github.com/geographiclib/geographiclib.git
    GIT_TAG        v2.3
    GIT_SHALLOW    TRUE
)
# Build only the library, skip tools/docs/tests
set(GEOGRAPHICLIB_LIB_TYPE   STATIC CACHE STRING "" FORCE)
set(BUILD_DOCUMENTATION      OFF    CACHE BOOL   "" FORCE)
set(EXAMPLEDIR               ""     CACHE PATH   "" FORCE)
FetchContent_MakeAvailable(GeographicLib)

# ── Application target ────────────────────────────────────────────────────────
qt_add_executable(Stepan
    src/main.cpp
    src/MainWindow.cpp
    src/CoordinateParser.cpp
    src/MapWidget.cpp
    src/TrayManager.cpp
    src/AutoStart.cpp
    src/SettingsDialog.cpp
    src/WebServer.cpp
    src/AboutDialog.cpp
    resources/resources.qrc
)

target_include_directories(Stepan PRIVATE src)

target_link_libraries(Stepan PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::Network
    GeographicLib::GeographicLib
)

# ── Platform-specific settings ────────────────────────────────────────────────
if(APPLE)
    # Include .icns in the bundle
    set(APP_ICNS "${CMAKE_SOURCE_DIR}/resources/app.icns")
    set_source_files_properties(${APP_ICNS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(Stepan PRIVATE ${APP_ICNS})

    set_target_properties(Stepan PROPERTIES
        MACOSX_BUNDLE                      TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER       "ua.ss-education.stepan"
        MACOSX_BUNDLE_BUNDLE_NAME          "Stepan"
        MACOSX_BUNDLE_BUNDLE_VERSION       ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_ICON_FILE            "app.icns"
        MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
    )
endif()

if(WIN32)
    target_sources(Stepan PRIVATE "${CMAKE_SOURCE_DIR}/resources/app.rc")
endif()

if(WIN32)
    set_target_properties(Stepan PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Lowercase binary name on Linux / other UNIX
if(UNIX AND NOT APPLE)
    set_target_properties(Stepan PROPERTIES OUTPUT_NAME "stepan")
endif()


# ── Install rules (used by CPack / deploy scripts) ────────────────────────────
include(GNUInstallDirs)
install(TARGETS Stepan
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Linux: install icons and .desktop file
if(UNIX AND NOT APPLE)
    foreach(SIZE 16 24 32 64 128 256 512)
        install(FILES "${CMAKE_SOURCE_DIR}/icons/icon-${SIZE}x${SIZE}.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${SIZE}x${SIZE}/apps"
            RENAME "stepan.png"
        )
    endforeach()

    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/stepan.desktop.in"
        "${CMAKE_BINARY_DIR}/stepan.desktop"
        @ONLY
    )
    install(FILES "${CMAKE_BINARY_DIR}/stepan.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )
endif()

# ── Qt deploy helper (optional, invoked by build scripts) ────────────────────
qt_generate_deploy_app_script(
    TARGET Stepan
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
