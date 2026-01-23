cmake_minimum_required(VERSION 3.20)
project(Mcaster1InstallStudio VERSION 1.0.0 LANGUAGES CXX)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # enables clangd / IDE IntelliSense

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
    PrerequisitesEditor.cpp
    CustomActionsEditor.cpp
    BuilderProfileDialog.cpp
    CodeSignDialog.cpp
    HelpPanel.cpp

    # Manifest library (compiled inline — no separate lib target)
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest/Manifest.cpp

    # Importers
    ${CMAKE_CURRENT_SOURCE_DIR}/../importers/NsisImporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../importers/InnoSetupImporter.cpp

    # Backends (including code signing + cert generation)
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/MacOsBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/WindowsBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/LinuxBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CodeSigner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CertGenerator.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CodeSigner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CertGenerator.h
    CodeSignDialog.h
    HelpPanel.h
    BuilderProfile.h
    BuilderProfileDialog.h
    EventLog.h
    BuildHistory.h
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
    # ── SVG → icns pipeline ──────────────────────────────────────────────────
    # Requires: rsvg-convert (brew install librsvg) + iconutil (macOS built-in)
    # If rsvg-convert is not found, falls back to the pre-built mcaster1.icns.
    find_program(RSVG_CONVERT rsvg-convert
        HINTS /opt/homebrew/bin /usr/local/bin)
    find_program(ICONUTIL iconutil)

    set(SVG_ICON    ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons/mcaster1.svg)
    set(ICNS_STATIC ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons/mcaster1.icns)

    if(RSVG_CONVERT AND ICONUTIL AND EXISTS "${SVG_ICON}")
        set(ICONSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/mcaster1.iconset)
        set(APP_ICON    ${CMAKE_CURRENT_BINARY_DIR}/mcaster1.icns)

        # Standard Apple iconset sizes: 16 32 64 128 256 512 and @2x variants
        add_custom_command(
            OUTPUT  "${APP_ICON}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ICONSET_DIR}"
            COMMAND ${RSVG_CONVERT} -w 16   -h 16   "${SVG_ICON}" -o "${ICONSET_DIR}/icon_16x16.png"
            COMMAND ${RSVG_CONVERT} -w 32   -h 32   "${SVG_ICON}" -o "${ICONSET_DIR}/icon_16x16@2x.png"
            COMMAND ${RSVG_CONVERT} -w 32   -h 32   "${SVG_ICON}" -o "${ICONSET_DIR}/icon_32x32.png"
            COMMAND ${RSVG_CONVERT} -w 64   -h 64   "${SVG_ICON}" -o "${ICONSET_DIR}/icon_32x32@2x.png"
            COMMAND ${RSVG_CONVERT} -w 128  -h 128  "${SVG_ICON}" -o "${ICONSET_DIR}/icon_128x128.png"
            COMMAND ${RSVG_CONVERT} -w 256  -h 256  "${SVG_ICON}" -o "${ICONSET_DIR}/icon_128x128@2x.png"
            COMMAND ${RSVG_CONVERT} -w 256  -h 256  "${SVG_ICON}" -o "${ICONSET_DIR}/icon_256x256.png"
            COMMAND ${RSVG_CONVERT} -w 512  -h 512  "${SVG_ICON}" -o "${ICONSET_DIR}/icon_256x256@2x.png"
            COMMAND ${RSVG_CONVERT} -w 512  -h 512  "${SVG_ICON}" -o "${ICONSET_DIR}/icon_512x512.png"
            COMMAND ${RSVG_CONVERT} -w 1024 -h 1024 "${SVG_ICON}" -o "${ICONSET_DIR}/icon_512x512@2x.png"
            COMMAND ${ICONUTIL} -c icns -o "${APP_ICON}" "${ICONSET_DIR}"
            DEPENDS "${SVG_ICON}"
            COMMENT "Generating mcaster1.icns from SVG (Studio)"
        )
        set(ICNS_GENERATED TRUE)
        message(STATUS "Studio: SVG → icns pipeline active (${RSVG_CONVERT})")
    else()
        set(APP_ICON       ${ICNS_STATIC})
        set(ICNS_GENERATED FALSE)
        if(NOT RSVG_CONVERT)
            message(STATUS "Studio: rsvg-convert not found — using static mcaster1.icns "
                           "(run: brew install librsvg  to enable SVG→icns generation)")
        endif()
    endif()

    set_source_files_properties("${APP_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)

    add_executable(Mcaster1InstallStudio MACOSX_BUNDLE
        ${STUDIO_SOURCES}
        ${STUDIO_HEADERS}
        "${APP_ICON}"
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

# ── Copy docs into app bundle (macOS) ────────────────────────────────────────
if(APPLE)
    set(DOCS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../docs)
    set(DOCS_DST "$<TARGET_BUNDLE_CONTENT_DIR:Mcaster1InstallStudio>/Resources/docs")
    add_custom_command(TARGET Mcaster1InstallStudio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DOCS_DST}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${DOCS_SRC}" "${DOCS_DST}"
        COMMENT "Copying docs/ into app bundle Resources/docs/"
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
if(APPLE)
    install(TARGETS Mcaster1InstallStudio BUNDLE DESTINATION .)
else()
    install(TARGETS Mcaster1InstallStudio RUNTIME DESTINATION bin)
endif()
