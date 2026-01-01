cmake_minimum_required(VERSION 3.20)
project(Mcaster1InstallSystem VERSION 2.5.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
qt_standard_project_setup()

set(SOURCES
    main.cpp
    InstallerWizard.cpp
    WelcomePage.cpp
    ComponentsPage.cpp
    InstallPage.cpp
    FinishPage.cpp
    InstallWorker.cpp
)

set(HEADERS
    InstallerWizard.h
    WelcomePage.h
    ComponentsPage.h
    InstallPage.h
    FinishPage.h
    InstallWorker.h
)

qt_add_executable(Mcaster1InstallSystem MACOSX_BUNDLE ${SOURCES} ${HEADERS})

target_link_libraries(Mcaster1InstallSystem PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
)

set_target_properties(Mcaster1InstallSystem PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST              "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    MACOSX_BUNDLE_ICON_FILE              "mcaster1"
    MACOSX_BUNDLE_BUNDLE_NAME            "Mcaster1InstallSystem"
    MACOSX_BUNDLE_GUI_IDENTIFIER         "com.mcaster1.Mcaster1InstallSystem"
    MACOSX_BUNDLE_BUNDLE_VERSION         "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING   "${PROJECT_VERSION}"
)

# ── Post-build: copy static resources into the .app bundle ─────────────────────
# All resources live in resources/ next to this CMakeLists.txt.
# The payload/ directory (Mcaster1DNAS.app + service binary + templates) is
# injected into Contents/Resources/payload/ by build.sh AFTER cmake builds this
# app, before macdeployqt runs.  It is NOT part of the cmake build.
add_custom_command(TARGET Mcaster1InstallSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:Mcaster1InstallSystem>/Contents/Resources"
    # App icon
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/mcaster1.icns"
        "$<TARGET_BUNDLE_DIR:Mcaster1InstallSystem>/Contents/Resources/mcaster1.icns"
    # Side panel — portrait column (Welcome + Finish pages)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/sidepanel.png"
        "$<TARGET_BUNDLE_DIR:Mcaster1InstallSystem>/Contents/Resources/sidepanel.png"
    # Header banner — top strip (all pages)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/header-banner.png"
        "$<TARGET_BUNDLE_DIR:Mcaster1InstallSystem>/Contents/Resources/header-banner.png"
    # Splash image — retained for DMG background (build.sh)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/splash.png"
        "$<TARGET_BUNDLE_DIR:Mcaster1InstallSystem>/Contents/Resources/splash.png"
    COMMENT "Copying installer resources (icon, side panel, banner, splash)"
)
