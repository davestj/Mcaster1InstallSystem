cmake_minimum_required(VERSION 3.20)
project(MisCC VERSION 1.0.0 LANGUAGES CXX)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Qt6 setup — Core + Concurrent only (no GUI) ───────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Concurrent)

# ── Source files ──────────────────────────────────────────────────────────────
set(MISCC_SOURCES
    miscc.cpp

    # Manifest library (compiled inline)
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest/Manifest.cpp

    # Platform backends
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/MacOsBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/WindowsBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/LinuxBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CodeSigner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CertGenerator.cpp
)

set(MISCC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest/Manifest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../studio/BuilderProfile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/BuildBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/MacOsBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/WindowsBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/LinuxBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CodeSigner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends/CertGenerator.h
)

# ── Include dirs ──────────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../manifest
    ${CMAKE_CURRENT_SOURCE_DIR}/../studio      # BuilderProfile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../backends
)

# ── Build target — plain CLI executable (no .app bundle) ─────────────────────
add_executable(miscc
    ${MISCC_SOURCES}
    ${MISCC_HEADERS}
)

# ── Link ─────────────────────────────────────────────────────────────────────
target_link_libraries(miscc PRIVATE
    Qt6::Core
    Qt6::Concurrent
)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(miscc PRIVATE /W3)
else()
    target_compile_options(miscc PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ── Windows: windeployqt post-build (copies Qt DLLs to output dir) ───────────
if(WIN32)
    find_program(WINDEPLOYQT windeployqt
        HINTS "${Qt6_DIR}/../../../bin" "$ENV{QT_MSVC_DIR}/bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET miscc POST_BUILD
            COMMAND "${WINDEPLOYQT}"
                    "--no-translations"
                    "$<TARGET_FILE:miscc>"
            COMMENT "windeployqt: copying Qt DLLs to output directory"
        )
    else()
        message(STATUS "miscc: windeployqt not found — copy Qt DLLs manually")
    endif()
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS miscc RUNTIME DESTINATION bin)

# ── Status summary ────────────────────────────────────────────────────────────
message(STATUS "miscc: CLI compiler target configured")
message(STATUS "  Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output:      ${CMAKE_CURRENT_BINARY_DIR}/miscc")
