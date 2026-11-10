@echo off
setlocal
rem Use only public PyPI for this build; no persistent pip settings are changed.
rem Run only if company policy permits direct access to public PyPI.
rem Pip config files are bypassed; proxy environment variables are preserved.
rem Requires pip >=22.2 and Python >=3.10 for the Windows trust store feature.
rem Optional first argument: path to an IT-provided PEM CA bundle.
set "PIP_CONFIG_FILE=NUL"
set "PIP_INDEX_URL=https://pypi.org/simple"
set "PIP_EXTRA_INDEX_URL="
rem Keep HTTPS verification enabled, including certificates trusted by Windows.
set "PIP_USE_FEATURE=%PIP_USE_FEATURE% truststore"

if not "%~1"=="" (
    if not exist "%~f1" (
        echo ERROR: CA bundle not found: "%~f1"
        endlocal
        exit /b 1
    )
    set "PIP_CERT=%~f1"
    set "REQUESTS_CA_BUNDLE=%~f1"
)

pushd "%~dp0"
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

echo Building nodemcu-32s_runstats using public PyPI only...
echo HTTPS verification uses Windows trust and any supplied CA bundle.
"%PLATFORMIO_EXE%" run -e nodemcu-32s_runstats
set "BUILD_EXIT_CODE=%ERRORLEVEL%"

popd
endlocal & exit /b %BUILD_EXIT_CODE%