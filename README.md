# Benoni

## Build

```
cmake -B build && cmake --build build
```

## Format

```
clang-format --style=file -i include/benoni/http.h src/apple/http.mm src/win32/http.cc src/linux/http.cc examples/http_example.cc test/postman-echo-get.cc
```

## Examples

Windows:

```
build\examples\Debug\http_example.exe
```

Non-Windows:

```
build/examples/http_example
```

## Test

```
ctest --test-dir build --output-on-failure --progress --parallel
```
