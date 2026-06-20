cmake_minimum_required(VERSION 3.20)
project(Stepan VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Version & build info ───────────────────────────────────────────────────────
# User-visible version (may include pre-release suffix); used in metadata/UI
set(APP_VERSION_FULL "0.1beta")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT GIT_COMMIT_HASH OR GIT_COMMIT_HASH STREQUAL "")
    set(GIT_COMMIT_HASH "unknown")
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
    @ONLY
)

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
# Build only the static library; disable shared to avoid missing .so/.dll at runtime.
# qt_standard_project_setup() sets BUILD_SHARED_LIBS=ON on Linux (Qt is shared there),
# so we must shadow it with a non-cache variable before GeographicLib processes it.
set(GEOGRAPHICLIB_LIB_TYPE   STATIC CACHE STRING "" FORCE)
set(GEOGRAPHICLIB_SHARED_LIB OFF    CACHE BOOL   "" FORCE)
set(GEOGRAPHICLIB_STATIC_LIB ON     CACHE BOOL   "" FORCE)
set(BUILD_DOCUMENTATION      OFF    CACHE BOOL   "" FORCE)
set(EXAMPLEDIR               ""     CACHE PATH   "" FORCE)
set(BUILD_SHARED_LIBS OFF)
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

target_include_directories(Stepan PRIVATE src "${CMAKE_BINARY_DIR}/generated")

target_link_libraries(Stepan PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::Network
    GeographicLib::GeographicLib_STATIC
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
        MACOSX_BUNDLE_BUNDLE_VERSION       "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION_FULL}"
        MACOSX_BUNDLE_LONG_VERSION_STRING  "Stepan ${APP_VERSION_FULL} (${GIT_COMMIT_HASH})"
        MACOSX_BUNDLE_COPYRIGHT            "Copyright © 2026 Andrii Kondratiev"
        MACOSX_BUNDLE_INFO_STRING          "Stepan ${APP_VERSION_FULL}, Copyright © 2026 Andrii Kondratiev"
        MACOSX_BUNDLE_ICON_FILE            "app.icns"
        MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
    )
endif()

if(WIN32)
    configure_file(
        "${CMAKE_SOURCE_DIR}/resources/app.rc.in"
        "${CMAKE_BINARY_DIR}/app.rc"
        @ONLY
    )
    target_sources(Stepan PRIVATE "${CMAKE_BINARY_DIR}/app.rc")
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

# ── CPack packaging (Linux only) ──────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    set(CPACK_PACKAGE_NAME                "stepan")
    set(CPACK_PACKAGE_VENDOR              "Andrii Kondratiev")
    set(CPACK_PACKAGE_CONTACT             "Andrii Kondratiev <h0st@ukr.net>")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Coordinate system converter (MGRS, DD, DMS, UTM, USK-2000)")
    set(CPACK_PACKAGE_VERSION             "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_HOMEPAGE_URL        "https://github.com/keedhost/stepan")
    set(CPACK_STRIP_FILES                 ON)

    # DEB (Debian / Ubuntu)
    set(CPACK_DEBIAN_PACKAGE_SECTION      "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY     "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libqt6webenginewidgets6 (>= 6.2), libqt6widgets6 (>= 6.2), libqt6network6 (>= 6.2), libqt6webchannel6 (>= 6.2)")
    set(CPACK_DEBIAN_FILE_NAME            "DEB-DEFAULT")

    # RPM (Fedora / RHEL / openSUSE)
    set(CPACK_RPM_PACKAGE_LICENSE         "GPL-3.0")
    set(CPACK_RPM_PACKAGE_GROUP           "Applications/Productivity")
    set(CPACK_RPM_PACKAGE_REQUIRES        "qt6-qtbase >= 6.2, qt6-qtwebengine >= 6.2")
    set(CPACK_RPM_FILE_NAME               "RPM-DEFAULT")
    # Prevent RPM from complaining about owning standard system directories
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        "/usr/share/applications"
        "/usr/share/icons"
        "/usr/share/icons/hicolor"
        "/usr/share/icons/hicolor/16x16"   "/usr/share/icons/hicolor/16x16/apps"
        "/usr/share/icons/hicolor/24x24"   "/usr/share/icons/hicolor/24x24/apps"
        "/usr/share/icons/hicolor/32x32"   "/usr/share/icons/hicolor/32x32/apps"
        "/usr/share/icons/hicolor/64x64"   "/usr/share/icons/hicolor/64x64/apps"
        "/usr/share/icons/hicolor/128x128" "/usr/share/icons/hicolor/128x128/apps"
        "/usr/share/icons/hicolor/256x256" "/usr/share/icons/hicolor/256x256/apps"
        "/usr/share/icons/hicolor/512x512" "/usr/share/icons/hicolor/512x512/apps"
    )

    include(CPack)
endif()

# ── Qt deploy helper (macOS + Windows only — Linux uses system Qt packages) ──
if(NOT (UNIX AND NOT APPLE))
    qt_generate_deploy_app_script(
        TARGET Stepan
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
