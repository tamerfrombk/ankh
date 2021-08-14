# ankh

`ankh` is a scripting language and REPL shell with an emphasis on simplicity and readability.

### Running

`ankhsh` will execute a shell while `ankhsh <script>` will run the provided script.

### Dependencies

- [CMake](https://cmake.org/)

### Building

Once the dependencies above are installed on your system, run the following in the root of the source tree:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=release ..
cmake --build .
```

This will build the `ankhsh` binary in the `build` directory.

### Installing/Uninstalling

TODO

### Design Goals

1. No third party dependencies
2. Single binary deployment
3. Only one obvious way of doing something
4. No feature creep

### Limitations

TODO
