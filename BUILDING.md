# Building


### Prerequisites

[Clang](https://releases.llvm.org/download.html) | [Cmake](https://cmake.org/download/)

Build

```console
cmake -B build .
cmake --build build
./build/bin/loud
```

Build release

```console
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build --config Release
```