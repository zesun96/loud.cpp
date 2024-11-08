# Building

### Prerequisites

[Clang](https://releases.llvm.org/download.html) | [Cmake](https://cmake.org/download/)

Windows

```console
winget install -e --id GnuWin32.Tar
winget install -e --id JernejSimoncic.Wget
winget install -e --id Kitware.CMake
```

Build

```console
cmake -B build .
cmake --build build --target loud
./build/bin/loud
```

Build release

```console
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build --config Release --target loud
```

## Gotchas

OpenMP not found on macOS

```console
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
```
