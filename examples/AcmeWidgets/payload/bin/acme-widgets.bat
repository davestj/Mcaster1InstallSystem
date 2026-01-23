@echo off
:: ──────────────────────────────────────────────────────────────────────────────
:: acme-widgets.bat  —  ACME Widgets 2.1.0  Windows wrapper  (example / demo)
::
:: This is a demonstration stub for the Mcaster1 Install System example project.
:: In a real distribution this would launch the actual application binary.
:: ──────────────────────────────────────────────────────────────────────────────

setlocal enabledelayedexpansion

set APP_NAME=Acme Widgets
set APP_VERSION=2.1.0
set APP_VENDOR=ACME Corporation
set INSTALL_DIR=%~dp0..

if "%~1"=="" goto :banner
if /i "%~1"=="--help" goto :banner
if /i "%~1"=="-h" goto :banner
if /i "%~1"=="version" goto :version
if /i "%~1"=="--version" goto :version
if /i "%~1"=="start" goto :start
if /i "%~1"=="stop" goto :stop
if /i "%~1"=="status" goto :status
if /i "%~1"=="list" goto :list
if /i "%~1"=="config" goto :config

echo ERROR: Unknown command '%~1'
echo Run: acme-widgets --help
exit /b 1

:banner
echo.
echo  +------------------------------------------+
echo  ^|  %APP_NAME% %APP_VERSION%                   ^|
echo  ^|  %APP_VENDOR%                      ^|
echo  +------------------------------------------+
echo.
echo Usage:  acme-widgets [command]
echo.
echo Commands:
echo   start       Start the widget service
echo   stop        Stop the widget service
echo   status      Show service status
echo   list        List all configured widgets
echo   config      Show current configuration
echo   version     Show version information
echo   --help      Show this message
echo.
echo Install directory: %INSTALL_DIR%
goto :eof

:version
echo %APP_NAME% %APP_VERSION%
echo Vendor:  %APP_VENDOR%
echo Support: https://support.acme-corp.example.com
goto :eof

:start
echo [INFO] Widget service starting... (demo - no real service started)
goto :eof

:stop
echo [WARN] Widget service stopping... (demo - no service was running)
goto :eof

:status
echo Service:  acme-widgets
echo Status:   installed (demo mode)
echo Version:  %APP_VERSION%
echo Config:   %INSTALL_DIR%\etc\acme-widgets.conf
goto :eof

:list
echo Configured widgets:
echo   1. acme-clock-widget      v1.0  [active]
echo   2. acme-weather-widget    v2.3  [active]
echo   3. acme-stock-widget      v1.8  [disabled]
echo.
echo (demo data - no real widgets are configured)
goto :eof

:config
if exist "%INSTALL_DIR%\etc\acme-widgets.conf" (
    echo Configuration (%INSTALL_DIR%\etc\acme-widgets.conf):
    echo.
    type "%INSTALL_DIR%\etc\acme-widgets.conf"
) else (
    echo WARN: Config file not found: %INSTALL_DIR%\etc\acme-widgets.conf
)
goto :eof

endlocal
