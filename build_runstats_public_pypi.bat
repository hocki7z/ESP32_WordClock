@echo off
setlocal
rem Use only public PyPI for this build; no persistent pip settings are changed.
rem Run only if company policy permits direct access to public PyPI.
rem This also bypasses proxy/certificate settings stored in pip config files.
set "PIP_CONFIG_FILE=NUL"
set "PIP_INDEX_URL=https://pypi.org/simple"
set "PIP_EXTRA_INDEX_URL="

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
"%PLATFORMIO_EXE%" run -e nodemcu-32s_runstats
set "BUILD_EXIT_CODE=%ERRORLEVEL%"

popd
endlocal & exit /b %BUILD_EXIT_CODE%