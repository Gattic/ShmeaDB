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


## Visual Studio Preparations
1. Make sure you have Desktop development with C++
# Download VCPKG
1. git clone https://github.com/microsoft/vcpkg.git
2. cd vcpkg
3. .\bootstrap-vcpkg.bat
4. Add to your PATH

# Download freetype
1. vcpkg install freetype

# Run program
1. make the build directory in ShmeaDB
2. cd build
3. cmake .. -G Ninja
4. ninja
5. cmake --install .

# Run Unit Tests
1. vcpkg install freetype
2. cd unit-tests
3. make the build directory in unit-tests
4. cd build
5. cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE="C:/Users/Matt/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
6. ninja
7. .\shmea-unit-tests.exe
