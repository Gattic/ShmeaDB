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
1. Make sure you have `Desktop development with C++` installed

# Download VCPKG
1. git clone https://github.com/microsoft/vcpkg.git
2. cd vcpkg
3. .\bootstrap-vcpkg.bat
4. Add to your PATH

# Download freetype
1. vcpkg install freetype

# Run program
```
make the build directory in ShmeaDB
cd build
cmake .. -G Ninja
ninja
cmake --install .
```
