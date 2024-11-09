# Building

### Prerequisites

[Clang](https://releases.llvm.org/download.html) | [Cmake](https://cmake.org/download/)

Windows

```console
winget install -e --id GnuWin32.Tar
winget install -e --id JernejSimoncic.Wget
winget install -e --id Kitware.CMake
winget install --id=Ninja-build.Ninja  -e
```

Build

```console
cmake -B build .
cmake --build build --target main
./build/bin/loud
```

Build release

```console
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build --config Release --target main
```

Run

```console
./build/bin/loud -h
```

## Gotchas

OpenMP not found on macOS

```console
brew install libomp
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
```

Windows doesn't produce `compile_commands.json` for clangd

Build with ninja backend

```console
cmake -G Ninja -B build . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```