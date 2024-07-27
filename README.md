# ankh

`ankh` is a scripting language and REPL shell with an emphasis on simplicity and readability.

## Language Reference

See [the wiki](https://github.com/tamerfrombk/ankh/wiki/Ankh).

## Running

`ankhsh` will execute a shell while `ankhsh <script>` will run the provided script.

## Building

Once the dependencies above are installed on your system, run the following in the root of the source tree:

```sh
cmake -S . -B build
cmake --build build --config release
```

This will build the `ankhsh` binary in the `build` directory.

## Testing

After building, navigate into the build directory and execute the test executable: `ankhtests`.

## Installing/Uninstalling

First, build the project using the steps above. To install, move `ankhsh` to a location on your `PATH`. To uninstall, delete `ankhsh`.

## Design Goals

1. No third party dependencies
2. Single binary deployment
3. Internal consistency in language rules.

### Limitations

This project is considered to be in pre-alpha stage.
