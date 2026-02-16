# Cask Engine

Game engine with fixed-timestep loop and pure functional system transforms.

## Setup

```bash
cmake -B build -S .
```

## Commands

```bash
make test          # build and run specs
make test-watch    # re-run on file changes (requires entr)
make build         # build only
make clean         # remove build directory
./build/cask ./build/cubes_plugin.dylib # with cubes
```

## Dependencies

- CMake 3.14+
- C++20 compiler
- [entr](https://github.com/eradman/entr) (optional, for test-watch)
