# Install, Compile, and Run

---

## Dependencies

### Debian

`cmake`

`make`

`g++`

`libfreetype6-dev`

### Fedora

sudo dnf install -y gcc gcc-c++ clang cmake make
sudo dnf install -y freetype-devel
sudo dnf install -y libasan

---

## Compilation

```
sh .configure.sh
```
or
```
mkdir build
cd build
cmake ..
make
```

---

## Installation

make install

---

## Uninstall

make uninstall

---

## Windows

### Prerequisites

Install [Strawberry Perl](https://strawberryperl.com/), which bundles `g++`, `cmake`, and `ninja`.

Add `C:\Strawberry\c\bin` to your system `PATH`.

FreeType is also required. Check if `C:\Strawberry\c\lib\libfreetype.a` already exists (Strawberry may include it). If not, install it via [vcpkg](https://github.com/microsoft/vcpkg):

```bash
vcpkg install freetype
```

### Compilation

From any shell with `C:\Strawberry\c\bin` in `PATH`, in the ShmeaDB root:

```bash
mkdir build
cd build
cmake .. -G Ninja
ninja
```

### Installation

```bash
ninja install
```

Installs to `%USERPROFILE%\shmea` (e.g. `C:\Users\YourName\shmea`).

### Unit Tests

```bash
cd unit-tests
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_PREFIX_PATH="$USERPROFILE/shmea"
ninja
cd ..
./build/shmea-unit-tests.exe
```
