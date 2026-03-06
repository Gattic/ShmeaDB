# Install, Compile, and Run

---

## Compilation

Linux/MacOS/Cygwin:

# Build

mkdir build
cd build
cmake ../

# run all tests
make run

# Run a specific suite
make run core
make run db
make run gnet
make run udp
make run images

Windows

# Build
mkdir -p build && cd build                                       
cmake .. -G Ninja
ninja
    
# Run all tests
cd ..
./build/shmea-unit-tests.exe

# Run a specific suite
./build/shmea-unit-tests.exe core
./build/shmea-unit-tests.exe db
./build/shmea-unit-tests.exe gnet
./build/shmea-unit-tests.exe udp
./build/shmea-unit-tests.exe images                                                 

Or using the CMake run target (sets the working directory automatically):

cd build
make run          # all tests
make run core     # specific suite

The working directory must be unit-tests/ (not build/) when running directly — the GDir tests look for a datasets/ subdirectory relative to CWD.
