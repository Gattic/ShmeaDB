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
cd build
cmake .. -G Ninja
ninja
ninja install
```
