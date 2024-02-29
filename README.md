# req

## Build

```
cmake -B build && cmake --build build
```

## Format
```
clang-format --style=file -i include/http.h src/apple/http.mm test/http.cc
```

## Test

```
ctest --test-dir build --output-on-failure --progress --parallel
```
