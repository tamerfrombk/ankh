# ankh

`ankh` is a scripting language and REPL shell with an emphasis on simplicity and readability.

### Language Reference

See [the wiki](https://github.com/tamerfrombk/ankh/wiki/Ankh).

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

First, build the project using the steps above. To install, move `ankhsh` to a location on your `PATH`. To uninstall, delete `ankhsh`.

### Design Goals

1. No third party dependencies
2. Single binary deployment
3. Internal consistency in language rules.

### Limitations

This project is considered to be in pre-alpha stage.
