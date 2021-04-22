# cleanshell

`clean` is a scripting language and shell with an emphasis on simplicity and readability. `clean` is a tentative name for the
language and is bound to change at any time.

### Running

`cleansh` will execute a shell while `cleansh <script>` will run the provided script.

### Dependencies

* [libreadline](https://tiswww.case.edu/php/chet/readline/rltop.html)
* [CMake](https://cmake.org/)

### Building

Once the dependencies above are installed on your system, run the following in the root of the source tree:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=release ..
cmake --build .
```

This will build the `cleansh` binary in the `build` directory.

### Installing/Uninstalling

TODO

### Design Goals

1. No third party dependencies
2. Single binary deployment
3. Only one obvious way of doing something
4. No feature creep

### Limitations

TODO