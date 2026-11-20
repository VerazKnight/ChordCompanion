# ChordCompanion

Group project for Software Engineering: a piano and chord-learning application.

Run all commands below from the repository root.

## Initialize dependencies

After cloning, run:

```sh
git submodule update --init --recursive
```

This downloads the bundled dependencies.

### Windows

With Visual Studio’s C++ build tools installed, using the Visual Studio generator, from the root: 

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Debug --target ChordCompanion --parallel 2
.\build\Debug\ChordCompanion.exe
```

After configuring with the Windows commands above, run tests with:

```powershell
cmake --build build --config Debug --target check --parallel 2
```

### Linux / Mac

With the project's dependencies installed, from the root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --target ChordCompanion --parallel 2
./build/ChordCompanion
```

Then build and run all unit tests:

```sh
cmake --build build --target check --parallel 2
```

### Test output

The `check` target rebuilds the tests before running them, prints diagnostic
output for failures, and returns a nonzero exit code if a test fails.