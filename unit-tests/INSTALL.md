# Install, Compile, and Run

---

## Compilation

Linux/MacOS:
```
mkdir build
cd build
cmake ../
make run
```
Windows
1. Make sure you have freetype install (Check README.MD)
2. Run Unit Tests:
```
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE="C:/Users/Matt/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
ninja
ninja run
OPTIONAL (if you want to see output):
cd ../
.\build\shmea-unit-tests.exe
```
