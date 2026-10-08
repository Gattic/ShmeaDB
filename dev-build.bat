@echo off
setlocal
set "SHMEA_BUILD_PRESET=windows-dev"
set "SHMEA_INSTALL_PREFIX=%USERPROFILE%\dev\installed"
pushd "%~dp0"
call build-and-install.bat %*
set "BUILD_RESULT=%errorlevel%"
popd
exit /b %BUILD_RESULT%
