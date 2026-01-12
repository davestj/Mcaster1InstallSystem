cmake_minimum_required(VERSION 3.20)
project(Mcaster1InstallStudio VERSION 1.0.0 LANGUAGES CXX)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt6 setup ─────────────────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)   # Using hand-written UI

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg SvgWidgets Concurrent)

# ── Source files ──────────────────────────────────────────────────────────────
set(STUDIO_SOURCES
    main.cpp
    StudioMainWindow.cpp
    ProjectSidebar.cpp
    AppInfoEditor.cpp
    FilesEditor.cpp
    BuildPanel.cpp
    ComponentsEditor.cpp
    ShortcutsEditor.cpp
    RegistryEditor.cpp
    SecurityEditor.cpp

    # Manifest library (compiled inline — no separate lib target)
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest/Manifest.cpp

    # Importers
    ${CMAKE_CURRENT_SOURCE_DIR}/../importers/NsisImporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../importers/InnoSetupImporter.cpp

    # Backends
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/MacOsBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/WindowsBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/LinuxBackend.cpp
)

set(STUDIO_HEADERS
    StudioMainWindow.h
    ProjectSidebar.h
    AppInfoEditor.h
    FilesEditor.h
    BuildPanel.h
    ComponentsEditor.h
    ShortcutsEditor.h
    RegistryEditor.h
    SecurityEditor.h
    StudioStyle.h
    SvgIcons.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest/Manifest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/BuildBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/MacOsBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/WindowsBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/LinuxBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../importers/NsisImporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../importers/InnoSetupImporter.h
)

# ── Include dirs ──────────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends
    ${CMAKE_CURRENT_SOURCE_DIR}/../importers
)

# ── macOS app bundle ──────────────────────────────────────────────────────────
if(APPLE)
    set(APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons/mcaster1.icns)
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    add_executable(Mcaster1InstallStudio MACOSX_BUNDLE
        ${STUDIO_SOURCES}
        ${STUDIO_HEADERS}
        ${APP_ICON}
    )

    set_target_properties(Mcaster1InstallStudio PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST  ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "Mcaster1 Install Studio"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mcaster1.installstudio"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE mcaster1.icns
    )
else()
    add_executable(Mcaster1InstallStudio WIN32
        ${STUDIO_SOURCES}
        ${STUDIO_HEADERS}
    )
endif()

# ── Link Qt6 ─────────────────────────────────────────────────────────────────
target_link_libraries(Mcaster1InstallStudio PRIVATE
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Concurrent
)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(Mcaster1InstallStudio PRIVATE /W3)
else()
    target_compile_options(Mcaster1InstallStudio PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
if(APPLE)
    install(TARGETS Mcaster1InstallStudio BUNDLE DESTINATION .)
else()
    install(TARGETS Mcaster1InstallStudio RUNTIME DESTINATION bin)
endif()
