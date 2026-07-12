cmake_minimum_required(VERSION 3.16)

project(KarttaGUI VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6::Core is listed explicitly because PresetManager uses QJsonDocument,
# QStandardPaths, and QRegularExpression — all in Core, not Widgets.
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

# OpenCV is optional — required once image processing features land
find_package(OpenCV QUIET)

# ---------------------------------------------------------------------------
# Sources — shared across all platforms
# NOTE: filenames are case-sensitive on Linux. PresetManager.cpp must match
# the actual file on disk exactly (capital P, capital M).
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/KarttaRunner.cpp
    src/PreviewPanel.cpp
    src/SettingsPanel.cpp
    src/ToolsPanel.cpp
    src/PresetManager.cpp
)

# Windows resource file embeds the app icon and version info into the .exe.
if(WIN32)
    list(APPEND SOURCES resources/resources.rc)
endif()

add_executable(KarttaGUI ${SOURCES})

# Suppress the console window on Windows (no effect on other platforms)
if(WIN32)
    set_target_properties(KarttaGUI PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Expose the CMake project version to C++ so the About dialog stays in sync
# with the version declared at the top of this file — no duplication needed.
target_compile_definitions(KarttaGUI PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(KarttaGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(KarttaGUI PRIVATE
    Qt6::Widgets
    Qt6::Core
)

if(OpenCV_FOUND)
    target_link_libraries(KarttaGUI PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(KarttaGUI PRIVATE HAVE_OPENCV)
    message(STATUS "OpenCV found — image processing features enabled")
else()
    message(STATUS "OpenCV not found — image processing features disabled")
endif()


# ==============================================================================
# DEPLOYMENT
# ==============================================================================

install(TARGETS KarttaGUI RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# Bundle the karttapullautin folder (pullauta binary + pullauta.ini).
# Both pullauta.exe (Windows) and pullauta (Linux) can coexist here —
# MainWindow.cpp picks the right one at runtime via #ifdef Q_OS_WIN.
# USE_SOURCE_PERMISSIONS preserves the execute bit on Linux.
# ---------------------------------------------------------------------------
get_filename_component(KARTTA_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../karttapullautin" ABSOLUTE)

message(STATUS "Looking for karttapullautin folder at: ${KARTTA_DIR}")

if(EXISTS "${KARTTA_DIR}")
    install(DIRECTORY "${KARTTA_DIR}"
            DESTINATION bin
            USE_SOURCE_PERMISSIONS)
    message(STATUS "SUCCESS: Will bundle karttapullautin folder")
else()
    message(WARNING "karttapullautin folder not found at ${KARTTA_DIR}")
endif()


# ---------------------------------------------------------------------------
# Windows: copy Qt + OpenCV DLLs, then run windeployqt
# ---------------------------------------------------------------------------
if(WIN32)
    if(OpenCV_FOUND)
        get_filename_component(OPENCV_ROOT "${OpenCV_DIR}/../../.." ABSOLUTE)
        install(DIRECTORY "${OPENCV_ROOT}/bin/"
                DESTINATION bin
                FILES_MATCHING PATTERN "*.dll")
    endif()

    set(QT_BIN_DIR "${CMAKE_PREFIX_PATH}/bin")
    install(CODE "
        message(STATUS \"Running windeployqt to collect Qt dependencies...\")
        execute_process(
            COMMAND \"${QT_BIN_DIR}/windeployqt.exe\"
                    \"\${CMAKE_INSTALL_PREFIX}/bin/KarttaGUI.exe\"
            WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/bin\"
        )
    ")
endif()


# ---------------------------------------------------------------------------
# Linux: install desktop entry + icon for system installs and AppImage
# (linuxdeploy picks these up from usr/share/ automatically)
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    install(FILES     resources/KarttaGUI.desktop
            DESTINATION share/applications)
    install(FILES     resources/icon.png
            DESTINATION share/icons/hicolor/256x256/apps
            RENAME      KarttaGUI.png)
endif()