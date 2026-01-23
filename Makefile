.PHONY: build test test-watch clean

build:
	@cmake --build build

test: build
	@./build/cask_redux_tests

test-watch:
	@find src spec -name '*.cpp' -o -name '*.hpp' | entr -c make test

clean:
	@rm -rf build
