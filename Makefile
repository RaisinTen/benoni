# Programs
CMAKE = cmake
CTEST = ctest
CLANG_FORMAT = clang-format
# See https://stackoverflow.com/a/30906085 \
!ifndef 0 # \
EXAMPLE = build\examples\Debug\http_example.exe # \
!else
EXAMPLE = build/examples/http_example
# \
!endif

configure: .always
	$(CMAKE) -B build -DBENONI_TESTS:BOOL=ON -DBENONI_EXAMPLES:BOOL=ON

build: .always
	$(CLANG_FORMAT) --style=file -i include/benoni/http.h src/apple/http.mm src/win32/http.cc src/linux/http.cc examples/http_example.cc test/unit/postman-echo-get.cc test/packaging/project/project.cc
	$(CMAKE) --build build
	$(CMAKE) --install build --prefix build/dist --config Debug --component benoni --verbose

example: .always
	$(EXAMPLE)

test: .always
	$(CTEST) -C Debug --test-dir build --parallel --output-on-failure

# For NMake, which doesn't support .PHONY
.always:
