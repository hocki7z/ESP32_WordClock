@echo off
setlocal
rem Dependency preparation and pip settings are handled by PlatformIO's pre-script.
rem Optional first argument: path to an IT-provided PEM CA bundle.
if not "%~1"=="" (
    if not exist "%~f1" (
        echo ERROR: CA bundle not found: "%~f1"
        endlocal
        exit /b 1
    )
    set "PIP_CERT=%~f1"
    set "REQUESTS_CA_BUNDLE=%~f1"
)

rem Build from the project root, regardless of the caller's working directory.
pushd "%~dp0.."
if errorlevel 1 (
    endlocal
    exit /b 1
)

set "PLATFORMIO_EXE=C:\_DEV_TOOLS\VSCode\python-3.12.2.amd64\Scripts\platformio.exe"
if not exist "%PLATFORMIO_EXE%" (
    echo ERROR: PlatformIO executable not found: "%PLATFORMIO_EXE%"
    popd
    endlocal
    exit /b 1
)

"%PLATFORMIO_EXE%" run -e nodemcu-32s_runstats
set "BUILD_EXIT_CODE=%ERRORLEVEL%"
popd
endlocal & exit /b %BUILD_EXIT_CODE%