# Install, Compile, and Run

---

## Dependencies

Linux/Cygwin:

`cmake`

`make`

`gcc`

`g++`

Cygwin:

`xinit`

MacOS:

Install homebrew, a macOS package manager.

`/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`
 
`brew install cmake`

Install Xcode command line tools.

`$ xcode-select --install`

`$ brew install pkgconfig`

---

## Installation

sudo make install

---

## Uninstall

sudo make uninstall

---

## Compilation

Linux/MacOS/Windows/Cygwin:
```
mkdir build
cd build
cmake ../
make
```
