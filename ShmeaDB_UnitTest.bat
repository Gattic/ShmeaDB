@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ============================================================
rem ShmeaDB_UnitTest.bat
rem Configure + build + run unit tests (Windows). Run from repo root.
rem ============================================================

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

set "BUILD_DIR=%ROOT_DIR%\build"

rem ---------- Defaults (override via env vars) ----------
if not defined BUILD_TYPE        set "BUILD_TYPE=Debug"
if not defined GENERATOR         set "GENERATOR="
if not defined JOBS              set "JOBS="
if not defined CLEAN             set "CLEAN=0"
if not defined CTEST_VERBOSE     set "CTEST_VERBOSE=1"
if not defined RUN_DIRECT        set "RUN_DIRECT=1"
if not defined TEST_ARGS         set "TEST_ARGS="

rem ---------- Clean ----------
if "%CLEAN%"=="1" (
  echo [UT] Cleaning build dir: "%BUILD_DIR%"
  if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

rem ---------- Configure ----------
echo [UT] Configuring (BUILD_TYPE=%BUILD_TYPE%)

set "CMAKE_ARGS= -S "%ROOT_DIR%" -B "%BUILD_DIR%" ^
 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
 -DSHMEA_BUILD_TESTS=ON"

if not "%GENERATOR%"=="" (
  set "CMAKE_ARGS= -G "%GENERATOR%" !CMAKE_ARGS!"
)

echo cmake !CMAKE_ARGS!
cmake !CMAKE_ARGS!
if errorlevel 1 exit /b 1

rem ---------- Build ----------
echo [UT] Building

set "BUILD_ARGS= --build "%BUILD_DIR%" --config %BUILD_TYPE%"

if not "%JOBS%"=="" (
  set "BUILD_ARGS=!BUILD_ARGS! --parallel %JOBS%"
) else (
  set "BUILD_ARGS=!BUILD_ARGS! --parallel"
)

echo cmake !BUILD_ARGS!
cmake !BUILD_ARGS!
if errorlevel 1 exit /b 1

rem ---------- Run via CTest ----------
echo [UT] Running tests via CTest

if "%CTEST_VERBOSE%"=="1" (
  ctest --test-dir "%BUILD_DIR%" -C %BUILD_TYPE% -V --output-on-failure
) else (
  ctest --test-dir "%BUILD_DIR%" -C %BUILD_TYPE% --output-on-failure
)

rem Do not hard-fail the script on test failure (change if you want)
rem If you want failure to stop the script, uncomment:
rem if errorlevel 1 exit /b 1

rem ---------- Run test exe directly for full stdout ----------
if not "%RUN_DIRECT%"=="1" (
  echo [UT] Done.
  exit /b 0
)

set "TEST_EXE="

rem Prefer common locations first
for %%P in (
  "%BUILD_DIR%\unit-tests\shmea_unit_tests.exe"
  "%BUILD_DIR%\unit-tests\%BUILD_TYPE%\shmea_unit_tests.exe"
  "%BUILD_DIR%\unit-tests\shmea-unit-tests.exe"
  "%BUILD_DIR%\unit-tests\%BUILD_TYPE%\shmea-unit-tests.exe"
  "%BUILD_DIR%\shmea_unit_tests.exe"
  "%BUILD_DIR%\%BUILD_TYPE%\shmea_unit_tests.exe"
) do (
  if exist %%~P (
    set "TEST_EXE=%%~P"
    goto :found_exe
  )
)

rem Fallback: search recursively
for /r "%BUILD_DIR%" %%F in (shmea_unit_tests.exe shmea-unit-tests.exe) do (
  set "TEST_EXE=%%F"
  goto :found_exe
)

:found_exe
if "%TEST_EXE%"=="" (
  echo [UT] Could not find the unit test executable.
  echo [UT] Done.
  exit /b 0
)

echo [UT] Also running test executable directly for full stdout:
echo      exe: "%TEST_EXE%"
echo      wd : "%ROOT_DIR%\unit-tests"
if not "%TEST_ARGS%"=="" echo      args: %TEST_ARGS%

pushd "%ROOT_DIR%\unit-tests" >nul
if not "%TEST_ARGS%"=="" (
  "%TEST_EXE%" %TEST_ARGS%
) else (
  "%TEST_EXE%"
)
popd >nul

echo [UT] Done.
exit /b 0
