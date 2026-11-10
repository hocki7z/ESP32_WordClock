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

call :prepare_littlefs
if errorlevel 1 (
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

:prepare_littlefs
set "GIT_EXE=C:\_DEV_TOOLS\VSCode\GitPortable\App\Git\bin\git.exe"
if not exist "%GIT_EXE%" set "GIT_EXE=git"
"%GIT_EXE%" --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Git is required to check out LittleFS.
    exit /b 1
)

set "LITTLEFS_DIR=components\littlefs"
set "LITTLEFS_COMMIT=c3fe82d4d2794c7db7a9b5c7d9ec151e32fc001b"
if not exist "%LITTLEFS_DIR%" (
    echo Checking out LittleFS v1.14.8 and its submodule...
    "%GIT_EXE%" clone --branch v1.14.8 --depth 1 --recurse-submodules --shallow-submodules https://github.com/joltwallet/esp_littlefs.git "%LITTLEFS_DIR%"
    if errorlevel 1 exit /b 1
)

rem Never reset or overwrite an existing checkout or vendored source directory.
if not exist "%LITTLEFS_DIR%\.git" (
    echo ERROR: Existing LittleFS directory is not a Git checkout. Leaving it unchanged.
    exit /b 1
)
rem Avoid nested command quoting and pipe-based matching when reading HEAD.
set "LITTLEFS_HEAD_FILE=%TEMP%\wordclock-littlefs-head-%RANDOM%-%RANDOM%.txt"
"%GIT_EXE%" -C "%LITTLEFS_DIR%" rev-parse --verify HEAD >"%LITTLEFS_HEAD_FILE%"
if errorlevel 1 (
    del /q "%LITTLEFS_HEAD_FILE%" >nul 2>&1
    echo ERROR: Cannot read the LittleFS checkout revision.
    exit /b 1
)
set "LITTLEFS_HEAD="
set /p LITTLEFS_HEAD=<"%LITTLEFS_HEAD_FILE%"
del /q "%LITTLEFS_HEAD_FILE%" >nul 2>&1
if /i not "%LITTLEFS_HEAD%"=="%LITTLEFS_COMMIT%" (
    echo ERROR: LittleFS checkout is not the pinned v1.14.8 commit. Leaving it unchanged.
    echo Expected: %LITTLEFS_COMMIT%
    echo Actual:   %LITTLEFS_HEAD%
    exit /b 1
)

"%GIT_EXE%" -C "%LITTLEFS_DIR%" submodule update --init --recursive
if errorlevel 1 exit /b 1
if not exist "%LITTLEFS_DIR%\include\esp_littlefs.h" exit /b 1
if not exist "%LITTLEFS_DIR%\src\littlefs\lfs.c" exit /b 1
exit /b 0