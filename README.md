# shmea v0.58

## Website
http://shmea-db.com/

## Github

https://github.com/MeeseeksLookAtMe/ShmeaDB

---

## Discord

https://discord.gg/4DN2WMm

---

## About

This is a database and networking library for C++.
This is coded in C++98.

## Linux/MacOS:
```
mkdir build
cd build
cmake ../
make install
```


## Visual Studio Preparations (WINDOWS ONLY)
1. Make sure you have `Desktop development with C++` installed with Visual Studio
2. Download https://strawberryperl.com/
3. Install strawberry and set the path to environment variables (Example Env Var PATH add C:\Strawberry\c\bin)
4. Download VCPKG
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
Add to your PATH
```
5. Download freetype
```
vcpkg install freetype
```
6. Run program
```
make the build directory in ShmeaDB
mkdir build
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target install

cd C:\Users\Matt\dev\vcpkg
.\vcpkg.exe install sdl2 sdl2-image sdl2-ttf freetype --triplet x64-windows
```

Clean rebuild

set CLEAN=1
ShmeaDB_UnitTest.bat


Use Ninja

set GENERATOR=Ninja
ShmeaDB_UnitTest.bat

set CLEAN=1
call ShmeaDB_Install.bat

Install
ShmeaDB_Install.bat

Release install to custom prefix

set BUILD_TYPE=Release
set INSTALL_PREFIX=%USERPROFILE%\.local
ShmeaDB_Install.bat