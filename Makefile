BIN_DIR = bin

.PHONY: configure debug release clean

configure:
	cmake -S . -B build

debug:
	cmake -S . -B debug
	cmake --build debug --config Debug

release:
	cmake -S . -B release
	cmake --build release --config Release

clean:
	cmake --build debug --target clean || true
	cmake --build release --target clean || true
