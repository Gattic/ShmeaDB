@echo off
setlocal

echo ============================================
echo  ShmeaDB - Build and Install (Windows)
echo ============================================
echo.

:: --------------------------------------------------
:: 1. Initialize VS Developer Environment if needed
:: --------------------------------------------------
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] cl.exe not found. Searching for Visual Studio installation...
    set "VSDEVCMD="
    for %%Y in (2022 2025 18 17) do (
        for %%E in (BuildTools Community Professional Enterprise) do (
            for %%P in ("%ProgramFiles(x86)%" "%ProgramFiles%") do (
                if exist "%%~P\Microsoft Visual Studio\%%Y\%%E\Common7\Tools\VsDevCmd.bat" (
                    set "VSDEVCMD=%%~P\Microsoft Visual Studio\%%Y\%%E\Common7\Tools\VsDevCmd.bat"
                    goto :found_vs
                )
            )
        )
    )
    echo [ERROR] Could not find Visual Studio Build Tools or any VS edition.
    echo         Install VS Build Tools with the "Desktop development with C++" workload.
    echo         See INSTALL.md for details on supported Visual Studio versions.
    exit /b 1

    :found_vs
    echo [INFO] Found: %VSDEVCMD%
    call "%VSDEVCMD%" -arch=amd64 >nul 2>&1
    echo [OK] VS Developer Environment initialized.
) else (
    echo [OK] cl.exe already available.
)

:: --------------------------------------------------
:: 2. Verify VCPKG_ROOT and fix if overridden by VS
:: --------------------------------------------------
:: The VS Developer Shell may override VCPKG_ROOT to a bundled vcpkg
:: that only supports manifest mode (no classic installs / no freetype).
:: Detect this and fall back to the user's own vcpkg.

if not defined VCPKG_ROOT (
    echo [ERROR] VCPKG_ROOT is not set.
    echo         Set it to your vcpkg installation, e.g.:
    echo           set VCPKG_ROOT=C:\vcpkg
    exit /b 1
)

:: Check if the current VCPKG_ROOT has freetype installed
if not exist "%VCPKG_ROOT%\installed\x64-windows\lib\freetype.lib" (
    echo [WARN] Freetype not found at %VCPKG_ROOT%\installed\x64-windows\lib\freetype.lib
    echo        The VS Developer Shell may have overridden VCPKG_ROOT.
    echo.
    :: Try common user vcpkg locations
    if exist "C:\vcpkg\installed\x64-windows\lib\freetype.lib" (
        echo [FIX] Found freetype at C:\vcpkg. Resetting VCPKG_ROOT.
        set "VCPKG_ROOT=C:\vcpkg"
    ) else if exist "%USERPROFILE%\vcpkg\installed\x64-windows\lib\freetype.lib" (
        echo [FIX] Found freetype at %USERPROFILE%\vcpkg. Resetting VCPKG_ROOT.
        set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
    ) else if exist "%USERPROFILE%\dev\vcpkg\installed\x64-windows\lib\freetype.lib" (
        echo [FIX] Found freetype at %USERPROFILE%\dev\vcpkg. Resetting VCPKG_ROOT.
        set "VCPKG_ROOT=%USERPROFILE%\dev\vcpkg"
    ) else (
        echo [ERROR] Could not find freetype in any known vcpkg location.
        echo         Install it with: vcpkg install freetype
        exit /b 1
    )
)

echo [OK] VCPKG_ROOT = %VCPKG_ROOT%

:: --------------------------------------------------
:: 3. Verify prerequisites
:: --------------------------------------------------
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] cmake not found on PATH.
    exit /b 1
)
echo [OK] cmake found.

where ninja >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] ninja not found on PATH.
    exit /b 1
)
echo [OK] ninja found.

echo.

:: --------------------------------------------------
:: 4. Configure
:: --------------------------------------------------
echo [STEP] Configuring with CMake preset 'windows-release'...
cmake --preset windows-release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)
echo [OK] Configure succeeded.
echo.

:: --------------------------------------------------
:: 5. Build
:: --------------------------------------------------
echo [STEP] Building...
cmake --build --preset windows-release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)
echo [OK] Build succeeded.
echo.

:: --------------------------------------------------
:: 6. Install
:: --------------------------------------------------
echo [STEP] Installing to %USERPROFILE%\shmea ...
cmake --install build
if %errorlevel% neq 0 (
    echo [ERROR] Install failed.
    exit /b 1
)
echo [OK] Installed to %USERPROFILE%\shmea
echo.

echo ============================================
echo  Done! ShmeaDB installed to %USERPROFILE%\shmea
echo ============================================

endlocal
