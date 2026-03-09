
## How to run
```
mkdir build && cd build
cmake ..
make -j$(nproc)
./build_ir
```

## How to pass tests
```
# in the build folder
ctest --output-on-failure
```