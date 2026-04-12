cmake_minimum_required(VERSION 3.20)
project(Mcaster1Installer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg SvgWidgets)

set(RUNTIME_SOURCES
    main.cpp
    InstallerWizard.cpp
    WelcomePage.cpp
    LicensePage.cpp
    PrerequisitesPage.cpp
    ComponentsPage.cpp
    DirectoryPage.cpp
    ReadyPage.cpp
    InstallPage.cpp
    FinishPage.cpp
    InstallEngine.cpp

    # Manifest (compiled inline)
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest/Manifest.cpp
)

set(RUNTIME_HEADERS
    InstallerWizard.h
    WelcomePage.h
    LicensePage.h
    ComponentsPage.h
    DirectoryPage.h
    ReadyPage.h
    InstallPage.h
    FinishPage.h
    InstallEngine.h
    InstallerStyle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest/Manifest.h
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest
)

# ── macOS app bundle ──────────────────────────────────────────────────────────
if(APPLE)
    # ── SVG → icns pipeline ──────────────────────────────────────────────────
    # Requires: rsvg-convert (brew install librsvg) + iconutil (macOS built-in)
    # Falls back to the pre-built mcaster1.icns when rsvg-convert is absent.
    find_program(RSVG_CONVERT rsvg-convert
        HINTS /opt/homebrew/bin /usr/local/bin)
    find_program(ICONUTIL iconutil)

    set(SVG_ICON    ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons/mcaster1.svg)
    set(ICNS_STATIC ${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons/mcaster1.icns)

    if(RSVG_CONVERT AND ICONUTIL AND EXISTS "${SVG_ICON}")
        set(ICONSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/mcaster1.iconset)
        set(APP_ICON    ${CMAKE_CURRENT_BINARY_DIR}/mcaster1.icns)

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
            COMMENT "Generating mcaster1.icns from SVG (Installer)"
        )
        set(ICNS_GENERATED TRUE)
        message(STATUS "Installer: SVG → icns pipeline active (${RSVG_CONVERT})")
    else()
        set(APP_ICON       ${ICNS_STATIC})
        set(ICNS_GENERATED FALSE)
        if(NOT RSVG_CONVERT)
            message(STATUS "Installer: rsvg-convert not found — using static mcaster1.icns "
                           "(run: brew install librsvg  to enable SVG→icns generation)")
        endif()
    endif()

    set_source_files_properties("${APP_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)

    add_executable(Mcaster1Installer MACOSX_BUNDLE
        ${RUNTIME_SOURCES}
        ${RUNTIME_HEADERS}
        "${APP_ICON}"
    )

    set_target_properties(Mcaster1Installer PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST  ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "Mcaster1 Installer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mcaster1.installer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE mcaster1.icns
    )
else()
    # Windows: embed .rc resource (icon + version info)
    set(WIN_RC ${CMAKE_CURRENT_SOURCE_DIR}/../windows/res/Mcaster1Installer.rc)
    add_executable(Mcaster1Installer WIN32
        ${RUNTIME_SOURCES}
        ${RUNTIME_HEADERS}
        ${WIN_RC}
    )
endif()

target_link_libraries(Mcaster1Installer PRIVATE
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
)

if(MSVC)
    target_compile_options(Mcaster1Installer PRIVATE /W3)
    # requireAdministrator UAC — set via linker flag so it owns the trustInfo
    # section (avoids mt.exe conflict with auto-generated manifest).
    # Merge our custom manifest (Windows 11 compat + DPI) as additional input.
    target_link_options(Mcaster1Installer PRIVATE
        "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'"
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/../windows/res/Mcaster1Installer.manifest"
    )
else()
    target_compile_options(Mcaster1Installer PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ── Windows: windeployqt post-build (runtime installer needs its own Qt DLLs) ─
if(WIN32)
    find_program(WINDEPLOYQT windeployqt
        HINTS "${Qt6_DIR}/../../../bin" "$ENV{QT_MSVC_DIR}/bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET Mcaster1Installer POST_BUILD
            COMMAND "${WINDEPLOYQT}"
                    "--no-translations"
                    "$<TARGET_FILE:Mcaster1Installer>"
            COMMENT "windeployqt: copying Qt DLLs for runtime installer"
        )
    endif()
endif()

install(TARGETS Mcaster1Installer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin)

# ── Copy docs into app bundle (macOS) ────────────────────────────────────────
if(APPLE)
    set(DOCS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../docs)
    set(DOCS_DST "$<TARGET_BUNDLE_CONTENT_DIR:Mcaster1Installer>/Resources/docs")
    add_custom_command(TARGET Mcaster1Installer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DOCS_DST}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${DOCS_SRC}" "${DOCS_DST}"
        COMMENT "Copying docs/ into app bundle Resources/docs/"
    )
endif()
