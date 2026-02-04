@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ============================================================
REM ShmeaDB_Install.bat
REM Configure + build + install ShmeaDB (Windows). From repo root.
REM ============================================================

REM --- Resolve repo root (directory of this .bat) ---
set "ROOT_DIR=%~dp0"
REM Remove trailing backslash if present
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

set "BUILD_DIR=%ROOT_DIR%\build-install"

REM --- Defaults (override via env vars before running) ---
if not defined BUILD_TYPE      set "BUILD_TYPE=Release"
if not defined INSTALL_PREFIX  set "INSTALL_PREFIX=%USERPROFILE%\.local"
if not defined GENERATOR       set "GENERATOR="
if not defined PLATFORM        set "PLATFORM="
if not defined JOBS            set "JOBS="
if not defined CLEAN           set "CLEAN=0"
if not defined WITH_TESTS      set "WITH_TESTS=0"

REM --- Convert WITH_TESTS to ON/OFF for CMake ---
set "TESTS_FLAG=OFF"
if /I "%WITH_TESTS%"=="1" set "TESTS_FLAG=ON"
if /I "%WITH_TESTS%"=="ON" set "TESTS_FLAG=ON"
if /I "%WITH_TESTS%"=="TRUE" set "TESTS_FLAG=ON"

REM --- Clean build dir if requested ---
if /I "%CLEAN%"=="1" (
  echo [INSTALL] Cleaning build dir: "%BUILD_DIR%"
  if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [INSTALL] Configuring (BUILD_TYPE=%BUILD_TYPE%)
set "CMAKE_ARGS=-S "%ROOT_DIR%" -B "%BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" -DSHMEA_BUILD_TESTS=%TESTS_FLAG%"

REM --- Generator selection ---
if not "%GENERATOR%"=="" (
  set "CMAKE_ARGS=%CMAKE_ARGS% -G "%GENERATOR%""
)

REM --- Optional platform (useful for Visual Studio generators) ---
if not "%PLATFORM%"=="" (
  set "CMAKE_ARGS=%CMAKE_ARGS% -A %PLATFORM%"
)

REM IMPORTANT:
REM For single-config generators (Ninja/MinGW Makefiles), CMAKE_BUILD_TYPE matters:
REM For multi-config (Visual Studio), --config controls it.
REM We pass -DCMAKE_BUILD_TYPE anyway; it's ignored by multi-config generators.
set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%"

echo cmake %CMAKE_ARGS%
cmake %CMAKE_ARGS%
if errorlevel 1 goto :fail

echo [INSTALL] Building
REM --- Build command ---
REM Multi-config generators need: --config %BUILD_TYPE%
set "BUILD_ARGS=--build "%BUILD_DIR%""
REM Always pass --config; CMake ignores it for single-config generators.
set "BUILD_ARGS=%BUILD_ARGS% --config %BUILD_TYPE%"

if not "%JOBS%"=="" (
  set "BUILD_ARGS=%BUILD_ARGS% -j %JOBS%"
)

echo cmake %BUILD_ARGS%
cmake %BUILD_ARGS%
if errorlevel 1 goto :fail

echo [INSTALL] Installing to: "%INSTALL_PREFIX%"
REM --- Install command ---
set "INSTALL_ARGS=--install "%BUILD_DIR%" --config %BUILD_TYPE%"
echo cmake %INSTALL_ARGS%
cmake %INSTALL_ARGS%
if errorlevel 1 goto :fail

echo [INSTALL] Done.
echo [INSTALL] Tip: if another project uses find_package(shmea), ensure CMAKE_PREFIX_PATH contains:
echo          "%INSTALL_PREFIX%"

exit /b 0

:fail
echo [INSTALL] FAILED with errorlevel %errorlevel%.
exit /b %errorlevel%
