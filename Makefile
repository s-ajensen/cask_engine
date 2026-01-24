.PHONY: build test test-watch run clean

build:
	@cmake --build build

test: build
	@./build/cask_redux_tests

test-watch:
	@find src spec -name '*.cpp' -o -name '*.hpp' | entr -c make test

run: build
	@./build/cask ./build/minimal_plugin.dylib

clean:
	@rm -rf build
