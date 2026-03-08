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

Install [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) with the **"Desktop development with C++"** workload. This provides `cl.exe` (MSVC compiler), `cmake`, and `ninja`.

> **Note:** The Visual Studio installation path varies depending on the version and edition you install. Common paths include:
>
> | Version | Path |
> |---------|------|
> | VS 2022 Build Tools | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
> | VS 2022 Community | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
> | VS 2025 Build Tools | `C:\Program Files (x86)\Microsoft Visual Studio\2025\BuildTools` |
> | VS 18 Build Tools | `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools` |
>
> The `.bat` scripts (`build-and-install.bat` and `unit-tests/build-and-run.bat`) will automatically search for known VS versions and editions. If your installation uses a non-standard path, you can either:
> 1. Open a **Developer Command Prompt for VS** before running the script (so `cl.exe` is already on PATH), or
> 2. Manually call `VsDevCmd.bat` from your VS installation before running the build.
>
> To find your installation path, run in PowerShell:
> ```powershell
> & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -all -property installationPath
> ```

Install [vcpkg](https://github.com/microsoft/vcpkg):

```bash
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

Set the `VCPKG_ROOT` environment variable to `C:\vcpkg` (or wherever you cloned it).

Install FreeType:

```bash
vcpkg install freetype
```

### Compilation (Command Line)

Open a **Developer Command Prompt for VS** (or run `vcvarsall.bat x64`). If using PowerShell, launch the VS Developer Shell (adjust the path to match your VS installation):

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64
```

If your VS version differs (e.g. `18` or `2025`), replace `2022\BuildTools` with your version and edition.

**Note:** The Developer Shell may override `VCPKG_ROOT` to point to a vcpkg bundled with Visual Studio. If `cmake` fails with `Could NOT find Freetype`, reset it to your own vcpkg:

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"   # adjust to your vcpkg path
```

Then:

```bash
cmake --preset windows-release
cmake --build --preset windows-release
```

### Compilation (VS Code)

1. Install the **CMake Tools** extension (`ms-vscode.cmake-tools`)
2. Open the ShmeaDB folder in VS Code
3. CMake Tools will detect `CMakePresets.json` — select the `windows-release` preset when prompted
4. Build with `Ctrl+Shift+B` or click **Build** in the status bar

**Note:** The CMake Tools extension may auto-detect a vcpkg bundled with Visual Studio and override the toolchain file. If the build fails with `Could NOT find Freetype`, add the following to `.vscode/settings.json`:

```json
{
    "cmake.useVcpkgToolchainFile": false,
    "cmake.configureArgs": [
        "-DCMAKE_TOOLCHAIN_FILE=C:/path/to/your/vcpkg/scripts/buildsystems/vcpkg.cmake"
    ]
}
```

Replace the path with your actual vcpkg location (matching your `VCPKG_ROOT` environment variable).

### Installation

```bash
cmake --install build
```

Installs to `%USERPROFILE%\shmea` (e.g. `C:\Users\YourName\shmea`).

### Unit Tests (Command Line)

Make sure `shmea.dll` and `freetype.dll` are on your PATH:

```powershell
$env:PATH = "$env:USERPROFILE\shmea\bin;$env:VCPKG_ROOT\installed\x64-windows\bin;$env:PATH"
```

Then build and run:

```powershell
cd unit-tests
cmake --preset windows-release
cmake --build --preset windows-release
.\build\shmea-unit-tests.exe
```

### Unit Tests (VS Code)

1. Open the `unit-tests` folder in VS Code (`File > Open Folder`)
2. CMake Tools will detect `CMakePresets.json` — select the `windows-release` preset
3. Build with `Ctrl+Shift+B` or click **Build** in the status bar
4. Set the launch target to `shmea-unit-tests` (click the launch target in the status bar)
5. Run with the **▶** button in the status bar

A `.vscode/settings.json` is included that sets the working directory and DLL paths automatically.
